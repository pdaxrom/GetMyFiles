#ifndef TCP_H
#define TCP_H

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#else
#include <windows.h>
#endif

enum {
    TCP_SERVER = 0,
    TCP_CLIENT
};

typedef struct _tcp_channel {
    int s;
    struct sockaddr_in my_addr;
    int mode;
} tcp_channel;

#define tcp_fd(u) (u->s)

tcp_channel *tcp_open(int mode, char *addr, int port);
tcp_channel *tcp_accept(tcp_channel *u);
int tcp_read(tcp_channel *u, uint8_t *buf, size_t len);
int tcp_write(tcp_channel *u, uint8_t *buf, size_t len);
int tcp_close(tcp_channel *u);

#endif
