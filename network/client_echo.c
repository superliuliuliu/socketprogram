#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>

#define DEFAULT_PORT  (unsigned short) 8978
#define MAXSIZE 1024



void error(char* message);
void echo(int sockfd,char* arg);

/**
 * linux 环境下实现echo客户程序  实现代码有两种一种迭代式的实现另一种并发式的实现
 * @author superliuliuliu1
 * @version 1.0
 * @date 2018/10/17
 */
int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in server;
    unsigned short port = DEFAULT_PORT;
    char* server_ip;
    char recv_buffer[MAXSIZE];
    char message[MAXSIZE];


    printf("欢迎您使用ECHO客户端！\n");
    printf("--------------------------------------------\n");
    if (argc < 2 || argc > 3)
    {
        printf("使用指南：\n");
		    printf("Usage:  %s [server ip] [port]\n", argv[0]);
		    printf("服务器端的默认开放端口号为7341,如果你已经设置请忽略！\n");
        error("Error on input");
	  }
    //设置端口号
    if(argc == 3)
    {
		    port = (unsigned short)atoi(argv[2]);
	  }

    server_ip = argv[1];

    //创建socket
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("Error open the socket!");
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(server_ip);

    //connect
    if ((connect(sockfd, (struct sockaddr*)&server, sizeof(server))) < 0)
    {
        error("Error on connect");
    }
    printf("please input words to echo server(input quit to exit the client)\n");
    while (true)
    {
        printf(">>");
        memset(recv_buffer, 0, MAXSIZE);
        if ((fgets(recv_buffer, sizeof(recv_buffer), stdin)) == NULL)
        {
            error("Error on input");
        }
        if (recv_buffer == NULL)
        {
            continue;
        }
        else if (strcmp(recv_buffer, "quit\n") == 0)
        {
            break;
        }
        else
        {
            printf(">>");
            echo(sockfd, recv_buffer);
        }
    }
    close(sockfd);
    printf("欢迎您下次继续使用！\n");
    return 0;

}

void echo(int sockfd,char* arg)
{
    char message[MAXSIZE];
    memset(message, 0 ,MAXSIZE);
    send(sockfd, arg, strlen(arg), 0);
    recv(sockfd, message, sizeof(message), 0);
    //printf("message send success! send %d bytes!", (int)strlen(arg));

    printf("%s", message);
    return;

}

void error(char* message)
{
    perror(message);
    exit(1);
}
