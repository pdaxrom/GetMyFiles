#ifndef _HTTP_H_
#define _HTTP_H_

#include "tcp.h"

char *http_response_begin(int status, char *reason);
char *http_response_add_content_type(char *resp, char *mime);
char *http_response_add_content_length(char *resp, size_t length);
char *http_response_add_connection(char *resp, char *token);
char *http_response_add_accept_ranges(char *resp);
char *http_response_add_range(char *resp, size_t from, size_t to, size_t length);
char *http_response_end(char *resp);

char *get_request_tag(char *header, char *tag);

int send_404(tcp_channel *c, char *url);
int send_error(tcp_channel *c, int error);
int process_dir(tcp_channel *c, char *url, char *http_request, char *path, int is_root, int *exit_request, int dircon_port);
int process_page(tcp_channel *channel, char *url, char *http_request, char *dir_prefix, char *dir_root, int *exit_request, int dircon_port);

#endif
