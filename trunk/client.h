#ifndef _CLIENT_H_
#define _CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CLIENT_GUI
void show_server_directory(char *str);
void update_client(char *vers);
#endif

int client_connect(char *_host, int _port, char *_root_dir, int *_exit_request);

#ifdef __cplusplus
}
#endif

#endif
