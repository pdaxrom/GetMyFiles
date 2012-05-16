#include <stdio.h>
#include <openssl/ssl.h>
#include "http.h"

#define BUF_SIZE 1024

static const char *tmpl_404 = "<html><head><title>404 Not Found</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" /></head><body><b>Oops!</b><br /><br />The requested URL %s was not found :(</body></html>";

int send_header(SSL *ssl, char *mime, int use_len, size_t len)
{
    char buf[BUF_SIZE];

    if (use_len)
	snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\nContent-Type: %s\nContent-Length: %ld\n\n", mime, len);
    else
	snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\nContent-Type: %s\n\n", mime);

    if (SSL_write(ssl, buf, strlen(buf)) != strlen(buf))
	return -1;
    return 0;
}

int send_data(SSL *ssl, char *mime, char *data, size_t len)
{
    if (send_header(ssl, mime, 1, len))
	return -1;

    if (SSL_write(ssl, data, len) != len)
	return -1;

    return 0;
}

int send_404(SSL *ssl, char *url)
{
    char buf[BUF_SIZE];
    snprintf(buf, sizeof(buf), tmpl_404, url);
#ifdef SOFT_404
    return send_data(ssl, "text/html", buf, strlen(buf));
#else
    char *resp = "HTTP/1.1 404 Not Found\nContent-Type: text/html; charset=UTF-8\nConnection: close\n\n";

    if (SSL_write(ssl, resp, strlen(resp)) != strlen(resp))
	return -1;

    if (SSL_write(ssl, buf, strlen(buf)) != strlen(buf))
	return -1;

    return 0;
#endif
}
