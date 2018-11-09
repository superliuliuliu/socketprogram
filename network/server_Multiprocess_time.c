#include <stdio.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>


#define DEFAULT_PORT (unsigned short) 7341
#define LISTENQUE 10
#define MAXSIZE 1024

void error(char *message);
static void sig_child(int signo);
void timenow(int connfd);

/**
 * 多进程时间服务器，能够向多台客服机返回当前时间  在本程序之中，当有连接出现后，服务器接收客户端发来的信息并判断，若收到是time则返回时间。
 * @author superliuliuliu1
 * @version 1.0
 */

int main(int argc, char **argv)
{
    int sockfd, connfd;
    struct sockaddr_in server, client;
    unsigned short port = DEFAULT_PORT;
    socklen_t len = sizeof(struct sockaddr_in);
    pid_t pid;

    if (argc > 2)
    {
        printf("Usage:  %s  [port]\n", argv[0]);
        printf("服务器端的默认开放端口号为7341,如果你已经设置请忽略！\n");
        error("Error on input");
    }

    if (argc > 1 ) {
        port = (unsigned short)atoi(argv[1]);
    }


    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("Error on socket");
    }
    //初始化server信息
    memset(&server, 0, sizeof(server));
    server.sin_port = htons(port);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    //bind 函数 只有当绑定成功时 返回值为0
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
            perror("accept");
            continue;
        }
        //捕捉信号
        signal(SIGCHLD, sig_child);
        //创建子进程来处理客户端服务请求
        pid = fork();
        switch (pid)
        {
             //fork失败
            case -1:
            {
                close(connfd);
                perror("Error on fork");
                break;
            }
            //进入到子进程中，开始对请求服务
            case 0:
            {
                printf("此时通信的客户端IP地址：%s\n", inet_ntoa(client.sin_addr));
                timenow(connfd);
            }
            //进入到父进程中，
            default:
            {
                close(connfd);
                break;
            }
        }
    }
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
 * 用来捕获子进程退出的消息SIGCHLD的信号处理函数
 * @author  superliuliuliu1
 * @version          1.0
 * @lastupdate       2018/11/04
 * @param      signo
 *
 */
static void sig_child(int signo)
{
       pid_t pid;
       int stat;
       //处理僵尸进程
       while ((pid = waitpid(-1, &stat, WNOHANG)) >0)
       {
           printf("child %d terminated.\n", pid);
       }

}

/**
 * 接受客户端发来的数据，并将当前的系统时间发送给客户端
 * @author superliuliuliu1
 * @version 1.0
 * @param   connfd    [description]
 */
void timenow(int connfd)
{
    char buffer[MAXSIZE];
    int recv_size;
    memset(buffer, 0, MAXSIZE);
    time_t ntime;
    char timestr[30];

    while ((recv_size = recv(connfd, buffer, MAXSIZE, 0)) > 0)
    {
        //接受客户端输入的命令并辨别  若收到time命令则将时间封装到buffer中，并发送出去
        if (strcmp(buffer, "time") == 0)
        {
            printf("Receive time request\n");
            ntime = time(NULL);
            //将时间转换为本地时间
            struct tm* tm_struct = localtime(&ntime);
            memset(timestr, 0, 30);
            strftime(timestr, 30, "%Y/%m/%d  %H:%M", tm_struct);
            printf("%s\n", timestr);
            //printf("%s\n", ctime(&ntime));
            //snprintf(buffer, sizeof(buffer), "%.24s\r", ctime(&ntime));
            send(connfd , timestr, sizeof(timestr), 0);
            memset(buffer, 0, MAXSIZE);
        }
        //else do nothing
    }

    if(recv_size <= 0)
    {
        printf("Client disconnected");
        fflush(stdout);
    }
    close(connfd);
    exit(0);
}
