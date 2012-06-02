#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include "utils.h"

static struct {
    char *ext;
    char *mime;
} mimetypes[] = {
#include "mime.h"
{NULL, NULL}
};

#ifdef ANDROID
#undef realpath
#define realpath _realpath
#include "realpath.c"
#endif

#ifdef __APPLE__
#include <stdlib.h>

char *_realpath(const char *path, char *resolved)
{
    char *ret;
    char *_ret;

    if (!resolved)
	ret = malloc(PATH_MAX);
    else
	ret = resolved;

    if (!(_ret = realpath(path, ret))) {
	if (!resolved)
	    free(ret);
    } else {
	if (ret[strlen(ret) - 1] == '\\')
	    ret[strlen(ret) - 1] = 0;
    }

    return _ret;
}
#endif

#ifdef _WIN32
char *realpath(const char *path, char *resolved)
{
    char *ret;
    char *_ret;

    if (!resolved)
	ret = malloc(_MAX_PATH);
    else
	ret = resolved;

    if (!(_ret = _fullpath(ret, path, _MAX_PATH))) {
	if (!resolved)
	    free(ret);
    } else {
	if (ret[strlen(ret) - 1] == '\\')
	    ret[strlen(ret) - 1] = 0;
    }

    return _ret;
}

BOOL Unicode16ToUtf8(WCHAR *in_Src, CHAR *out_Dst, INT in_MaxLen)
{
    INT  lv_Len;
    if (in_MaxLen <= 0)
	return FALSE;

    lv_Len = WideCharToMultiByte(CP_UTF8, 0, in_Src, -1, out_Dst, in_MaxLen, 0, 0);

    if (lv_Len < 0)
	lv_Len = 0;

    if (lv_Len < in_MaxLen)
	out_Dst[lv_Len] = 0;
    else if (out_Dst[in_MaxLen-1])
	out_Dst[0] = 0;

    return TRUE;
}

BOOL Utf8ToUnicode16(CHAR *in_Src, WCHAR *out_Dst, INT in_MaxLen)
{
    INT lv_Len;
    if (in_MaxLen <= 0)
	return FALSE;

    lv_Len = MultiByteToWideChar(CP_UTF8, 0, in_Src, -1, out_Dst, in_MaxLen);

    if (lv_Len < 0)
	lv_Len = 0;

    if (lv_Len < in_MaxLen)
	out_Dst[lv_Len] = 0;
    else if (out_Dst[in_MaxLen-1])
	out_Dst[0] = 0;

    return TRUE;
}

BOOL Unicode16ToACP(WCHAR *in_Src, CHAR *out_Dst, INT in_MaxLen)
{
    INT  lv_Len;
    if (in_MaxLen <= 0)
	return FALSE;

    lv_Len = WideCharToMultiByte(CP_ACP, 0, in_Src, -1, out_Dst, in_MaxLen, 0, 0);

    if (lv_Len < 0)
	lv_Len = 0;

    if (lv_Len < in_MaxLen)
	out_Dst[lv_Len] = 0;
    else if (out_Dst[in_MaxLen-1])
	out_Dst[0] = 0;

    return TRUE;
}

BOOL ACPToUnicode16(CHAR *in_Src, WCHAR *out_Dst, INT in_MaxLen)
{
    INT lv_Len;
    if (in_MaxLen <= 0)
	return FALSE;

    lv_Len = MultiByteToWideChar(CP_ACP, 0, in_Src, -1, out_Dst, in_MaxLen);

    if (lv_Len < 0)
	lv_Len = 0;

    if (lv_Len < in_MaxLen)
	out_Dst[lv_Len] = 0;
    else if (out_Dst[in_MaxLen-1])
	out_Dst[0] = 0;

    return TRUE;
}
#endif

char *remove_slashes(char *str)
{
    int i;

    for (i = 0; i < strlen(str) && str[i] != 0; i++) {
	while ((str[i] == '/') && (str[i + 1] == '/'))
	    memcpy(str + i, str + i + 1, strlen(str + i + 1) + 1);
    }
    if (str[i - 1] == '/')
	str[i - 1] = 0;

    return str;
}

char *get_mimetype(char *file)
{
    char *ptr = strrchr(file, '.');
    if (ptr && strlen(ptr + 1)) {
	int i = 0;
	while (mimetypes[i].ext) {
	    if (!strcmp(mimetypes[i].ext, ptr))
		return mimetypes[i].mime;
	    i++;
	}
    }

    return "application/octet-stream";
}
