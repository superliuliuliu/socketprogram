#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<stdlib.h>

#define BUFFSIZE 1024

void error(char* message);

int main(int argc, char** argv)
{
    int sourcefd, destfd;
    struct stat s_buff;//stat结构体用来存储文件的相关属性信息
    char command[5];
    ssize_t numread;
    char buffer[BUFFSIZE];

    if (argc != 3 || strcmp(argv[1], "--help") == 0)
    {
        printf("使用指南\n");
        printf("Usage: %s [source file path] [dest file path]\n", argv[0]);
        exit(0);
    }
    //判断输入的源文件是否是目录
    stat(argv[1], &s_buff);
    if (S_ISDIR(s_buff.st_mode))
    {
        error("The source file is a directionry");
    }
    //对应源文件不存在的情况
    sourcefd = open(argv[1], O_RDONLY);
    if (sourcefd == -1)
    {
        error("Error on find the source file");
    }

    //判断目标文件是否存在，然后做出选择
    if (access(argv[2], F_OK) == 0)
    {
        printf("目标文件已经存在，输入y去拼接文件（默认直接覆盖文件）\n");
        if ((fgets(command, sizeof(command), stdin)) == NULL)
        {
            error("Error on input");
        }

        if (strcmp(command, "y\n") == 0)
        {
            //以合并的方式打开文件
            destfd = open(argv[2], O_WRONLY | O_APPEND);
            printf("将以合并的形式对文件操作\n");
            if (destfd == -1)
            {
                error("Error on output");
            }
            lseek(destfd, 0, SEEK_END);//改变文件的偏移量至文件尾部，已达到拼接的目的
        }
        else
        {
            //以覆盖的方式打开文件
            destfd = open(argv[2], O_WRONLY | O_TRUNC);
            if (destfd == -1)
            {
                error("Error on output");
            }
        }
    }
    else
    {
        destfd = open(argv[2], O_WRONLY | O_CREAT | O_EXCL | O_TRUNC);
        if (destfd == -1)
        {
            error("Error on output");
        }
    }

    //初始化buffer
    memset(buffer, 0, BUFFSIZE);

    while ((numread = read(sourcefd, buffer, BUFFSIZE)) > 0 )
    {
        if (write(destfd, buffer, numread) != numread)
        {
            error("Error on write");
        }
    }
    close(sourcefd);
    close(destfd);
}

/**
 * error
 * @author superliuliuliu1
 * @version 1.0
 * @param   message   the pointer
 */
void error(char *message)
{
    perror(message);
    exit(1);
}
