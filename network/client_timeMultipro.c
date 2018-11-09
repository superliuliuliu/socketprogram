#include<stdio.h>
#include<sys/socket.h>
#include<stdbool.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/ioctl.h>
#include<arpa/inet.h>
#include<signal.h>
#include<sys/wait.h>

#define DEFAULT_PORT (unsigned short) 7341
#define COMMAND 512

//并发多进程客户端  需要实现可指定进程数的客户端
static void sig_child(int signo);
void error(char* message);
void trans(int connfd);

/**
 * 多进程客户端， 能够指定生成的进程数，并发的向服务器端发送time命令，而不再采取从命令行获取time命令的形式
 * @author superliuliuliu1
 * @version 2.0
 */
 int main(int argc, char** argv)
 {
     int sockfd;
     struct sockaddr_in server;
     unsigned short port = DEFAULT_PORT;
     char* server_ip;
     pid_t pid;
     int multi_number;

     printf("欢迎您使用TIME多进程客户端模拟测试并发程序！\n");
     printf("--------------------------------------------\n");
     if (argc != 4)
     {
         printf("使用指南：\n");
         printf("Usage:  %s [server ip] [port] [the number of process you want to test]\n", argv[0]);
         printf("服务器端的默认开放端口号为7341,如果你已经设置请忽略！\n");
         error("Error on input");
     }

     port = (unsigned short)atoi(argv[2]);
     multi_number = atoi(argv[3]);
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

     printf("本程序为模拟多个客户端向并发服务器发送命令过程因此您不需要输入任何内容！\n");
     int i;
     signal(SIGCHLD, sig_child);
     for (i = 0; i < multi_number; i++)
     {
         pid = fork();
         switch (pid)
         {
             case -1:
             {
                 close(sockfd);
                 error("Error on fork");
             }
             case 0:
             {
                 trans(sockfd);
                 break;
             }
             default:
             {
                 waitpid(pid, NULL, 0);
                 break;
             }
         }
     }
     close(sockfd);
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
  * @param   connfd    socket文件描述符
  */
void trans(int connfd)
 {
     char message[COMMAND];
     memset(message, 0 ,COMMAND);
     send(connfd, "time", 4, 0);
     recv(connfd, message, sizeof(message), 0);
     printf("%s\n", message);
     exit(0);//退出子进程
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
