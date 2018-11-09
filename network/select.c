#include<time.h>
#include<sys/select.h>
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

//编写基于select的I/O多路复用并发time服务器
int main(int argc, char** argv)
{
    int sockfd, connfd;
    struct sockaddr_in client, server;
    unsigned short port = DEFAULT_PORT;
    socklen_t len = sizeof(struct sockaddr_in);
    int reuse = 1;//used in setsockopt()

    fd_set readfds;//文件描述符集合
    struct timeval timeout;//设置select的阻塞时间
    int fdset[MAX_FD];


    if (argc > 2)
    {
        printf("Usage:  %s  [port]\n", argv[0]);
        printf("服务器端的默认开放端口号为7341,如果你已经设置请忽略！\n");
        error("Error on input");
    }
    if (argc > 1) {
        port = (unsigned short)atoi(argv[1]);
    }

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

    //fdset数组中的元素全部设置为-1
    memset(fdset, 0xff, sizeof(fdset));
    //设置select函数的阻塞时间
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    int ready;//select函数的返回值
    int i;
    int maxfd = 0;//为了设置的循环检测范围，该数值应该比三个文件描述符集合的值还要大1，该参数用在select之中，使select更加有效率
    while (true)
    {
        FD_ZERO(&readfds);
        //首先将监听sockfd加入到readfds中
        FD_SET(sockfd, &readfds);
        maxfd = sockfd;
        //检测
        for (i = 0; i < MAX_FD; i++)
        {
            if (fdset[i] != -1)
            {
                if (i > maxfd)
                {
                    maxfd = i;
                }
                FD_SET(i, &readfds);
            }
        }

        ready = select(maxfd + 1, &readfds, NULL, NULL, &timeout);
        if (ready == -1)
        {
            //EINTR错误代表该调用被信号处理历程给中断了，select不会自动恢复，因此需要我们加以操作
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
        else //代表select执行成功
        {
            if (FD_ISSET(sockfd, &readfds))
            {
                memset(&client, 0, sizeof(client));
                connfd = accept(sockfd, (struct sockaddr*)&client, &len);//接收客户端的连接
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
                    fdset[connfd] = 0;
                    if (connfd > maxfd)
                    {
                        maxfd = connfd;
                    }
                }
            }
        }
        for (i = 0; i < MAX_FD; i++)
        {
            if (fdset[i]!=-1 && FD_ISSET(i, &readfds))
            {
                if (timenow(i) <= 0)
                {
                    fdset[i] = -1;
                    close(i);
                    FD_CLR(i, &readfds);
                }
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

        //else do nothing
    }

    return recv_size;
    /*if (recv_size < 0)
    {
        error("Error on recv");
    }
    if(recv_size == 0)
    {
        printf("Client disconnected");
        fflush(stdout);
    }
    close(connfd);*/
}
