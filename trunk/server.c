#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifndef _WIN32
#include <sys/select.h>
#else
#include <windows.h>
#endif
#include <openssl/ssl.h>
#include <pthread.h>
#include <signal.h>

#include "tcp.h"

#define BUF_SIZE	1024
#define KEY_SIZE	16

static const char *SSL_CIPHER_LIST = "ALL:!LOW";

static const char *oops_reply = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><head><title>Oops!</title></head><body><b>This link doesn't exists!</b></body></html>";

static SSL_CTX *ctx_http;
static SSL_CTX *ctx_user;
static SSL_CTX *ctx_data;

typedef struct http_info {
    tcp_channel	*channel;
    SSL		*ssl;
    char	key[KEY_SIZE];
    struct http_info *next;
} http_info;

typedef struct user_info {
    tcp_channel *channel;
    SSL		*ssl;
    char	*name;
    struct user_info *next;
} user_info;

static user_info *first_user = NULL;
static http_info *first_http = NULL;

static pthread_mutex_t mutex_users;
static pthread_mutex_t mutex_https;

static void panic(char *str)
{
    fprintf(stderr, "PANIC:   %s\n", str);
    exit(1);
}

static void warning(char *str)
{
    fprintf(stderr, "WARNING: %s\n", str);
}

static RSA *ssl_genkey(SSL *ssl_connection, int export, int key_length)
{
    static RSA *key = NULL;

    if (key == NULL) {
	if ((key = RSA_generate_key(export ? key_length : 1024, RSA_F4, NULL, NULL)) == NULL)
	    panic("Failed to generate temporary key.");
    }

    return key;
}

static SSL_CTX *ssl_initialize(void)
{
    SSL_CTX *ssl_context;

    static char cert_path[FILENAME_MAX + 1];

    /* -1. check if certificate exists */
    sprintf(cert_path, CONFIG_DIR "/server.pem");

    /* 0. initialize library */
    SSL_library_init();
    SSL_load_error_strings();

    /* 1. initialize context */
    if ((ssl_context = SSL_CTX_new(SSLv3_method())) == NULL)
	panic("Failed to initialize SSL context.");

    SSL_CTX_set_options(ssl_context, SSL_OP_ALL);

    if (!SSL_CTX_set_cipher_list(ssl_context, SSL_CIPHER_LIST))
	panic("Failed to set SSL cipher list.");

    /* 2. load certificates */
    if (!SSL_CTX_use_certificate_chain_file(ssl_context, cert_path))
	panic("Failed to load certificate.");

    if (!SSL_CTX_use_RSAPrivateKey_file(ssl_context, cert_path, SSL_FILETYPE_PEM))
	panic("Failed to load private key.");

    if (SSL_CTX_need_tmp_RSA(ssl_context))
	SSL_CTX_set_tmp_rsa_callback(ssl_context, ssl_genkey);

    return ssl_context;
}

static void ssl_tear_down(SSL_CTX *ctx)
{
	SSL_CTX_free(ctx);
}

static void user_add(tcp_channel *client, SSL *ssl, char *name)
{
    user_info *user = malloc(sizeof(user_info));
    user->channel = client;
    user->ssl = ssl;
    user->name = name;
    user->next = NULL;

    pthread_mutex_lock(&mutex_users);

    if (first_user == NULL) {
	first_user = user;
    } else {
	user_info *tmp = first_user;
	while (tmp->next) {
	    tmp = tmp->next;
	}
	tmp->next = user;
    }

    pthread_mutex_unlock(&mutex_users);
}

static void user_info_free(user_info *info)
{
    SSL_shutdown(info->ssl);
    SSL_free(info->ssl);
    tcp_close(info->channel);
    free(info->name);
    free(info);
}

static void user_del(user_info *info)
{
    pthread_mutex_lock(&mutex_users);

    user_info *tmp = first_user;
    user_info *prv;
    while (tmp) {
	if (tmp == info) {
	    if (tmp == first_user) {
		first_user = first_user->next;
	    } else {
		prv->next = tmp->next;
	    }
	    user_info_free(info);
	    break;
	}
	prv = tmp;
	tmp = tmp->next;
    }

    pthread_mutex_unlock(&mutex_users);
}

static void users_dump(void)
{
    user_info *tmp = first_user;
    fprintf(stderr, "-------------------------\n");
    while (tmp) {
	fprintf(stderr, "Client: %s\n", tmp->name);
	tmp = tmp->next;
    }
    fprintf(stderr, "-------------------------\n\n");
}

