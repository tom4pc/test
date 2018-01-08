#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <wait.h>

typedef struct sockaddr SA;

void str_echo(int sockfd);

void err_sys(const char *str) {
    printf("%s\n", str);
    exit(-1);
}

void sig_child(int signo) {
    pid_t pid;
    int stat;
    printf("in to sig fun\n");
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        printf("child %d terminated\n", pid);
    }
    printf("return..\n");
    return;
}

int main(int argc, char *argv[]) {
    int listenfd, connfd; // 监听套接字和已连接套接字
    char ipadd[90];       //用于存储解析后的IP地址字符串

    // pid_t childpid;
    socklen_t clilen;

    struct sockaddr_in cliaddr, servaddr;
    printf("create socket...\n");
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("fail to create socket. %s", strerror(errno));
        exit(-2);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(6666);

    printf("bind...\n");
    if (bind(listenfd, (SA *) &servaddr, sizeof(servaddr)) < 0) {
        printf("fail to bind. %s", strerror(errno));
        exit(-5);
    }

    printf("listen...\n");
    if (listen(listenfd, 5) < 0) {
        printf("error\n");
        exit(-3);
    };

    void sig_child(int);
    signal(SIGCHLD, sig_child);  //子进程结束的waitpid处理

    for (;;) {
        clilen = sizeof(cliaddr);
        if ((connfd = accept(listenfd, (SA *) &cliaddr, &clilen)) < 0) {
            if (errno == EINTR)  //处理accept的中断
                continue;
        }
        inet_ntop(AF_INET, &cliaddr.sin_addr, ipadd, clilen);
        printf("get from %s:%d\n", ipadd, ntohs(cliaddr.sin_port));
        if (fork() == 0) { //如果发现是子进程
            close(listenfd);
            str_echo(connfd);
            exit(0);
        }
        close(connfd);
    }
}

void str_echo(int sockfd) {
    ssize_t n;
    char buf[101];
    printf("into str_echo\n");
    again:
    while ((n = read(sockfd, buf, 100)) > 0)
        write(sockfd, buf, n);
    if (n < 0 && errno == EINTR) //处理中断
        goto again;
    else if (n < 0)
        err_sys("str_echo:read err");
}