#ifndef _HTTP_H_
#define _HTTP_H_

int send_header(SSL *ssl, char *mime, int use_len, size_t len);
int send_data(SSL *ssl, char *mime, char *data, size_t len);
int send_404(SSL *ssl, char *url);

#endif
