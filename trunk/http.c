#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#ifndef _WIN32
#include <dirent.h>
#include <errno.h>
#else
#include <windows.h>
#endif

#include "urldecode.h"
#include "http.h"
#include "utils.h"
#include "getaddr.h"

#ifdef __MINGW32__
void *alloca(size_t);
#endif

#define BUF_SIZE 4096

#define BUF_RESP_SIZE	1024

static const char *tmpl_page_begin =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">"
"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">";

static const char *tmpl_header_begin =
"<head>";

static const char *tmpl_title =
"<title>%s</title>";

static const char *tmpl_title_error =
"<title>%d %s</title>";

static const char *tmpl_title_dir =
"<title>Index of %s</title>";

static const char *tmpl_charset =
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />";

static const char *tmpl_scripts =
"<script type=\"text/javascript\" src=\"/js/p2p.js\"></script>";
//"<script type=\"text/javascript\" src=\"/js/imageviewer.js\"></script>"
//"<script type=\"text/javascript\" src=\"/js/player5.js\"></script>";

static const char *tmpl_style =
"<style type=\"text/css\">"
"a, a:active {text-decoration: none; color: blue;}"
"a:visited {color: #48468F;}"
"a:hover, a:focus {text-decoration: underline; color: red;}"
"body {background-color: #F5F5F5;}"
"h2 {margin-bottom: 12px;}"
"table {margin-left: 12px;}"
"th, td { font: 90% monospace; text-align: left;}"
"th { font-weight: bold; padding-right: 14px; padding-bottom: 3px;}"
"td {padding-right: 14px;}"
"td.s, th.s {text-align: right;}"
"div.list { background-color: white; border-top: 1px solid #646464; border-bottom: 1px solid #646464; padding-top: 10px; padding-bottom: 14px;}"
"div.err { font: 90% monospace; color: #F50000; text-align: left;}"
"div.foot { font: 90% monospace; color: #787878; padding-top: 4px;}"
"</style>";

static const char *tmpl_header_end =
"</head>";

static const char *tmpl_body_begin_dir =
"<body>"
"<h2>Index of %s</h2>"
"<div class=\"list\">"
"<table summary=\"Directory Listing\" cellpadding=\"0\" cellspacing=\"0\">"
"<thead><tr><th class=\"n\">Name</th><th class=\"m\">Last Modified</th><th class=\"s\">Size</th><th class=\"t\">Type</th></tr></thead>"
"<tbody>";

static const char *tmpl_body_end_dir =
"</tbody>"
"</table>"
"</div>"
"<div class=\"foot\">Powered by <a href=\"http://getmyfil.es\">getmyfil.es</a></div>"
"<script type=\"text/javascript\" src=\"http://webplayer.yahooapis.com/player.js\"></script>"
"</body>";

static const char *tmpl_body_error =
"<body>"
"<h2>Not Found</h2>"
"<div class=\"list\">"
"<div class=\"err\">The requested URL %s was not found on this server.</div>"
"</div>"
"<div class=\"foot\">Powered by <a href=\"http://getmyfil.es\">getmyfil.es</a></div>"
"</body>";

static const char *tmpl_body_error_416 =
"<body>"
"<h2>Requested Range Not Satisfiable</h2>"
"<div class=\"list\">"
"<div class=\"err\">The file is already fully retrieved; nothing to do.</div>"
"</div>"
"<div class=\"foot\">Powered by <a href=\"http://getmyfil.es\">getmyfil.es</a></div>"
"</body>";

static const char *tmpl_body_error_501 =
"<body>"
"<h2>Not Implemented</h2>"
"<div class=\"list\">"
"<div class=\"err\">The request is not supported.</div>"
"</div>"
"<div class=\"foot\">Powered by <a href=\"http://getmyfil.es\">getmyfil.es</a></div>"
"</body>";

static const char *tmpl_body_error_502 =
"<body>"
"<h2>Bad Gateway</h2>"
"<div class=\"list\">"
"<div class=\"err\">The server received an invalid response from the requested shared link.</div>"
"</div>"
"<div class=\"foot\">Powered by <a href=\"http://getmyfil.es\">getmyfil.es</a></div>"
"</body>";

static const char *tmpl_body_error_504 =
"<body>"
"<h2>Gateway Timeout</h2>"
"<div class=\"list\">"
"<div class=\"err\">The server did not receive a timely response from the requested shared link.</div>"
"</div>"
"<div class=\"foot\">Powered by <a href=\"http://getmyfil.es\">getmyfil.es</a></div>"
"</body>";

static const char *tmpl_page_end =
"</html>";

