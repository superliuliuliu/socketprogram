#include<time.h>
#include<poll.h>
#include<sys/socket.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<stdio.h>
#include<stdbool.h>
#include<arpa/inet.h>
#include<sys/ioctl.h>


#define DEFAULT_PORT (unsigned short) 7341
#define LISTENQUE 10
#define MAXSIZE 1024
#define MAX_FD 1024

void error(char *message);
int timenow(int connfd);
static int setsocket(int port);

//编写基于poll的I/O多路复用并发time服务器
int main(int argc, char** argv)
{
    int sockfd, connfd;
    struct sockaddr_in client;
    socklen_t len = sizeof(struct sockaddr_in);
    unsigned short port = DEFAULT_PORT;
    struct pollfd fdset[MAX_FD];
    int maxfd = 0;
    int i;//循环变量
    int ready;//the number poll return
    if (argc > 2)
    {
        printf("Usage:  %s  [port]\n", argv[0]);
        printf("服务器端的默认开放端口号为7341,如果你已经设置请忽略！\n");
        error("Error on input");
    }
    if (argc > 1) {
        port = (unsigned short)atoi(argv[1]);
    }
    //创建监听socket并与server绑定
    sockfd = setsocket(port);

    //初始化存储文件描述符信息的集合
    fdset[0].fd = sockfd;
    fdset[0].events = POLLIN;
    for (i= 1; i< MAX_FD; i++)
    {
        fdset[i].fd = -1;
    }

    while (true)
    {
        //将timeout设置为-1让poll阻塞到描述符集合中有可用的描述符之时
        ready = poll(fdset, maxfd + 1, -1);
        if (ready == -1)
        {
            //EINTR错误代表该调用被信号处理历程给中断了，poll不会自动恢复，因此需要我们加以操作
            if (errno == EINTR)
            {
                continue;
            }
            error("Error on select()");
        }
        else if (ready == 0)
        {
            continue;
        }
        else
        {   //检测监听描述符是否就绪，若就绪则进行下面的建立数据连接准备传输数据，否则continue
            if (fdset[0].revents & POLLIN)
            {
                memset(&client, 0, sizeof(client));
                connfd = accept(sockfd, (struct sockaddr*)&client, &len);
                if (connfd < 0)
                {
                    perror("error on accept()");
                    break;
                }
                else if (connfd > MAX_FD)
                {
                    perror("too many clients");
                    close(connfd);
                }
                else
                {
                    printf("此时通信的客户端IP地址：%s\n", inet_ntoa(client.sin_addr));
                    //将新的文件描述符添加到fdset之中  并更新maxfd
                    if (connfd > maxfd)
                    {
                        maxfd = connfd;
                    }
                    for (i = 1; i < MAX_FD; i++)
                    {
                        if (fdset[i].fd < 0)
                        {
                            fdset[i].fd = connfd;
                            fdset[i].events = POLLIN;
                            break;//找到一个空闲的赋值之后便跳出循环
                        }
                    }
                }

            }
        }
        for (i = 1; i < MAX_FD; i++)
        {
            if ((fdset[i].fd > 0) && (fdset[i].revents & POLLIN))
            {
                if (timenow(fdset[i].fd) <= 0)
                {
                    fdset[i].fd = -1;
                    close(fdset[i].fd);
                }
            }
        }
    }
    close(sockfd);
    return 0;
}

/**
 * 为了使main程序结构看的更加清晰，选择将创建socket、初始化server、bind的部分提取出来
 * @author  superliuliuliu1
 * @version         1.0
 * @lastupdate      2018/10/09
 * @return     sockfd
 */
static int setsocket(int port) {
    int sockfd;
    struct sockaddr_in server;
    int reuse = 1;//used in setsockopt()

    //对server进行初始化
    memset(&server, 0, sizeof(server));
    server.sin_port = htons(port);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    //创建监听socket
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("Error on socket()");
    }

    //设置套接字选项SO_REUSEADDR 目的在于让本地的端口号能够重用
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
    {
        close(sockfd);
        error("Error on setsockopt()");
    }

    //绑定sock
    if ((bind(sockfd, (struct sockaddr*)&server, sizeof(server))) != 0)
    {
        close(sockfd);
        error("Error on bind()");
    }

    //listen设定待连接的最大数目
    if (listen(sockfd, LISTENQUE) < 0)
    {
        close(sockfd);
        error("Error on listen()");
    }
    return sockfd;
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
 * 接受客户端发来的数据，并将当前的系统时间发送给客户端
 * @author superliuliuliu1
 * @version 1.0
 * @param   connfd    [description]
 */
int timenow(int connfd)
{
    char buffer[MAXSIZE];
    int recv_size;
    memset(buffer, 0, MAXSIZE);
    time_t ntime;
    char timestr[30];

    if ((recv_size = recv(connfd, buffer, MAXSIZE, 0)) > 0)
    {
        //接受客户端输入的命令并辨别  若收到time命令则将时间封装到buffer中，并发送出去
        if (strcmp(buffer, "time") == 0)
        {
            printf("Receive time request\n");
            ntime = time(NULL);
            struct tm* tm_struct = localtime(&ntime);
            memset(timestr, 0, 30);
            strftime(timestr, 30, "%Y/%m/%d  %H:%M", tm_struct);
            printf("%s\n", timestr);
            send(connfd , timestr, sizeof(timestr), 0);
            memset(buffer, 0, MAXSIZE);
        }
    }

    return recv_size;
}
