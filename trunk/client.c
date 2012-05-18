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
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <limits.h>
#include <signal.h>

#include "tcp.h"
#include "urldecode.h"
#include "utils.h"
#include "http.h"

#ifdef CLIENT_GUI
#include <client.h>
#endif

#ifdef __MINGW32__
void *alloca(size_t);
#endif

#define BUF_SIZE	1024
#define KEY_SIZE	16

static SSL_CTX *ctx;
static SSL_CTX *ctx_data;

static const char *tmpl_dir_header =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">"
"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">"
"<head>"
"<title>Index of %s</title>"
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />"
"<script type=\"text/javascript\" src=\"/js/imageviewer.js\"></script>"
"<script type=\"text/javascript\" src=\"/js/player5.js\"></script>"
"<style type=\"text/css\">"
"a, a:active {text-decoration: none; color: blue;}"
"a:visited {color: #48468F;}"
"a:hover, a:focus {text-decoration: underline; color: red;}"
"body {background-color: #F5F5F5;}"
"h2 {margin-bottom: 12px;}"
"table {margin-left: 12px;}"
"th, td { font: 90%% monospace; text-align: left;}"
"th { font-weight: bold; padding-right: 14px; padding-bottom: 3px;}"
"td {padding-right: 14px;}"
"td.s, th.s {text-align: right;}"
"div.list { background-color: white; border-top: 1px solid #646464; border-bottom: 1px solid #646464; padding-top: 10px; padding-bottom: 14px;}"
"div.foot { font: 90%% monospace; color: #787878; padding-top: 4px;}"
"</style>"
"</head>"
"<body>"
"<h2>Index of %s</h2>"
"<div class=\"list\">"
"<table summary=\"Directory Listing\" cellpadding=\"0\" cellspacing=\"0\">"
"<thead><tr><th class=\"n\">Name</th><th class=\"m\">Last Modified</th><th class=\"s\">Size</th><th class=\"t\">Type</th></tr></thead>"
"<tbody>";

static const char *tmpl_dir_footer =
"</tbody>"
"</table>"
"</div>"
"<div class=\"foot\">Powered by <a href=\"http://getmyfil.es\">getmyfil.es</a></div>"
"</body>"
"</html>";

typedef struct upload_info {
    tcp_channel		*channel;
    SSL			*ssl;
    char		key[KEY_SIZE];
    char		path[1024];
} upload_info;

static char *dir_root;
static char dir_prefix[512];

static void warning(char *str)
{
    fprintf(stderr, "WARNING: %s\n", str);
}

static SSL_CTX *ssl_initialize(void)
{
    SSL_CTX *ctx;
    /* Set up the library */
    SSL_library_init();
    ERR_load_BIO_strings();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    SSL_METHOD *meth;
    meth = SSLv3_client_method();
    ctx = SSL_CTX_new(meth);

    if (!ctx) {
	ERR_print_errors_fp(stderr);
	exit(1);
    }

    return ctx;
}

static void ssl_tear_down(SSL_CTX *ctx)
{
	SSL_CTX_free(ctx);
}

static char *file_size(char *ret, int s, size_t size)
{
    char t;
    float d;
    float f = size;
    if (size < 1000*1000) {
	t = 'K';
	d = 1024;
    } else if (size < 1000*1000*1000) {
	t = 'M';
	d = 1024*1024;
    } else {
	t = 'G';
	d = 1024*1024*1024;
    }

    f = f / d;
    if ((size > 0) && (f < 0.1))
	f = 0.1;

    snprintf(ret, s, "%.1f%c", f, t);

    return ret;
}

