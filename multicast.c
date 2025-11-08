#include "multicast.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int setup_multicast_socket(const AppConfig *config) {
    if (!config) {
        fprintf(stderr, "Invalid configuration: config is NULL\n");
        return -1;
    }

    printf("Initializing multicast receiver...\n");
    printf("Configured multicast group: %s\n", config->mc_group);
    printf("Configured multicast port : %d\n", config->mc_port);

    if (config->mc_port <= 0 || config->mc_port > 65535) {
        fprintf(stderr, "Invalid multicast port: %d (must be 1-65535)\n", config->mc_port);
        return -1;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }
    printf("UDP socket created (fd=%d)\n", sock);

    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        close(sock);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config->mc_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        return -1;
    }
    printf("Bound to INADDR_ANY:%d, waiting for multicast group join...\n", config->mc_port);

    struct in_addr mc_addr;
    if (inet_aton(config->mc_group, &mc_addr) == 0) {
        fprintf(stderr, "Invalid multicast group address: %s\n", config->mc_group);
        close(sock);
        return -1;
    }

    // Validate that the address is in the multicast range (224.0.0.0 - 239.255.255.255)
    uint32_t mc = ntohl(mc_addr.s_addr);
    if (mc < 0xE0000000U || mc > 0xEFFFFFFFU) {
        fprintf(stderr,
                "Address %s is not a valid IPv4 multicast address (expected 224.0.0.0-239.255.255.255)\n",
                config->mc_group);
        close(sock);
        return -1;
    }

    struct ip_mreqn mreq;
    memset(&mreq, 0, sizeof(mreq));
    mreq.imr_multiaddr = mc_addr;
    mreq.imr_address.s_addr = htonl(INADDR_ANY);
    mreq.imr_ifindex = 0;

    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt(IP_ADD_MEMBERSHIP)");
        close(sock);
        return -1;
    }

    printf("Subscribed to multicast group %s on port %d. Ready to receive data.\n",
           config->mc_group, config->mc_port);

    return sock;
}