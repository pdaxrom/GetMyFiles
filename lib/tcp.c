#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef _WIN32
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#define closesocket close
#else
#include <windows.h>
#endif

#define PORT 9930

#include "tcp.h"

#ifdef _WIN32
typedef int socklen_t;

static int winsock_inited = 0;
static int winsock_init(void)
{
    WSADATA w;

    if (winsock_inited)
	return 0;

    /* Open windows connection */
    if (WSAStartup(0x0101, &w) != 0) {
	fprintf(stderr, "Could not open Windows connection.\n");
	return -1;
    }
    
    winsock_inited = 1;
    return 0;
}
#endif

tcp_channel *tcp_open(int mode, char *addr, int port)
{
#ifdef _WIN32
    if (winsock_init())
	return NULL;
#endif

    tcp_channel *u = (tcp_channel *)malloc(sizeof(tcp_channel));

    u->mode = mode;

    if ((u->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
	fprintf(stderr, "socket() error!\n");
	free(u);
	return NULL;
    }

    if (mode == TCP_SERVER) {
#ifndef _WIN32
	int yes = 1;
	if(setsockopt(u->s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
	    fprintf(stderr, "setsockopt() error!\n");
	    free(u);
	    return NULL;
	}
#endif

	memset(&u->my_addr, 0, sizeof(u->my_addr));
	u->my_addr.sin_family = AF_INET;
        u->my_addr.sin_addr.s_addr = INADDR_ANY;
        u->my_addr.sin_port = htons(port);

	if(bind(u->s, (struct sockaddr *)&u->my_addr, sizeof(u->my_addr)) == -1) {
	    fprintf(stderr, "bind() error!\n");
	    closesocket(u->s);
	    free(u);
	    return NULL;
	}

	if (listen(u->s, 10) == -1) {
	    fprintf(stderr, "listen() error!\n");
	    closesocket(u->s);
	    free(u);
	    return NULL;
	}
    } else {
	struct hostent *server = gethostbyname(addr);
	if (server == NULL) {
	    fprintf(stderr, "gethostbyname() no such host\n");
	    free(u);
	    return NULL;
	}

	memset(&u->my_addr, 0, sizeof(u->my_addr));
	u->my_addr.sin_family = AF_INET;
        u->my_addr.sin_addr = *((struct in_addr *)server->h_addr);
        u->my_addr.sin_port = htons(port);

	if (connect(u->s, (struct sockaddr *)&u->my_addr, sizeof(struct sockaddr)) == -1) {
	    fprintf(stderr, "connect()\n");
	    closesocket(u->s);
	    free(u);
	    return NULL;
	}
    }

    return u;
}

int tcp_close(tcp_channel *u)
{
    if (u->s >=0)
	closesocket(u->s);
    free(u);
/*
#ifdef _WIN32
    if (winsock_inited) {
	WSACleanup();
	winsock_inited = 0;
    }
#endif
 */
    return 0;
}

tcp_channel *tcp_accept(tcp_channel *u)
{
    tcp_channel *n = (tcp_channel *)malloc(sizeof(tcp_channel));
    n->mode = TCP_CLIENT;
    socklen_t l = sizeof(struct sockaddr);
    if ((n->s = accept(u->s, (struct sockaddr *)&n->my_addr, &l)) < 0) {
	fprintf(stderr, "accept()\n");
	free(n);
	return NULL;
    }

    return n;
}

int tcp_read(tcp_channel *u, uint8_t *buf, size_t len)
{
    int r;

    if ((r = recv(u->s, buf, len, 0)) == -1) {
    	fprintf(stderr, "recvfrom()\n");
    }

    return r;
}

int tcp_write(tcp_channel *u, uint8_t *buf, size_t len)
{
    int r;
    if ((r = send(u->s, buf, len, 0)) < 0) {
	    fprintf(stderr, "sendto()\n");
    }

    return r;
}
