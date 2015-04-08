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

#include "client.h"
#include "connctrl.h"

#define BUF_SIZE	1024
#define KEY_SIZE	16

typedef struct upload_info {
    tcp_channel		*channel;
    char		*request;
    char		key[KEY_SIZE];
    char		path[1024];
    char		*dir_prefix;
    char		*dir_root;
    int			*exit_request;
    int			httpd_port;
} upload_info;

static void thread_upload(void *arg)
{
    upload_info *c = (upload_info *) arg;

    if (tcp_write(c->channel, c->key, KEY_SIZE) == KEY_SIZE) {
	if (!conn_counter_limit(CONN_EXT)) {
	    conn_counter_inc(CONN_EXT);
	    process_page(c->channel, c->path, c->request, c->dir_prefix, c->dir_root, c->exit_request, c->httpd_port);
	    conn_counter_dec(CONN_EXT);
	} else
	    send_error(c->channel, 503);
    } else
	fprintf(stderr, "tcp_write(c->key)\n");

    free(c->request);
    tcp_close(c->channel);
    free(c);
}

static void thread_httpd(void *arg)
{
    if (httpd_main((httpd_args *)arg)) {
	fprintf(stderr, "can't start http daemon\n");
    }
}

int client_connect(client_args *client)
{
    httpd_args h_args;
    char dir_prefix[PATH_MAX];
    int r;

    char *dir_root = _realpath(client->root_dir, NULL);

    fprintf(stderr, "Shared directory: %s\n", dir_root);

#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif

    tcp_channel *server = tcp_open(TCP_SSL_CLIENT, client->host, client->port);
    if (!server) {
	fprintf(stderr, "tcp_open()\n");
	free(dir_root);
	return 1;
    }

    conn_counter_init(CONN_EXT, client->max_ext_conns);
    client->exit_request = 0;

    {
	int wdt_counter;
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
#ifdef PUBLIC_SERVICE
		snprintf(buf, sizeof(buf), "https://%s%s", client->host, dir_prefix);
#else
		snprintf(buf, sizeof(buf), "https://%s:%d%s", client->host, client->port - 100, dir_prefix);
#endif
#ifdef CLIENT_GUI
		show_server_directory(client, buf);
#else
		fprintf(stderr, "Server directory: %s\n", buf);
#endif
	    } else if (!strncmp(buf, "UPD: ", 5)) {
		fprintf(stderr, "Update client to version %s or better.\n", buf + 5);
#ifdef CLIENT_GUI
		update_client(client, buf + 5);
#endif
		goto exit1;
	    } else {
		goto exit1;
	    }
	}

	if (client->enable_httpd) {
	    h_args.port = 8000;
	    h_args.root = dir_root;
	    h_args.prefix = dir_prefix;
	    h_args.max_conns = client->max_int_conns;
#if !defined(_WIN32) || defined(ENABLE_PTHREADS)
	    if (pthread_create(&tid, NULL, (void *) &thread_httpd, (void *) &h_args) != 0) {
#else
	    if (_beginthread(thread_httpd, 0, (VOID *) &h_args) == -1) {
#endif
		fprintf(stderr, "pthread_create(thread_httpd)\n");
	    }
	} else
	    fprintf(stderr, "HTTPD disabled.\n");

	wdt_counter = 0;

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

	    if (client->exit_request)
		break;

	    // exit if no server activity more 31 sec.
	    if (wdt_counter++ > 62) {
		fprintf(stderr, "Watchdog timer...\n");
		break;
	    }

	    if (!res)
		continue;

	    if (!FD_ISSET(tcp_fd(server), &fds))
		continue;

	    wdt_counter = 0;

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
		arg->exit_request = &client->exit_request;
		arg->channel = tcp_open(TCP_SSL_CLIENT, client->host, client->port + 1);
		if (arg->channel) {
		    char *tmp = buf + KEY_SIZE;
		    if (!strncmp(tmp, "GET ", 4)) {
			int i = 0;
			for (; (tmp[i + 4] > ' ') && (i < BUF_SIZE - KEY_SIZE - 1); i++)
			    arg->path[i] = tmp[i + 4];
			arg->path[i] = 0;
			arg->request = strdup(tmp);
			arg->httpd_port = h_args.port;

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

    conn_counter_fini(CONN_EXT);

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
    client_args client;
    client.max_ext_conns = 2;
    client.max_int_conns = 2;
    client.exit_request = 0;
    client.enable_httpd = 1;
    client.priv = NULL;

#if 0
    if (argc < 3) {
	fprintf(stderr, "%s <host> <port> <directory>\n", argv[0]);
	return 1;
    }

    client.host = argv[1];
    client.port = atoi(argv[2]);
    client.root_dir = argv[3];
#else
    if (argc < 2) {
	fprintf(stderr, "%s <directory>\n", argv[0]);
	return 1;
    }

    client.host = "getmyfil.es";
    client.port = 8100;
    client.root_dir = argv[1];
#endif

    return client_connect(&client);
}
#endif
