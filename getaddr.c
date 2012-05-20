#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#endif

static char **ip_list = NULL;

#ifdef _WIN32
char **get_ipaddr_list(void)
{
    MIB_IPADDRTABLE	*pIPAddrTable;
    DWORD		dwSize = 0;
    DWORD		dwRetVal;

    if (ip_list)
	return ip_list;

    pIPAddrTable = (MIB_IPADDRTABLE*) malloc( sizeof(MIB_IPADDRTABLE) );

    if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
	free( pIPAddrTable );
	pIPAddrTable = (MIB_IPADDRTABLE *) malloc ( dwSize );
    }

    if ( (dwRetVal = GetIpAddrTable( pIPAddrTable, &dwSize, 0 )) == NO_ERROR ) {
	int j;
	int i = 1;
	for (j = 0; j < (int) pIPAddrTable->dwNumEntries; j++) {
	    IN_ADDR IPAddr;
	    IPAddr.S_un.S_addr = (u_long) pIPAddrTable->table[j].dwAddr;
	    if (!(!strncmp(inet_ntoa(IPAddr), "127.", 4))) {
		printf("IP Address[%d]:     \t%s\n", j, inet_ntoa(IPAddr) );
		ip_list = realloc(ip_list, (i + 1) * sizeof(char *));
		ip_list[i - 1] = strdup(inet_ntoa(IPAddr));
		ip_list[i++] = NULL;
	    }
	}
    }

    if (pIPAddrTable)
	free(pIPAddrTable);

    return ip_list;
}
#else
char **get_ipaddr_list(void)
{
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];
    int i = 1;

    if (ip_list)
	return ip_list;

    if (getifaddrs(&ifaddr) == -1) {
	perror("getifaddrs");
	return NULL;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
	if (ifa->ifa_addr == NULL)
	    continue;

	family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) {
	    s = getnameinfo(ifa->ifa_addr,
			    (family == AF_INET) ? sizeof(struct sockaddr_in) :
			    sizeof(struct sockaddr_in6),
			    host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
	    if (s != 0) {
		printf("getnameinfo() failed: %s\n", gai_strerror(s));
		goto exit1;
	    }
	    if (!(!strncmp(host, "127.", 4))) {
		printf("\taddress: <%s>\n", host);
		ip_list = realloc(ip_list, (i + 1) * sizeof(char *));
		ip_list[i - 1] = strdup(host);
		ip_list[i++] = NULL;
	    }
	}
    }
 exit1:
    freeifaddrs(ifaddr);
    return ip_list;
}
#endif

void free_ipaddr_list(void)
{
    int i;
    while (ip_list[i]) {
	free(ip_list[i++]);
    }
    free(ip_list);
    ip_list = NULL;
}
