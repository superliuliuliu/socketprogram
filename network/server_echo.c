#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>


#define DEFAULT_PORT (unsigned short) 7341
#define LISTENQUE 10
#define MAXSIZE 1024

/**
 * linux 环境下实现echo服务器  实现代码有两种一种迭代式的实现另一种并发式的实现
 * @author superliuliuliu1
 * @version 1.0
 * @date 2018/10/17
 */

 void error(char* message);
 void transport(int connfd);


int main(int argc, char **argv)
{
    int sockfd, connfd;
    struct sockaddr_in server, client;
    unsigned short port = DEFAULT_PORT;
    socklen_t len = sizeof(struct sockaddr_in);

    //若启动服务器时未输入端口号，则端口号设置为默认的端口号
    if(argc > 1){
		    port = (unsigned short)atoi(argv[1]);
	  }

    //创建socket
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("Error open the socket!");
    }

    //对server相关信息进行初始化  注意字节序问题
    memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind()函数的返回值为0时才表示绑定成功
    if ((bind(sockfd, (struct sockaddr*)&server, sizeof(server))) != 0)
    {
        error("Error on bind()");
    }

    // 允许LISTENQUE个连接在队列处等待
    if (listen(sockfd, LISTENQUE) < 0)
    {
        error("Error on listen");
    }

    while (true)
    {
        len = sizeof(client);
        if ((connfd = accept(sockfd, (struct sockaddr *)&client, &len)) < 0)
        {
            fprintf(stderr, "accept error!\n");
            continue;
        }
        printf("此时通信的客户端IP地址：%s\n", inet_ntoa(client.sin_addr));
        transport(connfd);
    }
    close(sockfd);
    return 0;
}

void transport(int connfd)
{
    char buffer[MAXSIZE];
    int recv_size;

    memset(buffer, 0, MAXSIZE);
    while((recv_size = recv(connfd, buffer, MAXSIZE, 0)) > 0)
    {
        write(connfd , buffer, recv_size);
        memset(buffer, 0, MAXSIZE);
    }

    if(recv_size == 0)
    {
        printf("Client disconnected");
        fflush(stdout);
    }
    else if(recv_size == -1)
    {
        error("Error on recv");
    }
    close(connfd);

}


void error(char* message)
{
    perror(message);
    exit(1);
}
