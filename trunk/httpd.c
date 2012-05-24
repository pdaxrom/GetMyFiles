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

#if 0
static char direct_icon[] = {
#include "direct-icon.h"
};
#endif

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
	memset(url, 0, sizeof(url));
	for (; (buf[i + 4] > ' ') && (i < BUF_SIZE - 1); i++)
	    url[i] = buf[i + 4];
	url[i] = 0;

	if ((!strncmp(url, "/js/getmyfiles.js?", 18)) &&
	    (!strncmp(url + 18, client->prefix, strlen(client->prefix)))) {
	    char *h_val = get_request_tag(buf, "Host:");
	    char buf[BUF_SIZE];
	    snprintf(buf, sizeof(buf), "P2P.go(\"%s\");", h_val);
	    char *resp = http_response_begin(200, "OK");
	    http_response_add_content_type(resp, get_mimetype("getmyfiles.js"));
	    http_response_add_content_length(resp, strlen(buf));
	    http_response_end(resp);
	    if (tcp_write(client->c, resp, strlen(resp)) == strlen(resp)) {
		tcp_write(client->c, buf, strlen(buf));
	    }
	    free(resp);
	    free(h_val);
	} else if (!strcmp(url, "/js/p2p.js")) {
	    char *resp = http_response_begin(200, "OK");
	    char buf[BUF_SIZE];
	    snprintf(buf, sizeof(buf), "%s", "function remove(id){ return (elem=document.getElementById(id)).parentNode.removeChild(elem); }; window.onload = function(){ remove(\"direct\"); };");
	    http_response_add_content_type(resp, get_mimetype("p2p.js"));
	    http_response_add_content_length(resp, strlen(buf));
	    http_response_end(resp);
	    if (tcp_write(client->c, resp, strlen(resp)) == strlen(resp)) {
		tcp_write(client->c, buf, strlen(buf));
	    }
	    free(resp);
#if 0
	} else if (!strcmp(url, "/pics/wifi.png")) {
	    char *resp = http_response_begin(200, "OK");
	    http_response_add_content_type(resp, get_mimetype("wifi.png"));
	    http_response_add_content_length(resp, sizeof(direct_icon));
	    http_response_end(resp);
	    if (tcp_write(client->c, resp, strlen(resp)) == strlen(resp)) {
		tcp_write(client->c, direct_icon, sizeof(direct_icon));
	    }
	    free(resp);
#endif
	} else {
	    process_page(client->c, url, buf, client->prefix, client->root, client->exit_request);
	}
    } else
	send_error(client->c, 501);

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