static user_info *user_get(char *name)
{
    char buf[BUF_SIZE];
    user_info *tmp = first_user;
    char *ptr = buf;
    int i = 0;
    while (name[i] > ' ') {
	if (!((name[i] == '/') && (name[i + 1] == '/'))) {
	    *ptr++ = name[i];
	}
	i++;
    }
    *ptr = 0;

    while (tmp) {
	if (!strncmp(tmp->name, buf, strlen(tmp->name)))
	    return tmp;
	tmp = tmp->next;
    }
    return NULL;
}

static void http_add(http_info *info)
{
    pthread_mutex_lock(&mutex_https);

    if (first_http == NULL) {
	first_http = info;
    } else {
	http_info *tmp = first_http;
	while (tmp->next) {
	    tmp = tmp->next;
	}
	tmp->next = info;
    }

    pthread_mutex_unlock(&mutex_https);
}

static void http_info_free(http_info *info)
{
    SSL_shutdown(info->ssl);
    SSL_free(info->ssl);
    tcp_close(info->channel);
    free(info);
}

static void http_del(http_info *info)
{
    pthread_mutex_lock(&mutex_https);

    http_info *tmp = first_http;
    http_info *prv;
    while (tmp) {
	if (tmp == info) {
	    if (tmp == first_http) {
		first_http = first_http->next;
	    } else {
		prv->next = tmp->next;
	    }
	    http_info_free(info);
	    break;
	}
	prv = tmp;
	tmp = tmp->next;
    }

    pthread_mutex_unlock(&mutex_https);
}

static http_info *http_get(char *key)
{
    http_info *tmp = first_http;
    while (tmp) {
	if (!memcmp(tmp->key, key, KEY_SIZE))
	    return tmp;
	tmp = tmp->next;
    }
    return NULL;
}

static void thread_user(void *arg)
{
    char buf[BUF_SIZE];
    tcp_channel *server = (tcp_channel *)arg;

    while (1) {
	int r;
	SSL *ssl;
	tcp_channel *client = tcp_accept(server);
	if (!client) {
	    fprintf(stderr, "tcp_accept()\n");
	    continue;
	}

	if ((ssl = SSL_new(ctx_user)) == NULL)
		panic("Failed to create SSL connection.");

	SSL_set_fd(ssl, tcp_fd(client));

	if (SSL_accept(ssl) < 0)
		warning("Unable to accept SSL connection.");
	else {
	    if ((r = SSL_read(ssl, (uint8_t *)buf, BUF_SIZE)) > 0) {
		buf[r] = 0;
		//fprintf(stderr, "client:[%s] to ", buf);
		char *name = tempnam("/","share");
		strcpy(name, name + 1);
		fprintf(stderr, "New client: [%s]\n", name);
		sprintf(buf, "URL: %s", name);
		SSL_write(ssl, buf, strlen(buf));
		user_add(client, ssl, name);
		users_dump();
	    } else {
		SSL_shutdown(ssl);
		SSL_free(ssl);
		tcp_close(client);
	    }
	}
    }
}

static void thread_http_request(void *arg)
{
    char buffer[BUF_SIZE + KEY_SIZE];
    int r;
    http_info *c = (http_info *) arg;
    char *buf = buffer + KEY_SIZE;

    if ((r = SSL_read(c->ssl, (uint8_t *)buf, BUF_SIZE)) > 0) {
	if (r < 2)
	    r = SSL_read(c->ssl, (uint8_t *)buf + r, BUF_SIZE - r);
	buf[r] = 0;
#ifdef DEBUG
	fprintf(stderr, "buf[%d]=%s\n", r, buf);
#endif
	if (!strncmp(buf, "GET ", 4)) {
	    char *ptr = strchr(buf + 4, ' ');
	    if (ptr) {
		user_info *info;
#ifdef DEBUG
		fprintf(stderr, "path: [%s] %d\n", buf + 4, tcp_fd(c->channel));
#endif
		if ((info = user_get(buf + 4))) {
		    memcpy(buffer, c->key, KEY_SIZE);
//		    fprintf(stderr, "OKAY!\n");
		    http_add(c);
		    if (SSL_write(info->ssl, buffer, strlen(buf) + KEY_SIZE) <= 0) {
			warning("SSL_write()");
			SSL_write(c->ssl, oops_reply, strlen(oops_reply));
			http_del(c);
			user_del(info);
			users_dump();
			return;
		    } else {
//			fprintf(stderr, "OKAY2\n");
			return;
		    }
		} else {
		    SSL_write(c->ssl, oops_reply, strlen(oops_reply));
		}
	    }
	} else
	    warning("Unknown request!");
    } else
	fprintf(stderr, "SSL_read() %d\n", r);

    SSL_shutdown(c->ssl);
    SSL_free(c->ssl);
    tcp_close(c->channel);
    free(c);
}