static char *http_version = "HTTP/1.1";

char *http_response_begin(int status, char *reason)
{
    char *resp = malloc(BUF_RESP_SIZE);
    if (resp)
	snprintf(resp, BUF_RESP_SIZE, "%s %d %s\n", http_version, status, reason);

    return resp;
}

char *http_response_add_content_type(char *resp, char *mime)
{
    char *ptr = resp + strlen(resp);
    snprintf(ptr, BUF_RESP_SIZE - strlen(resp), "Content-Type: %s\n", mime);

    return resp;
}

char *http_response_add_content_length(char *resp, size_t length)
{
    char *ptr = resp + strlen(resp);
    snprintf(ptr, BUF_RESP_SIZE - strlen(resp), "Content-Length: %lu\n", length);

    return resp;
}

char *http_response_add_connection(char *resp, char *token)
{
    char *ptr = resp + strlen(resp);
    snprintf(ptr, BUF_RESP_SIZE - strlen(resp), "Connection: %s\n", token);

    return resp;
}

char *http_response_add_accept_ranges(char *resp)
{
    char *ptr = resp + strlen(resp);
    snprintf(ptr, BUF_RESP_SIZE - strlen(resp), "Accept-Ranges: bytes\n");

    return resp;
}

char *http_response_add_range(char *resp, size_t from, size_t to, size_t length)
{
    char *ptr = resp + strlen(resp);
    snprintf(ptr, BUF_RESP_SIZE - strlen(resp), "Content-Range: bytes %lu-%lu/%lu\n", from, to, length);

    return resp;
}

char *http_response_end(char *resp)
{
    return strncat(resp, "\n", BUF_RESP_SIZE);
}

int send_404(tcp_channel *c, char *url)
{
    char page[BUF_SIZE];
    char *resp = http_response_begin(404, "Not found");
    http_response_add_content_type(resp, "text/html; charset=UTF-8");
    http_response_add_connection(resp, "close");
    http_response_end(resp);
    if (tcp_write(c, resp, strlen(resp)) != strlen(resp)) {
	free(resp);
	return 1;
    }
    free(resp);

    char *d_url = url_decode(url);
    snprintf(page, sizeof(page), "%s", tmpl_page_begin);
    tcp_write(c, page, strlen(page));
    snprintf(page, sizeof(page), "%s", tmpl_header_begin);
    tcp_write(c, page, strlen(page));
    snprintf(page, sizeof(page), tmpl_title, "404 Not Found");
    tcp_write(c, page, strlen(page));
    snprintf(page, sizeof(page), "%s", tmpl_charset);
    tcp_write(c, page, strlen(page));
    snprintf(page, sizeof(page), "%s", tmpl_style);
    tcp_write(c, page, strlen(page));
    snprintf(page, sizeof(page), "%s", tmpl_header_end);
    tcp_write(c, page, strlen(page));
    snprintf(page, sizeof(page), tmpl_body_error, d_url);
    tcp_write(c, page, strlen(page));
    snprintf(page, sizeof(page), "%s", tmpl_page_end);
    tcp_write(c, page, strlen(page));
    free(d_url);

    return 0;
}

int send_error(tcp_channel *c, int error)
{
    char page[BUF_SIZE];
    char *err_text;
    const char *template;
    switch (error) {
    case 416:  err_text = "Requested Range Not Satisfiable"; template = tmpl_body_error_416; break;
    case 502:  err_text = "Bad Gateway"; template = tmpl_body_error_502; break;
    case 504:  err_text = "Gateway Timeout"; template = tmpl_body_error_504; break;
    default:   err_text = "Not Implemented"; template = tmpl_body_error_501; break;
    }
    char *resp = http_response_begin(error, err_text);
    http_response_add_content_type(resp, "text/html; charset=UTF-8");
    http_response_add_connection(resp, "close");
    http_response_end(resp);
    if (tcp_write(c, resp, strlen(resp)) != strlen(resp)) {
	free(resp);
	return 1;
    }
    free(resp);

    snprintf(page, sizeof(page), "%s", tmpl_page_begin);
    tcp_write(c, page, strlen(page));
    snprintf(page, sizeof(page), "%s", tmpl_header_begin);
    tcp_write(c, page, strlen(page));
    snprintf(page, sizeof(page), tmpl_title_error, error, err_text);
    tcp_write(c, page, strlen(page));
    snprintf(page, sizeof(page), "%s", tmpl_charset);
    tcp_write(c, page, strlen(page));
    snprintf(page, sizeof(page), "%s", tmpl_style);
    tcp_write(c, page, strlen(page));
    snprintf(page, sizeof(page), "%s", tmpl_header_end);
    tcp_write(c, page, strlen(page));
    snprintf(page, sizeof(page), "%s", template);
    tcp_write(c, page, strlen(page));
    snprintf(page, sizeof(page), "%s", tmpl_page_end);
    tcp_write(c, page, strlen(page));

    return 0;
}

