#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>



#define DEFAULT_PORT (unsigned short) 7341
#define LISTENQUE 10
#define MAXSIZE 1024

/**
 * linux 环境下实现echo服务器  本代码实现并发式的服务器
 * @author superliuliuliu1
 * @version 1.0
 * @date 2018/10/17
 */

 void error(char* message);
 void transport(int connfd);
 static void sig_child(int signo);


int main(int argc, char **argv)
{
    int sockfd, connfd;
    struct sockaddr_in server, client;
    unsigned short port = DEFAULT_PORT;
    pid_t pid1;
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
        error("Error on bind");
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
        signal(SIGCHLD, sig_child);
        pid1 = fork();
        switch(pid1)
        {
            case -1:
            {
               close(connfd);
               perror("Error on fork");
               break;
            }
            case 0:
            {
                printf("此时通信的客户端IP地址：%s\n", inet_ntoa(client.sin_addr));
                transport(connfd);
            }
            default:
            {
                close(connfd);
                break;
            }
        }

    }
    //close(sockfd);
    return 0;
}

/**
 * 传输函数   子进程调用该函数来进行与客户端的通信  并且通过调用exit来结束子进程
 * @author superliuliuliu1
 * @version [version]
 * @param   connfd    [description]
 */
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
        printf("Client put nothing");
        fflush(stdout);
    }
    else if(recv_size == -1)
    {
        error("Error on recv");
    }
    close(connfd);
    exit(0);

}


void error(char* message)
{
    perror(message);
    exit(1);
}

// WNOHANG 如果pid指定的子进程没有结束，则waitpid()函数立即返回0，而不是阻塞在这个函数上等待；如果结束了，则返回该子进程的进程号
static void sig_child(int signo)
{
    pid_t pid;
    int stat;

    //处理僵尸进程
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
      printf("child %d terminated.\n", pid);

}
