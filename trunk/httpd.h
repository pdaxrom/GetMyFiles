#ifndef _HTTPD_H_
#define _HTTPD_H_

typedef struct {
    char	*root;
    char	*prefix;
    int		port;
    int		exit_request;
} httpd_args;

int httpd_main(httpd_args *args);

#endif