char *get_request_tag(char *header, char *tag)
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

static void send_p2p_ips(tcp_channel *c)
{
    static char *script_begin = "<script>var my_ips=[";
    static char *script_end = "];</script>";
    int i = 0;
    char buf[128];
    char **ip_list = get_ipaddr_list();
    tcp_write(c, script_begin, strlen(script_begin));
    while (ip_list[i]) {
	snprintf(buf, sizeof(buf), "\"%s:%d\",", ip_list[i++], 8000);
	tcp_write(c, buf, strlen(buf));
    }
    tcp_write(c, script_end, strlen(script_end));
}

int process_dir(tcp_channel *c, char *url, char *http_request, char *path, int is_root, int *exit_request)
{
    char buf[BUF_SIZE];

    struct stat sb;
    if (stat(path, &sb) != -1) {
	if ((sb.st_mode & S_IFMT) == S_IFDIR) {
//	    char *page = alloca(strlen(tmpl_dir_header) + BUF_SIZE);
	    char page[BUF_SIZE];
	    char *resp = http_response_begin(200, "OK");
	    http_response_add_content_type(resp, "text/html; charset=UTF-8");
	    http_response_end(resp);
	    if (tcp_write(c, resp, strlen(resp)) != strlen(resp)) {
		free(resp);
		return -1;
	    }
	    free(resp);

	    char *d_url = url_decode(url);
	    snprintf(page, sizeof(page), "%s", tmpl_page_begin);
	    tcp_write(c, page, strlen(page));
	    snprintf(page, sizeof(page), "%s", tmpl_header_begin);
	    tcp_write(c, page, strlen(page));
	    snprintf(page, sizeof(page), tmpl_title_dir, d_url);
	    tcp_write(c, page, strlen(page));
	    snprintf(page, sizeof(page), "%s", tmpl_charset);
	    tcp_write(c, page, strlen(page));
	    send_p2p_ips(c);
	    snprintf(page, sizeof(page), "%s", tmpl_scripts);
	    tcp_write(c, page, strlen(page));
	    snprintf(page, sizeof(page), "%s", tmpl_style);
	    tcp_write(c, page, strlen(page));
	    snprintf(page, sizeof(page), "%s", tmpl_header_end);
	    tcp_write(c, page, strlen(page));
	    snprintf(page, sizeof(page), tmpl_body_begin_dir, d_url);
	    tcp_write(c, page, strlen(page));
	    free(d_url);
#ifndef _WIN32
	    struct dirent **namelist;
	    int n = scandir(path, &namelist, 0, alphasort);
	    if (n >= 0) {
		int i;
		for (i = 0; i < n; i++) {
		    snprintf(buf, sizeof(buf), "%s/%s", path, namelist[i]->d_name);
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

			if ((sb.st_mode & S_IFMT) == S_IFREG) {
			    ftype = get_mimetype(name);
			}

			if (!(is_root && name[0] == '.' && name[1] == '.' && name[2] == 0) &&
			    !(name[0] == '.' && name[1] == 0)) {
			    snprintf(buf, BUF_SIZE, "<tr><td class=\"n\"><a href=\"%s/%s\">%s</a></td><td class=\"m\">%s</td><td class=\"s\">%s</td><td class=\"t\">%s</td></tr>", url, name, namelist[i]->d_name, ftime, fsize, ftype);
			    tcp_write(c, buf, strlen(buf));
			}
			free(name);
		    }
		    free(namelist[i]);
		    if (exit_request)
			if (*exit_request)
			    break;
		}
		for (; i < n; i++)
		    free(namelist[i]);
		free(namelist);
	    }
#else
	    WIN32_FIND_DATA ffd;
	    HANDLE hFind = INVALID_HANDLE_VALUE;

	    char szDir[_MAX_PATH];
	    snprintf(szDir, _MAX_PATH, "%s\\*", path);

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
			ftype = get_mimetype(ffd.cFileName);
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
			snprintf(buf, BUF_SIZE, "<tr><td class=\"n\"><a href=\"%s/%s\">%s</a></td><td class=\"m\">%s</td><td class=\"s\">%s</td><td class=\"t\">%s</td></tr>", url, name, szFile1, ftime, fsize, ftype);
			tcp_write(c, buf, strlen(buf));
		    }
		    free(name);
		    if (exit_request)
			if (*exit_request)
			    break;
		} while (FindNextFile(hFind, &ffd) != 0);
		FindClose(hFind);
	    }
