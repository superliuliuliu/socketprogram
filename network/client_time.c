#include<stdio.h>
#include<sys/socket.h>
#include<stdbool.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#define DEFAULT_PORT (unsigned short) 7341
#define COMMAND 512

void error(char *message);
void trans(int connfd);

/**
 * 单进程客户端，接受服务器向其发送的时间，并打印在屏幕之上
 * @author superliuliuliu1
 * @version 1.0
 */
int main(int argc, char** argv)
{
    int sockfd;
    struct sockaddr_in server;
    unsigned short port = DEFAULT_PORT;
    char* server_ip;
    char command[COMMAND];


    printf("欢迎您使用TIME客户端！\n");
    printf("--------------------------------------------\n");
    if (argc < 2 || argc > 3)
    {
        printf("使用指南：\n");
        printf("Usage:  %s [server ip] [port]\n", argv[0]);
        printf("服务器端的默认开放端口号为7341,如果你已经设置请忽略！\n");
        error("Error on input");
    }

    if (argc == 3)
    {
        port = (unsigned short)atoi(argv[2]);
    }

    server_ip = argv[1];

    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("Error on socket");
    }


    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(server_ip);


    //客户端与服务器端相连接
    if ((connect(sockfd, (struct sockaddr*)&server, sizeof(server))) < 0)
    {
        error("Error on connect");
    }

    printf("Begin to get the time(input the time to get the server time or input quit to exit the client)\n");

    while (true)
    {
        printf(">>");
        memset(command, 0, COMMAND);
        fgets(command, sizeof(command), stdin);
        if (command == NULL)
        {
            continue;
        }
        else if (strcmp(command, "quit\n") == 0)
        {
            break;
        }
        else if (strcmp(command, "time\n") == 0 )
        {
            //
            trans(sockfd);
        }
        else
        {
            printf("I do not understand what you input, please try again!\n");
        }

    }
    close(sockfd);
    printf("欢迎您下次继续使用！\n");
    return 0;

}

/**
 * 错误处理函数：遇到错误退出程序，并输出相关的错误信息
 * @author superliuliuliu1
 * @version 1.0
 * @param   message   错误提示信息
 */
void error(char *message)
{
    perror(message);
    exit(1);
}

/**
 * 客户端发送接受数据
 * @author superliuliuliu1
 * @version 1.0
 * @param   connfd    [description]
 */
void trans(int connfd)
{
    char message[COMMAND];
    memset(message, 0 ,COMMAND);
    send(connfd, "time", 4, 0);
    recv(connfd, message, sizeof(message), 0);
    printf("%s\n", message);
}