static void thread_upload(void *arg)
{
    char *buf = alloca(BUF_SIZE);
    upload_info *c = (upload_info *) arg;

    char *path = url_decode(c->path);

    //fprintf(stderr, "before: %s\n", path);
    remove_slashes(c->path);
    remove_slashes(path);
    //fprintf(stderr, "after: %s\n", path);

    c->ssl = SSL_new(ctx_data);

    SSL_set_fd(c->ssl, tcp_fd(c->channel));

    if (SSL_connect(c->ssl) < 0) {
	warning("SSL_connect(arg->ssl)");
    } else if (SSL_write(c->ssl, c->key, KEY_SIZE) == KEY_SIZE) {
	if (!strncmp(dir_prefix, path, strlen(dir_prefix))) {

#ifdef _WIN32
	    wchar_t wbuf[BUF_SIZE];
	    char *_path = alloca(strlen(path));
	    memset(wbuf, 0, BUF_SIZE * sizeof(wchar_t));
	    Utf8ToUnicode16(path, wbuf, BUF_SIZE);
	    Unicode16ToACP(wbuf, _path, BUF_SIZE);
	    snprintf(buf, BUF_SIZE, "%s/%s", dir_root, _path + strlen(dir_prefix));
#else
	    snprintf(buf, BUF_SIZE, "%s/%s", dir_root, path + strlen(dir_prefix));
#endif

	    char *normal_path = _realpath(buf, NULL);

	    //fprintf(stderr, "Localpath =  %s - %s - %s - %d\n", path, normal_path, dir_root, is_root);

	    fprintf(stderr, "Get: %s\n", normal_path);

	    if (!normal_path) {
		send_404(c->ssl, path);
	    } else if (!strncmp(normal_path, dir_root, strlen(dir_root))) {
		int is_root = 0;
		if (!strcmp(normal_path, dir_root))
		    is_root = 1;
#ifdef _WIN32
		if (normal_path[strlen(normal_path) - 1] == '\\')
		    normal_path[strlen(normal_path) - 1] = 0;
#endif
		struct stat sb;
		if (stat(normal_path, &sb) != -1) {
		    if ((sb.st_mode & S_IFMT) == S_IFDIR) {
			char *page = alloca(strlen(tmpl_dir_header) + BUF_SIZE);
			send_header(c->ssl, "text/html", 0, 0);
			snprintf(page, strlen(tmpl_dir_header) + BUF_SIZE, tmpl_dir_header, path, path);
			SSL_write(c->ssl, page, strlen(page));

#ifndef _WIN32
			struct dirent **namelist;
			int n;

			n = scandir(normal_path, &namelist, 0, alphasort);
			if (n >= 0) {
			    int i;
			    for (i = 0; i < n; i++) {
				snprintf(buf, BUF_SIZE, "%s/%s", normal_path, namelist[i]->d_name);
				if (stat(buf, &sb) != -1) {
				    char *ftype;
				    char fsize[256];
				    char ftime[256];
				    switch (sb.st_mode & S_IFMT) {
					case S_IFBLK: ftype = "Block device";		break;
					case S_IFCHR: ftype = "Character device";	break;
					case S_IFDIR: ftype = "Directory";		break;
					case S_IFIFO: ftype = "FIFO/pipe";		break;
					case S_IFLNK: ftype = "Symlink";		break;
					case S_IFREG: ftype = "Regular file";		break;
					case S_IFSOCK: ftype = "Socket";		break;
					default: ftype = "Unknown?";			break;
				    }

				    if ((sb.st_mode & S_IFMT) == S_IFREG)
					file_size(fsize, sizeof(fsize), sb.st_size);
				    else
					strcpy(fsize, "-");

				    char *name = url_encode(namelist[i]->d_name);
				    if (!(name[0] == '.' && name[1] == '.' && name[2] == 0)) {
					struct tm *tm = localtime(&sb.st_mtime);
					strftime(ftime, sizeof(ftime), "%Y-%b-%d %H-%M-%S", tm);
				    } else
					strcpy(ftime, "");

				    if (!(is_root && name[0] == '.' && name[1] == '.' && name[2] == 0) &&
					!(name[0] == '.' && name[1] == 0)) {
					snprintf(buf, BUF_SIZE, "<tr><td class=\"n\"><a href=\"%s/%s\">%s</a></td><td class=\"m\">%s</td><td class=\"s\">%s</td><td class=\"t\">%s</td></tr>", c->path, name, namelist[i]->d_name, ftime, fsize, ftype);
					SSL_write(c->ssl, buf, strlen(buf));
				    }
				    free(name);
				}
				free(namelist[i]);
			    }
			    free(namelist);
			}
#else
			WIN32_FIND_DATA ffd;
			HANDLE hFind = INVALID_HANDLE_VALUE;

			char szDir[_MAX_PATH];
			snprintf(szDir, _MAX_PATH, "%s\\*", normal_path);

			if ((hFind = FindFirstFile(szDir, &ffd)) != INVALID_HANDLE_VALUE) {
			    do {
				char *ftype;
				char fsize[256];
				char ftime[256];
				wchar_t szFile[_MAX_PATH];
				char szFile1[_MAX_PATH];

				if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				    ftype = "Directory";
				    strcpy(fsize, "-");
				} else {
				    ftype = "Regular file";
				    file_size(fsize, sizeof(fsize), (ffd.nFileSizeHigh * (MAXDWORD + 1)) + ffd.nFileSizeLow);
				}

				ACPToUnicode16(ffd.cFileName, szFile, _MAX_PATH);
				Unicode16ToUtf8(szFile, szFile1, _MAX_PATH);
				char *name = url_encode(szFile1);

				if (!(name[0] == '.' && name[1] == '.' && name[2] == 0)) {
				    static const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
				    SYSTEMTIME systemTime;
				    FileTimeToSystemTime (&ffd.ftLastWriteTime, &systemTime);
				    snprintf(ftime, sizeof(ftime), "%04d-%s-%02d %02d:%02d:%02d",
					systemTime.wYear, months[systemTime.wMonth - 1], systemTime.wDay,
					systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
				} else
				    strcpy(ftime, "");

				if (!(is_root && name[0] == '.' && name[1] == '.' && name[2] == 0) &&
				    !(name[0] == '.' && name[1] == 0)) {
				    snprintf(buf, BUF_SIZE, "<tr><td class=\"n\"><a href=\"%s/%s\">%s</a></td><td class=\"m\">%s</td><td class=\"s\">%s</td><td class=\"t\">%s</td></tr>", c->path, name, szFile1, ftime, fsize, ftype);
				    SSL_write(c->ssl, buf, strlen(buf));
				}
				free(name);
			    } while (FindNextFile(hFind, &ffd) != 0);
			    FindClose(hFind);
			}
#endif
			snprintf(page, strlen(tmpl_dir_header) + BUF_SIZE, tmpl_dir_footer, c->path);
			SSL_write(c->ssl, page, strlen(page));

		    } else {
			FILE *f = fopen(normal_path, "rb");
			if (f) {
			    int r;
			    send_header(c->ssl, "application/octet-stream", 1, sb.st_size);
			    while ((r = fread(buf, 1, BUF_SIZE, f)) > 0) {
				if (SSL_write(c->ssl, buf, r) != r)
				    break;
			    }
			    fclose(f);
			} else
			    send_404(c->ssl, path);
		    }
		} else
		    send_404(c->ssl, path);
	    } else
		send_404(c->ssl, path);
	    free(normal_path);
	} else {
	    send_404(c->ssl, path);
	}
    } else
	warning("SSL_write(c->key)");

    free(path);
    SSL_shutdown(c->ssl);
    SSL_free(c->ssl);
    tcp_close(c->channel);
    free(c);
}