#endif
	    snprintf(page, sizeof(page), "%s", tmpl_body_end_dir);
	    tcp_write(c, page, strlen(page));
	    snprintf(page, sizeof(page), "%s", tmpl_page_end);
	    tcp_write(c, page, strlen(page));
	} else {
	    size_t file_len = sb.st_size;
	    size_t pos_from = 0;
	    size_t pos_to = file_len - 1;

	    //fprintf(stderr, "[%s]\n", http_request);
	    char *tag_range = get_request_tag(http_request, "Range:");
	    if (tag_range) {
		sscanf(tag_range, "bytes=%lu-%lu", &pos_from, &pos_to);
		if (pos_to <= 0)
		    pos_to = file_len - 1;
		if (pos_from < 0)
		    pos_from = 0;
		//fprintf(stderr, "From %lu, to %lu, size %lu\n", pos_from, pos_to, file_len);
		free(tag_range);
	    }

	    if ((pos_from > pos_to) || (pos_to >= file_len)) {
		send_error(c, 416);
		return 0;
	    }

	    FILE *f = fopen(path, "rb");
	    if (f) {
		int need_read = pos_to - pos_from + 1;
		char *resp;
		if ((pos_from > 0) || (pos_to != file_len - 1))
		    resp = http_response_begin(206, "Partial Content");
		else
		    resp = http_response_begin(200, "OK");
		http_response_add_content_type(resp, get_mimetype(path));
		http_response_add_content_length(resp, need_read);
		http_response_add_accept_ranges(resp);
		if ((pos_from > 0) || (pos_to != file_len - 1))
		    http_response_add_range(resp, pos_from, pos_to, file_len);
		http_response_end(resp);
		if (tcp_write(c, resp, strlen(resp)) != strlen(resp)) {
		    free(resp);
		    fclose(f);
		    return -1;
		}
		free(resp);

		if (pos_from > 0)
		    fseek(f, pos_from, SEEK_SET);
		
		while (need_read > 0) {
		    int len = (need_read > BUF_SIZE)?BUF_SIZE:need_read;
		    int ret = fread(buf, 1, len, f);
		    if (ret != len)
			break;
		    if (tcp_write(c, buf, ret) != ret)
			break;
		    if (exit_request)
			if (*exit_request)
			    break;
		    need_read -= ret;
		}
		fclose(f);
	    } else
		send_404(c, url);
	}
    } else
	send_404(c, url);
    return 0;
}

int process_page(tcp_channel *channel, char *url, char *http_request, char *dir_prefix, char *dir_root, int *exit_request)
{
    char buf[BUF_SIZE];
    char *path = url_decode(url);

    //fprintf(stderr, "before: %s\n", path);
    remove_slashes(url);
    remove_slashes(path);
    //fprintf(stderr, "after: %s\n", path);


    if (!strncmp(dir_prefix, path, strlen(dir_prefix))) {
#ifdef _WIN32
	wchar_t wbuf[BUF_SIZE];
	char *_path = alloca(strlen(path));
	memset(wbuf, 0, BUF_SIZE * sizeof(wchar_t));
	Utf8ToUnicode16(path, wbuf, BUF_SIZE);
	Unicode16ToACP(wbuf, _path, BUF_SIZE);
	snprintf(buf, sizeof(buf), "%s/%s", dir_root, _path + strlen(dir_prefix));
#else
	snprintf(buf, sizeof(buf), "%s/%s", dir_root, path + strlen(dir_prefix));
#endif

	char *normal_path = _realpath(buf, NULL);

	//fprintf(stderr, "Localpath =  %s - %s - %s - %d\n", path, normal_path, dir_root, is_root);

	fprintf(stderr, "Get: %s\n", normal_path);

	if (!normal_path) {
	    send_404(channel, path);
	} else if (!strncmp(normal_path, dir_root, strlen(dir_root))) {
	    int is_root = 0;
	    if (!strcmp(normal_path, dir_root))
		is_root = 1;
#ifdef _WIN32
	    if (normal_path[strlen(normal_path) - 1] == '\\')
		normal_path[strlen(normal_path) - 1] = 0;
#endif
	    process_dir(channel, url, http_request, normal_path, is_root, exit_request);
	} else
	    send_404(channel, url);
	free(normal_path);
    } else {
	send_404(channel, url);
    }

    free(path);

    return 0;
}
