#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "tcp.h"
#include "http.h"
#include "httpd.h"
#include "utils.h"

#define BUF_SIZE	1024

typedef struct {
    tcp_channel *c;
    char *root;
    char *prefix;
    int *exit_request;
} client_info;

char *get_header_tag(char *header, char *tag)
{
    char *ptr = header;

    do {
	if (!strncmp(ptr, tag, strlen(tag))) {
	    int i = 0;
	    ptr += strlen(tag) + 1;
	    while (ptr[i++] >= ' ');
	    char *ret = malloc(i);
	    memcpy(ret, ptr, i);
	    ret[i - 1] = 0;
	    return ret;
	} else
	    while (*ptr++ > ' ');
	while (*ptr > 0 && *ptr < ' ')
	    ptr++;
    } while(*ptr != 0);

    return NULL;
}

static void thread_client(void *arg)
{
    char buf[BUF_SIZE];
    client_info *client = (client_info *) arg;

    int len = tcp_read(client->c, buf, sizeof(buf));
    if (len > 0) {
	buf[len] = 0;
//	fprintf(stderr, "local httpd [%s]\n", buf);
    }

    if (!strncmp(buf, "GET ", 4)) {
	int i = 0;
	char url[BUF_SIZE];
	for (; (buf[i + 4] > ' ') && (i < BUF_SIZE - 1); i++)
	    url[i] = buf[i + 4];
	url[i] = 0;

	if (!strcmp(url, "/js/getmyfiles.js")) {
	    char *h_val = get_header_tag(buf, "Host:");
	    char buf[BUF_SIZE];
	    snprintf(buf, sizeof(buf), "P2P.go(\"%s\");", h_val);
	    char *resp = http_response_begin(200, "OK");
	    http_response_add_content_type(resp, get_mimetype(url));
	    http_response_add_content_length(resp, strlen(buf));
	    http_response_end(resp);
	    if (tcp_write(client->c, resp, strlen(resp)) == strlen(resp)) {
		tcp_write(client->c, buf, strlen(buf));
	    }
	    free(resp);
	    free(h_val);
	} else {
	    process_page(client->c, url, client->prefix, client->root, client->exit_request);
	}
    }

    tcp_close(client->c);
    free(client);
}

int httpd_main(httpd_args *args)
{
    tcp_channel *server = tcp_open(TCP_SERVER, "localhost", args->port);
    if (!server) {
	fprintf(stderr, "%s tcp_open()\n", __FUNCTION__);
	return 1;
    }

    args->exit_request = 0;

    fprintf(stderr, "Starting local httpd http://localhost:%d%s -> %s\n", args->port, args->prefix, args->root);

    while (!args->exit_request){
#if !defined(_WIN32) || defined(ENABLE_PTHREADS)
	pthread_t tid;
#endif
	client_info *info;

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

	if (args->exit_request)
	    break;

	if (!res)
	    continue;

	if (!FD_ISSET(tcp_fd(server), &fds))
	    continue;

	tcp_channel *client = tcp_accept(server);
	if (!client)
	    break;
	info = malloc(sizeof(client_info));
	info->c = client;
	info->root = args->root;
	info->prefix = args->prefix;
	info->exit_request = &args->exit_request;
#if !defined(_WIN32) || defined(ENABLE_PTHREADS)
	if (pthread_create(&tid, NULL, (void *) &thread_client, (void *) info) != 0) {
#else
	if (_beginthread(thread_client, 0, (VOID *) info) == -1) {
#endif
	    free(info);
	    fprintf(stderr, "pthread_create(thread_upload)\n");
	    break;
	}
    }

    tcp_close(server);

    return 0;
}