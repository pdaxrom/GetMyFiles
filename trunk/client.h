#ifndef _CLIENT_H_
#define _CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char	*host;
    int		port;
    const char	*root_dir;
    int		enable_httpd;
    int		max_ext_conns;
    int		max_int_conns;
    int		exit_request;
    void	*priv;
} client_args;

#ifdef CLIENT_GUI
void show_server_directory(client_args *client, char *str);
void update_client(client_args *client, char *vers);
#endif

int client_connect(client_args *client);

#ifdef __cplusplus
}
#endif

#endif
