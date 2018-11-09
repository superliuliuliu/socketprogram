#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#define DEFAULT_PORT (unsigned short) 7341
#define LISTENQUE 10
#define MAXSIZE 1024

void error(char *message);
void* timenow(void *argv);

/**
 * 多线程时间服务器，能够向多台客户机返回服务器当前时间
 * @author superliuliuliu1
 * @version 1.0
 */

int main(int argc, char **argv)
{
    int sockfd, connfd;
    struct sockaddr_in server, client;
    unsigned short port = DEFAULT_PORT;
    socklen_t len = sizeof(struct sockaddr_in);
    pthread_t tid;
    int *deliever;

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
        printf("此时通信的客户端IP地址：%s\n", inet_ntoa(client.sin_addr));
        //因为线程创建函数是通过一个指针来传递参数，因此需要为其分配内存空间
        deliever = (int*)malloc(sizeof(int));
        *deliever = connfd;//将connfd传递给线程start函数
        if (pthread_create(&tid, NULL, timenow, deliever) != 0)
        {
            perror("Error on thread create");
        }
        //free(deliever);
    }
    pthread_exit(NULL);
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
 * 线程start函数
 * @author superliuliuliu1
 * @version 1.0
 * @param   connfd    [description]
 */
void* timenow(void* argv)
{
    char buffer[MAXSIZE];
    int recv_size;
    memset(buffer, 0, MAXSIZE);
    time_t tricks;
    char timen[30];
    int connfd = *((int*)argv);

    //设置在子线程终止时，自动释放其资源
    pthread_detach(pthread_self());

    while ((recv_size = recv(connfd, buffer, MAXSIZE, 0)) > 0)
    {
        //接受客户端输入的命令并辨别  若收到time命令则将时间封装到timen中，并发送出去
        if (strcmp(buffer, "time") == 0)
        {
            tricks = time(NULL);
            struct tm* tm_struct = localtime(&tricks);
            printf("Receive time request\n");
            memset(timen, 0, 30);
            strftime(timen, 30, "%Y/%m/%d  %H:%M", tm_struct);
            printf("%s\n", timen);
            //printf("%s\n", ctime(&tricks));
            //snprintf(buffer, sizeof(buffer), "%.24s\r", ctime(&tricks));
            send(connfd , timen, sizeof(timen), 0);
            //memset(timen, 0, 30);
            memset(buffer, 0, MAXSIZE);
        }
        //else do nothing
    }

    if(recv_size <= 0)
    {
        printf("Client disconnected");
        fflush(stdout);
    }
    free(argv);
    close(connfd);
    pthread_exit(NULL);
}