static void thread_redirector(void *arg)
{
    int r;
    SSL *ssl;
    char buf[BUF_SIZE];
    tcp_channel *client = (tcp_channel *) arg;

    if ((ssl = SSL_new(ctx_user)) == NULL) {
	warning("Failed to create SSL connection.");
	tcp_close(client);
	return;
    }

    SSL_set_fd(ssl, tcp_fd(client));

    if (SSL_accept(ssl) < 0)
	warning("Unable to accept SSL connection.");
    else {
	if ((r = SSL_read(ssl, (uint8_t *)buf, KEY_SIZE)) == KEY_SIZE) {
	    http_info *info = http_get(buf);
	    if (info) {
#ifdef DEBUG
		warning("http found");
#endif
		while ((r = SSL_read(ssl, (uint8_t *) buf, BUF_SIZE)) > 0) {
		    if (SSL_write(info->ssl, buf, r) < 0) {
			warning("SSL_write(info->ssl)");
			break;
		    }
		}
		http_del(info);
	    }
	} else
	    warning("SSL_read(key)");
    }
    SSL_shutdown(ssl);
    SSL_free(ssl);
    tcp_close(client);
}

static void thread_data(void *arg)
{
    tcp_channel *server = (tcp_channel *)arg;

    while (1) {
	pthread_t tid;
	tcp_channel *client = tcp_accept(server);
	if (!client) {
	    fprintf(stderr, "tcp_accept()\n");
	    continue;
	}
	if (pthread_create(&tid, NULL, (void *) &thread_redirector, (void *) client) != 0) {
	    warning("pthread_create(thread_http)");
	    tcp_close(client);
	}
    }
}

static void generate_key(char *key)
{
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f)
	panic("fopen(/dev/urandom)");

    fread(key, 1, KEY_SIZE, f);

    fclose(f);
}

int main(int argc, char *argv[])
{
    int http_port;
    int user_port;
    int data_port;

    pthread_t tid;

    if (argc < 3) {
	fprintf(stderr, "%s <http port> <user port>\n", argv[0]);
	return -1;
    }

    http_port = atoi(argv[1]);
    user_port = atoi(argv[2]);
    data_port = user_port + 1;

    fprintf(stderr, "HTTP port %d\n", http_port);
    fprintf(stderr, "USER port %d\n", user_port);
    fprintf(stderr, "DATA port %d\n", data_port);

    signal(SIGPIPE, SIG_IGN);

    tcp_channel *http = tcp_open(TCP_SERVER, NULL, http_port);
    if (!http)
	panic("tcp_open(http_port)");

    tcp_channel *user = tcp_open(TCP_SERVER, NULL, user_port);
    if (!user)
	panic("tcp_open(user_port)");

    tcp_channel *data = tcp_open(TCP_SERVER, NULL, data_port);
    if (!user)
	panic("tcp_open(data_port)");

    ctx_http = ssl_initialize();
    ctx_user = ssl_initialize();
    ctx_data = ssl_initialize();

    pthread_mutex_init(&mutex_users, NULL);
    pthread_mutex_init(&mutex_https, NULL);

    if (pthread_create(&tid, NULL, (void *) &thread_user, (void *) user) != 0)
	panic("pthread_create(thread_user)");

    if (pthread_create(&tid, NULL, (void *) &thread_data, (void *) data) != 0)
	panic("pthread_create(thread_data)");

    while (1) {
	SSL *ssl;
	tcp_channel *client = tcp_accept(http);
	if (!client) {
	    fprintf(stderr, "tcp_accept()\n");
	    continue;
	}

	if ((ssl = SSL_new(ctx_http)) == NULL)
		panic("Failed to create SSL connection.");

	SSL_set_fd(ssl, tcp_fd(client));

	if (SSL_accept(ssl) < 0)
		warning("Unable to accept SSL connection.");
	else {
	    http_info *arg = malloc(sizeof(http_info));
	    arg->channel = client;
	    arg->ssl = ssl;
	    generate_key(arg->key);
	    arg->next = NULL;

	    if (pthread_create(&tid, NULL, (void *) &thread_http_request, (void *) arg) != 0) {
		warning("pthread_create(thread_http)");
		SSL_shutdown(ssl);
		SSL_free(ssl);
		tcp_close(client);
	    }
	}

    }

    tcp_close(http);
    tcp_close(user);
    tcp_close(data);

    ssl_tear_down(ctx_user);
    ssl_tear_down(ctx_http);
    ssl_tear_down(ctx_data);

    pthread_mutex_destroy(&mutex_users);
    pthread_mutex_destroy(&mutex_https);

    return 0;
}
