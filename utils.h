#ifndef _UTILS_H_
#define _UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
#define  dprintf(...)  fprintf(stderr, __VA_ARGS__)
#else
#define dprintf(...)
#endif

#if !defined(__APPLE__) && !defined(ANDROID)
#define _realpath realpath
#endif

#if defined(__APPLE__) || defined(ANDROID)
char *_realpath(const char *path, char *resolved);
#endif

#ifdef _WIN32
char *realpath(const char *path, char *resolved);
BOOL Unicode16ToUtf8(WCHAR *in_Src, CHAR *out_Dst, INT in_MaxLen);
BOOL Utf8ToUnicode16(CHAR *in_Src, WCHAR *out_Dst, INT in_MaxLen);
BOOL Unicode16ToACP(WCHAR *in_Src, CHAR *out_Dst, INT in_MaxLen);
BOOL ACPToUnicode16(CHAR *in_Src, WCHAR *out_Dst, INT in_MaxLen);
#endif

char *remove_slashes(char *str);
char *get_mimetype(char *file);

#ifdef __cplusplus
}
#endif

#endif
