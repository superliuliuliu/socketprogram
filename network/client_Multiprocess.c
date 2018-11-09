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
 * 多进程客户端，接受服务器向其发送的时间，并打印在屏幕之上
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
     pid_t pid;


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
             signal(SIGCHLD, sig_child);
             pid = fork();

             switch(pid)
             {
                 case -1:
                 {
                     perror("Error on fork");
                     break;
                 }
                 case 0:
                 {
                     trans(sockfd);
                 }
                 default:
                 {
                     break;
                 }
             }
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
     exit(0);//退出子进程
 }

 // WNOHANG 如果pid指定的子进程没有结束，则waitpid()函数立即返回0，而不是阻塞在这个函数上等待；如果结束了，则返回该子进程的进程号
static void sig_child(int signo)
{
       pid_t        pid;
       int        stat;
       //处理僵尸进程
       while ((pid = waitpid(-1, &stat, WNOHANG)) >0)
              printf("child %d terminated.\n>>", pid);
}