int client_connect(char *_host, int _port, char *_root_dir, int *_sock)
{
    SSL *ssl;
    char *host;
    int port;
    int r;

    host = _host;
    port = _port;
    dir_root = _realpath(_root_dir, NULL);

    fprintf(stderr, "Shared directory: %s\n", dir_root);

#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif

    tcp_channel *server = tcp_open(TCP_CLIENT, host, port);
    if (!server) {
	fprintf(stderr, "tcp_open()\n");
	free(dir_root);
	return 1;
    }

    ctx = ssl_initialize();
    ctx_data = ssl_initialize();

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, tcp_fd(server));

    if (_sock)
	*_sock = tcp_fd(server);

    if (SSL_connect(ssl) < 0) {
	fprintf(stderr, "SSL_connect()\n");
    } else {
	char buf[BUF_SIZE];
	snprintf(buf, BUF_SIZE, "VERSION: %f", VERSION);
	if ((r = SSL_write(ssl, (uint8_t *)buf, strlen(buf))) <= 0) {
	    fprintf(stderr, "SSL_write()\n");
	    goto exit1;
	}
	if ((r = SSL_read(ssl, (uint8_t *)buf, BUF_SIZE)) <= 0) {
	    fprintf(stderr, "SSL_read()\n");
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
		snprintf(buf, BUF_SIZE, "https://%s:%d%s", host, port - 100, dir_prefix);
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

	while (1) {
	    if ((r = SSL_read(ssl, (uint8_t *)buf, BUF_SIZE)) <= 0) {
		fprintf(stderr, "SSL_read()\n");
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
		arg->channel = tcp_open(TCP_CLIENT, host, port + 1);
		if (arg->channel) {
#if !defined(_WIN32) || defined(ENABLE_PTHREADS)
		    pthread_t tid;
#endif
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
			    warning("pthread_create(thread_upload)");
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

    SSL_shutdown(ssl);
    SSL_free(ssl);
    tcp_close(server);
    ssl_tear_down(ctx);
    ssl_tear_down(ctx_data);
    free(dir_root);

    return 0;
}

void client_disconnect(int _sock)
{
#ifdef _WIN32
    closesocket(_sock);
#else
    close(_sock);
#endif
}

#ifndef CLIENT_GUI
int main(int argc, char *argv[])
{
    char *host;
    int port;
    char *dir_root;

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

    return client_connect(host, port, dir_root, NULL);
}
#endif
