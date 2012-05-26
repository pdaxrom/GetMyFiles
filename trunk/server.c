#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#ifndef _WIN32
#include <sys/select.h>
#else
#include <windows.h>
#endif
#include <pthread.h>
#include <signal.h>
#include <limits.h>

#include "tcp.h"
#include "utils.h"
#include "http.h"

#define HTTP_TIMEOUT	3

#define BUF_SIZE	1024
#define KEY_SIZE	16

#ifndef WWWROOT
#define WWWROOT		"/var/lib/getmyfiles/htdocs"
#endif

static char *dir_root = WWWROOT;

typedef struct http_info {
    tcp_channel	*channel;
    char	key[KEY_SIZE];
    int		inactive;
    struct http_info *next;
} http_info;

typedef struct user_info {
    tcp_channel *channel;
    char	*name;
    int		in_use;
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

static void user_add(tcp_channel *client, char *name)
{
    user_info *user = malloc(sizeof(user_info));
    user->channel = client;
    user->name = name;
    user->in_use = 0;
    user->next = NULL;

    if (first_user == NULL) {
	first_user = user;
    } else {
	user_info *tmp = first_user;
	while (tmp->next) {
	    tmp = tmp->next;
	}
	tmp->next = user;
    }
}

static void user_info_free(user_info *info)
{
    tcp_close(info->channel);
    free(info->name);
    free(info);
}

static void user_del(user_info *info)
{
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
}

static void users_dump(void)
{
    int total = 0;
    user_info *tmp = first_user;
    fprintf(stderr, "-------------------------\n");
    while (tmp) {
	fprintf(stderr, "Client: %s\n", tmp->name);
	tmp = tmp->next;
	total++;
    }
    fprintf(stderr, "-------------------------\nTotal: %d\n\n", total);
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
    http_info *ret = NULL;
    pthread_mutex_lock(&mutex_https);
    http_info *tmp = first_http;
    while (tmp) {
	if (!memcmp(tmp->key, key, KEY_SIZE)) {
	    ret = tmp;
	    ret->inactive = 0; // disable watchdog for this connection
	    break;
	}
	tmp = tmp->next;
    }
    pthread_mutex_unlock(&mutex_https);
    return ret;
}

static void thread_user(void *arg)
{
    char buf[BUF_SIZE];
    tcp_channel *server = (tcp_channel *)arg;

    while (1) {
	int r;
	tcp_channel *client = tcp_accept(server);
	if (!client) {
	    fprintf(stderr, "%s tcp_accept()\n", __FUNCTION__);
	    continue;
	} else {
	    if ((r = tcp_read(client, buf, sizeof(buf))) > 0) {
		buf[r] = 0;
		float serv_vers = VERSION;
		float vers = 0;
		sscanf(buf, "VERSION: %f", &vers);
		dprintf("Client version: %f (server %f)\n", vers, serv_vers);
		if (vers < serv_vers) {
		    fprintf(stderr, "Reject client version: %f (need %f or better)\n", vers, serv_vers);
		    snprintf(buf, sizeof(buf), "UPD: %.2f", serv_vers);
		    tcp_write(client, buf, strlen(buf));
		} else {
		    char *name = tempnam("/","share");
		    strcpy(name, name + 1);
		    dprintf("New client: [%s]\n", name);
		    sprintf(buf, "URL: %s", name);
		    if (tcp_write(client, buf, strlen(buf)) == strlen(buf)) {
			pthread_mutex_lock(&mutex_users);
			user_add(client, name);
			users_dump();
			pthread_mutex_unlock(&mutex_users);
			continue;
		    } else {
			fprintf(stderr, "%s tcp_write()\n", __FUNCTION__);
		    }
		}
	    }
	}
	tcp_close(client);
    }
}

static void thread_http_request(void *arg)
{
    char buffer[BUF_SIZE + KEY_SIZE];
    int r;
    http_info *c = (http_info *) arg;
    char *buf = buffer + KEY_SIZE;

    if ((r = tcp_read(c->channel, buf, BUF_SIZE)) > 0) {
	if (r < 2)
	    r = tcp_read(c->channel, buf + r, BUF_SIZE - r);
	buf[r] = 0;
	dprintf("buf[%d]=%s\n", r, buf);
	if (!strncmp(buf, "GET ", 4)) {
	    char *ptr = strchr(buf + 4, ' ');
	    if (ptr) {
		user_info *info;
		dprintf("path: [%s] %d\n", buf + 4, tcp_fd(c->channel));
		pthread_mutex_lock(&mutex_users);
		if ((info = user_get(buf + 4))) {
		    info->in_use++;
		    pthread_mutex_unlock(&mutex_users);
		    memcpy(buffer, c->key, KEY_SIZE);
		    http_add(c);
		    if (tcp_write(info->channel, buffer, strlen(buf) + KEY_SIZE) != strlen(buf) + KEY_SIZE) {
			fprintf(stderr, "%s tcp_write()\n", __FUNCTION__);
			send_error(c->channel, 502);
			http_del(c);
			pthread_mutex_lock(&mutex_users);
			info->in_use--;
			if (!info->in_use) {
			    user_del(info);
			    users_dump();
			}
			pthread_mutex_unlock(&mutex_users);
			return;
		    } else {
			pthread_mutex_lock(&mutex_users);
			info->in_use--;
			pthread_mutex_unlock(&mutex_users);
			return;
		    }
		} else {
		    char path[PATH_MAX];
		    pthread_mutex_unlock(&mutex_users);
		    char *ptr = strchr(buf + 4, ' ');
		    if (ptr)
			*ptr = 0;
		    remove_slashes(buf + 4);
		    snprintf(path, sizeof(path), "%s/%s", dir_root, buf + 4);
		    dprintf("%s path [%s]\n", __FUNCTION__, path);
		    char *normal_path = _realpath(path, NULL);
		    dprintf("%s normal_path [%s]\n", __FUNCTION__, normal_path);
		    if (!normal_path)
			send_404(c->channel, buf + 4);
		    else if (!strncmp(normal_path, dir_root, strlen(dir_root))) {
			struct stat sb;
			if ((stat(normal_path, &sb) != -1) &&
			    ((sb.st_mode & S_IFMT) != S_IFDIR)) {
			    FILE *f = fopen(normal_path, "rb");
			    if (f) {
				int r;
				char *resp = http_response_begin(200, "OK");
				http_response_add_content_type(resp, get_mimetype(normal_path));
				http_response_add_content_length(resp, sb.st_size);
				http_response_end(resp);
				if (tcp_write(c->channel, resp, strlen(resp)) == strlen(resp)) {
				    while ((r = fread(buf, 1, BUF_SIZE, f)) > 0) {
					if (tcp_write(c->channel, buf, r) != r)
					    break;
				    }
				    fclose(f);
				} else
				    fprintf(stderr, "tcp_write(resp)\n");
				free(resp);
			    } else
				send_404(c->channel, buf + 4);
			} else
			    send_404(c->channel, buf + 4);
		    } else
			send_404(c->channel, buf + 4);
		    free(normal_path);
		}
	    }
	} else
	    send_error(c->channel, 501);
    } else
	fprintf(stderr, "%s tcp_read() %d\n", __FUNCTION__, r);

    tcp_close(c->channel);
    free(c);
}

static void thread_redirector(void *arg)
{
    int r;
    char buf[BUF_SIZE];
    tcp_channel *client = (tcp_channel *) arg;

    if ((r = tcp_read(client, buf, KEY_SIZE)) == KEY_SIZE) {
	http_info *info = http_get(buf);
	if (info) {
#ifdef DEBUG
	    fprintf(stderr, "http found\n");
#endif
	    while ((r = tcp_read(client, buf, sizeof(buf))) > 0) {
		if (tcp_write(info->channel, buf, r) != r) {
		    fprintf(stderr, "%s tcp_write(info->channel)\n", __FUNCTION__);
		    break;
		}
	    }
	    http_del(info);
	}
    } else
	fprintf(stderr, "%s tcp_read(key)\n", __FUNCTION__);

    tcp_close(client);
}

static void thread_data(void *arg)
{
    tcp_channel *server = (tcp_channel *)arg;

    while (1) {
	pthread_t tid;
	tcp_channel *client = tcp_accept(server);
	if (!client) {
	    fprintf(stderr, "%s tcp_accept()\n", __FUNCTION__);
	    continue;
	}
	if (pthread_create(&tid, NULL, (void *) &thread_redirector, (void *) client) != 0) {
	    fprintf(stderr, "%s pthread_create(thread_http)\n", __FUNCTION__);
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

//
// Remove inactive clients
//
static void thread_user_ping(void *argc)
{
    while (1) {
	int users = 0;
	user_info *tmp = first_user;
	user_info *prv;

	pthread_mutex_lock(&mutex_users);

	while (tmp) {
	    if (!tmp->in_use) {
		static char *ping_str = "PING :-O";
		if (tcp_write(tmp->channel, ping_str, strlen(ping_str)) != strlen(ping_str)) {
		    user_info *tmp1 = tmp;
		    if (tmp == first_user) {
			first_user = first_user->next;
			tmp = first_user;
		    } else {
			prv->next = tmp->next;
			tmp = tmp->next;
		    }
		    user_info_free(tmp1);
		    users++;
		    continue;
		}
	    }
	    prv = tmp;
	    tmp = tmp->next;
	}

	pthread_mutex_unlock(&mutex_users);

	if (users)
	    fprintf(stderr, "Dead clients removed: %d\n", users);

	sleep(10);
    }
}

//
// Remove inactive http connections after timeout
//
static void thread_http_cleanup(void *argc)
{
    while (1) {
	int https = 0;
	http_info *tmp = first_http;
	http_info *prv;

	pthread_mutex_lock(&mutex_https);

	while (tmp) {
	    if (tmp->inactive)
		tmp->inactive++;
	    if (tmp->inactive > HTTP_TIMEOUT) {
		http_info *tmp1 = tmp;
		if (tmp == first_http) {
		    first_http = first_http->next;
		    tmp = first_http;
		} else {
		    prv->next = tmp->next;
		    tmp = tmp->next;
		}
		
		send_error(tmp1->channel, 504);
		
		http_info_free(tmp1);
		https++;
		continue;
	    }
	    prv = tmp;
	    tmp = tmp->next;
	}

	pthread_mutex_unlock(&mutex_https);

	if (https)
	    fprintf(stderr, "Dead http requests removed: %d\n", https);

	sleep(5);
    }
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
    fprintf(stderr, "HTDOCS    %s\n", dir_root);

    signal(SIGPIPE, SIG_IGN);

    tcp_channel *http = tcp_open(TCP_SSL_SERVER, NULL, http_port);
    if (!http)
	panic("tcp_open(http_port)");

    tcp_channel *user = tcp_open(TCP_SSL_SERVER, NULL, user_port);
    if (!user)
	panic("tcp_open(user_port)");

    tcp_channel *data = tcp_open(TCP_SSL_SERVER, NULL, data_port);
    if (!user)
	panic("tcp_open(data_port)");

    pthread_mutex_init(&mutex_users, NULL);
    pthread_mutex_init(&mutex_https, NULL);

    if (pthread_create(&tid, NULL, (void *) &thread_user, (void *) user) != 0)
	panic("pthread_create(thread_user)");

    if (pthread_create(&tid, NULL, (void *) &thread_data, (void *) data) != 0)
	panic("pthread_create(thread_data)");

    if (pthread_create(&tid, NULL, (void *) &thread_user_ping, NULL) != 0)
	panic("pthread_create(thread_http_cleanup)");

    if (pthread_create(&tid, NULL, (void *) &thread_http_cleanup, NULL) != 0)
	panic("pthread_create(thread_http_cleanup)");

    while (1) {
	tcp_channel *client = tcp_accept(http);
	if (!client) {
	    fprintf(stderr, "%s tcp_accept()\n", __FUNCTION__);
	    continue;
	}

	dprintf("%s tcp_accept ok\n", __FUNCTION__);

	{
	    http_info *arg = malloc(sizeof(http_info));
	    arg->channel = client;
	    arg->inactive = 1;
	    generate_key(arg->key);
	    arg->next = NULL;

	    if (pthread_create(&tid, NULL, (void *) &thread_http_request, (void *) arg) != 0) {
		fprintf(stderr, "%s pthread_create(thread_http)\n", __FUNCTION__);
		tcp_close(client);
	    }
	}

    }

    tcp_close(http);
    tcp_close(user);
    tcp_close(data);

    pthread_mutex_destroy(&mutex_users);
    pthread_mutex_destroy(&mutex_https);

    return 0;
}
