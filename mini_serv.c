#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

fd_set fds, rr, ww;

char r_buff[4200], w_buff[200000];

int max = 0, next_id = 0;

typedef struct client {

    int id;
    int len;
    char msg[200000];

} t_client;

t_client clients[128];

void err(void) {
    write(2, "Fatal error\n", 12);
    exit(1);
}

void send_it(int c) {
    for (int i = 0; i <= max; i++) {
        if (FD_ISSET(i, &ww) && i != c) {
            send(i, w_buff, strlen(w_buff), 0);
        }
    }
}

int main(int ac, char **av) {

    if (ac != 2) {
        write(2, "Wrong number of arguments\n", 26);
        exit(1);
    }

    bzero(&clients, sizeof(clients));
    FD_ZERO(&fds);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        err();

    max = sockfd;
    FD_SET(sockfd, &fds);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(2130706433);
    addr.sin_port = htons(atoi(av[1]));

    if ((bind(sockfd, (const struct sockaddr *)&addr, sizeof(addr))) < 0)
        err();

    if (listen(sockfd, 128) < 0)
        err();

    while(1) {
        
        rr = ww = fds;

        if (select( max + 1, &rr, &ww, NULL, NULL) < 0)
            continue;
        
        for (int fd = 0; fd <= max; fd++) {
            if (FD_ISSET(fd, &rr)) {
                if (fd == sockfd) {
                    int conn = accept(fd, NULL, NULL);
                    if (conn < 0)
                        continue;
                    if (max < conn)
                        max = conn;
                    clients[conn].id = next_id++;
                    clients[conn].len = 0;
                    FD_SET(conn, &fds);
                    sprintf(w_buff, "server: client %d just arrived\n", clients[conn].id);
                    send_it(conn);
                    break;
                } else {

                    int len = recv(fd, r_buff, 4100, 0);
                    if (len <= 0) {
                        sprintf(w_buff, "server: client %d just left\n", clients[fd].id);
                        send_it(fd);
                        FD_CLR(fd, &fds);
                        close(fd);
                        break;
                    } else {

                        for (int i = 0; i < len; i++) {
                            clients[fd].msg[clients[fd].len++] = r_buff[i];
                            if (r_buff[i] == '\n') {
                                clients[fd].msg[clients[fd].len] = '\0';
                                clients[fd].len = 0;
                                sprintf(w_buff, "client %d: %s", clients[fd].id, clients[fd].msg);
                                send_it(fd);
                            }   
                        }
                        break;
                    }
                }
            }
        }
    }
}



