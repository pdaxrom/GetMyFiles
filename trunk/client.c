#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <sys/select.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#else
#include <windows.h>
#endif
#include <limits.h>
#include <signal.h>

#include "tcp.h"
#include "urldecode.h"
#include "utils.h"
#include "http.h"
#include "httpd.h"

#ifdef CLIENT_GUI
#include <client.h>
#endif

#ifdef __MINGW32__
void *alloca(size_t);
#endif

#define BUF_SIZE	1024
#define KEY_SIZE	16

typedef struct upload_info {
    tcp_channel		*channel;
    char		key[KEY_SIZE];
    char		path[1024];
    char		*dir_prefix;
    char		*dir_root;
    int			*exit_request;
} upload_info;

static void thread_upload(void *arg)
{
    upload_info *c = (upload_info *) arg;

    if (tcp_write(c->channel, c->key, KEY_SIZE) == KEY_SIZE) {
	process_page(c->channel, c->path, c->dir_prefix, c->dir_root, c->exit_request);
    } else
	fprintf(stderr, "tcp_write(c->key)\n");

    tcp_close(c->channel);
    free(c);
}

static void thread_httpd(void *arg)
{
    if (httpd_main((httpd_args *)arg)) {
	fprintf(stderr, "can't start http daemon\n");
    }
}

int client_connect(char *_host, int _port, char *_root_dir, int *_exit_request)
{
    httpd_args h_args;
    char dir_prefix[PATH_MAX];
    char *host;
    int port;
    int r;

    host = _host;
    port = _port;
    char *dir_root = _realpath(_root_dir, NULL);

    fprintf(stderr, "Shared directory: %s\n", dir_root);

#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif

    tcp_channel *server = tcp_open(TCP_SSL_CLIENT, host, port);
    if (!server) {
	fprintf(stderr, "tcp_open()\n");
	free(dir_root);
	return 1;
    }

    *_exit_request = 0;

    {
#if !defined(_WIN32) || defined(ENABLE_PTHREADS)
	pthread_t tid;
#endif
	char buf[BUF_SIZE];
	snprintf(buf, sizeof(buf), "VERSION: %f", VERSION);
	if ((r = tcp_write(server, buf, strlen(buf))) <= 0) {
	    fprintf(stderr, "tcp_write()\n");
	    goto exit1;
	}
	if ((r = tcp_read(server, buf, sizeof(buf))) <= 0) {
	    fprintf(stderr, "tcp_read()\n");
	    goto exit1;
	} else {
	    buf[r] = 0;
	    if (!strncmp(buf, "URL: ", 5)) {
		int i = 0;
		for (; (buf[i + 5] >= ' ') && (i < BUF_SIZE - 1); i++)
		    dir_prefix[i] = buf[i + 5];
		dir_prefix[i] = 0;
		fprintf(stderr, "Server directory: https://%s:%d%s\n", host, port - 100, dir_prefix);
#ifdef CLIENT_GUI
		snprintf(buf, sizeof(buf), "https://%s:%d%s", host, port - 100, dir_prefix);
		show_server_directory(buf);
#endif
	    } else if (!strncmp(buf, "UPD: ", 5)) {
		fprintf(stderr, "Update client to version %s or better.\n", buf + 5);
#ifdef CLIENT_GUI
		update_client(buf + 5);
#endif
		goto exit1;
	    } else {
		goto exit1;
	    }
	}

	h_args.port = 8000;
	h_args.root = dir_root;
	h_args.prefix = dir_prefix;
#if !defined(_WIN32) || defined(ENABLE_PTHREADS)
	if (pthread_create(&tid, NULL, (void *) &thread_httpd, (void *) &h_args) != 0) {
#else
	if (_beginthread(thread_httpd, 0, (VOID *) &h_args) == -1) {
#endif
	    fprintf(stderr, "pthread_create(thread_httpd)\n");
	}

	while (1) {
	    fd_set fds;
	    int res;
	    struct timeval tv = {0, 500000};
	    FD_ZERO (&fds);
	    FD_SET (tcp_fd(server), &fds);
	    res = select(tcp_fd(server) + 1, &fds, NULL, NULL, &tv);
	    if (res < 0) {
		fprintf(stderr, "%s select()\n", __FUNCTION__);
		break;
	    }

	    if (*_exit_request)
		break;

	    if (!res)
		continue;

	    if (!FD_ISSET(tcp_fd(server), &fds))
		continue;

	    if ((r = tcp_read(server, buf, sizeof(buf))) <= 0) {
		fprintf(stderr, "tcp_read()\n");
		break;
	    } else {
		buf[r] = 0;
		if (r < KEY_SIZE) {
#ifdef DEBUG
		    fprintf(stderr, "Recived [%s]\n", buf);
#endif
		    continue;
		}
#ifdef DEBUG
		fprintf(stderr, ">> %s\n", buf + KEY_SIZE);
		for (r = 0; r < 16; r++)
		    fprintf(stderr, "%02X ", (unsigned char)buf[r]);
		fprintf(stderr, "\n");
#endif
		upload_info *arg = malloc(sizeof(upload_info));
		memcpy(arg->key, buf, KEY_SIZE);
		arg->dir_root = dir_root;
		arg->dir_prefix = dir_prefix;
		arg->exit_request = _exit_request;
		arg->channel = tcp_open(TCP_SSL_CLIENT, host, port + 1);
		if (arg->channel) {
		    char *tmp = buf + KEY_SIZE;
		    if (!strncmp(tmp, "GET ", 4)) {
			int i = 0;
			for (; (tmp[i + 4] > ' ') && (i < BUF_SIZE - KEY_SIZE - 1); i++)
			    arg->path[i] = tmp[i + 4];
			arg->path[i] = 0;

#if !defined(_WIN32) || defined(ENABLE_PTHREADS)
			if (pthread_create(&tid, NULL, (void *) &thread_upload, (void *) arg) != 0) {
#else
			if (_beginthread(thread_upload, 0, (VOID *) arg) == -1) {
#endif
			    fprintf(stderr, "pthread_create(thread_upload)\n");
			}
			continue;
		    }
		}
		free(arg);
	    }
	}
    }

 exit1:
    fprintf(stderr, "Exit...\n");

    h_args.exit_request = 1;
#ifdef _WIN32
    Sleep(1000);
#else
    sleep(1);
#endif

    tcp_close(server);
    free(dir_root);

    return 0;
}

#ifndef CLIENT_GUI
int main(int argc, char *argv[])
{
    char *host;
    int port;
    char *dir_root;
    int exit_request = 0;

#if 0
    if (argc < 3) {
	fprintf(stderr, "%s <host> <port> <directory>\n", argv[0]);
	return 1;
    }

    host = argv[1];
    port = atoi(argv[2]);
    dir_root = argv[3];
#else
    if (argc < 2) {
	fprintf(stderr, "%s <directory>\n", argv[0]);
	return 1;
    }

    host = "getmyfil.es";
    port = 8100;
    dir_root = argv[1];
#endif

    return client_connect(host, port, dir_root, &exit_request);
}
#endif
