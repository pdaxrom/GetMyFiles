#ifndef _CLIENT_H_
#define _CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

void show_server_directory(char *str);

int client_connect(char *_host, int _port, char *_root_dir, int *_sock);

void client_disconnect(int _sock);


#ifdef __cplusplus
}
#endif

#endif
