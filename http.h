#ifndef _HTTP_H_
#define _HTTP_H_

#include "tcp.h"

char *http_response_begin(int status, char *reason);
char *http_response_add_content_type(char *resp, char *mime);
char *http_response_add_content_length(char *resp, size_t length);
char *http_response_add_connection(char *resp, char *token);
char *http_response_end(char *resp);

int send_404(tcp_channel *c, char *url);
int send_502(tcp_channel *c);
int send_504(tcp_channel *c);
int process_dir(tcp_channel *c, char *url, char *path, int is_root, int *exit_request);
int process_page(tcp_channel *channel, char *url, char *dir_prefix, char *dir_root, int *exit_request);

#endif
