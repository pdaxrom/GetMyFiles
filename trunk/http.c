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

#define BUF_SIZE 1024

#define BUF_RESP_SIZE	1024

static const char *tmpl_404 = "<html><head><title>404 Not Found</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" /></head><body><b>Oops!</b><br /><br />The requested URL %s was not found :(</body></html>";

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

char *http_response_end(char *resp)
{
    return strncat(resp, "\n", BUF_RESP_SIZE);
}

int send_404(tcp_channel *c, char *url)
{
    int ret = -1;
    char buf[BUF_SIZE];
    char *resp = http_response_begin(404, "Not found");
    http_response_add_content_type(resp, "text/html; charset=UTF-8");
    http_response_add_connection(resp, "close");
    http_response_end(resp);

    snprintf(buf, sizeof(buf), tmpl_404, url);

    if (tcp_write(c, resp, strlen(resp)) == strlen(resp)) {
	if (tcp_write(c, buf, strlen(buf)) == strlen(buf))
	    ret = 0;
    }
    free(resp);
    return 0;
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

int process_dir(tcp_channel *c, char *url, char *path, int is_root)
{
    char buf[BUF_SIZE];

    struct stat sb;
    if (stat(path, &sb) != -1) {
	if ((sb.st_mode & S_IFMT) == S_IFDIR) {
	    char *page = alloca(strlen(tmpl_dir_header) + BUF_SIZE);
	    char *resp = http_response_begin(200, "OK");
	    http_response_add_content_type(resp, "text/html; charset=UTF-8");
	    http_response_end(resp);
	    if (tcp_write(c, resp, strlen(resp)) != strlen(resp)) {
		free(resp);
		return -1;
	    }
	    free(resp);

	    snprintf(page, strlen(tmpl_dir_header) + BUF_SIZE, tmpl_dir_header, path, path);
	    tcp_write(c, page, strlen(page));

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

			if (!(is_root && name[0] == '.' && name[1] == '.' && name[2] == 0) &&
			    !(name[0] == '.' && name[1] == 0)) {
			    snprintf(buf, BUF_SIZE, "<tr><td class=\"n\"><a href=\"%s/%s\">%s</a></td><td class=\"m\">%s</td><td class=\"s\">%s</td><td class=\"t\">%s</td></tr>", url, name, namelist[i]->d_name, ftime, fsize, ftype);
			    tcp_write(c, buf, strlen(buf));
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
			tcp_write(c, buf, strlen(buf));
		    }
		    free(name);
		} while (FindNextFile(hFind, &ffd) != 0);
		    FindClose(hFind);
	    }
#endif
	    snprintf(page, strlen(tmpl_dir_header) + BUF_SIZE, tmpl_dir_footer, url);
	    tcp_write(c, page, strlen(page));

	} else {
	    FILE *f = fopen(path, "rb");
	    if (f) {
		int r;
		char *resp = http_response_begin(200, "OK");
		http_response_add_content_type(resp, "application/octet-stream");
		http_response_add_content_length(resp, sb.st_size);
		http_response_end(resp);
		if (tcp_write(c, resp, strlen(resp)) != strlen(resp)) {
		    free(resp);
		    fclose(f);
		    return -1;
		}
		free(resp);
		while ((r = fread(buf, 1, BUF_SIZE, f)) > 0) {
		    if (tcp_write(c, buf, r) != r)
			break;
		}
		fclose(f);
	    } else
		send_404(c, path);
	}
    } else
	send_404(c, path);
    return 0;
}
