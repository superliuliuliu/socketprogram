#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<dirent.h>
#include<sys/types.h>
#include<time.h>
#include<pwd.h>
#include<grp.h>
#include<stdbool.h>
#include<sys/wait.h>

//定义一个结构体 用来存储文件的相关信息
typedef struct file_info
{
    char authority[11];//文件的权限 包括文件类型 用户权限 用户组权限 其他用户权限
    char *u_name;//文件所属用户名
    char *g_name;//文件所属用户所在用户组名
    char *f_name;//文件名
    off_t f_size;//文件大小
    time_t *latest_time;//最近修改时间
    nlink_t link_num;//文件的硬连接数

}file_detail;

void traversal_dir(char *dir);
void error(char* message);
void print_info(file_detail info);
char* uid_2uname(uid_t u1);
char* gid_2gname(gid_t g1);
void fill_file_detail(struct stat stat_file, struct dirent *entry);
void docopy(struct dirent *entry1);

/**
 * 遍历当前目录，当文件是子目录之时，创建子进程进入目录执行之前编写的cp程序复制子目录下所有文件到/home/lgy中  主进程执行ls -l即列出目录下的文件
 * @author  superliuliuliu1
 * @version         1.0
 * @lastupdate      2018/11/7
 * @param
 * @param
 * @return
 */
int main(int argc, char **argv)
{
    char *current_dir = ".";
    traversal_dir(current_dir);
    return 1;
}

void traversal_dir(char *dir)
{
    DIR *dp;
    struct stat stat_file;
    struct dirent *entry;//目录的入口
    pid_t pid;

    //打开目录
    if ((dp = opendir(dir)) == NULL)
    {
        error("Error on open the dir");
    }
    //遍历目录中的文件，并打印相关信息直至到达目录结尾
    while ((entry = readdir(dp)) != NULL)
    {   //默认返回链接文件原文件的属性
        lstat(entry->d_name, &stat_file);
        //打印文件信息
        fill_file_detail(stat_file, entry);
        //判断文件是否是目录，如果不是则跳过
        if (!S_ISDIR(stat_file.st_mode))
        {
            continue;
        }
        if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
        {
            continue;
        }
        DIR* childdp;
        struct stat stat_file1;
        struct dirent* entry1;
        //打开目录  传进来的参数文件名为子目录文件名
        if ((childdp = opendir(entry->d_name)) == NULL)
        {
            error("Error on open the dir");
        }
        //遍历目录中的文件，并打印相关信息直至到达目录结尾，这里的每一个目录项为子目录里的每一个文件
        while ((entry1 = readdir(childdp)) != NULL)
        {
            //跳过子目录里的.和..文件
            if (strcmp(".", entry1->d_name) == 0 || strcmp("..", entry1->d_name) == 0)
            {
                continue;
            }
            char path[50];
            memset(path, 0, sizeof(path));
            getcwd(path, 50);
            char name[100];
            memset(name, 0, sizeof(name));
            sprintf(name, "%s/%s/%s", path, entry->d_name, entry1->d_name);

            char name2[100];
            memset(name2, 0, sizeof(name2));
            sprintf(name2, "/home/liugaoyang/%s", entry1->d_name);

            pid = fork();
            switch(pid)
            {
                case -1:
                {
                    error("Error on fork");//因为这里直接退出程序所以不用break
                }
                case 0:
                {
                    printf("子进程工作中....\n");//一个子进程只处理一个文件的复制工作
                    printf("copy file %s to /home/liugaoyang/ ...\n", entry1->d_name);
                    execl("/root/linux/copy", "copy", name, name2, NULL);
                }
                default:
                {
                    //wait()也可以，waitpid中第三个参数为0的话 代表主进程会阻塞直至子进程完成工作
                    //除此之外还有两个参数WNOHANG 代表如果pid指定的子进程还没有结束，则函数立即返回0而不是阻塞在这个函数上等待，如果结束了则返回子进程的进程号
                    //WUNTRACED 如果子进程进入暂停状态，马上返回
                    waitpid(pid, NULL, 0);
                    printf("子进程执行的操作已经完成\n");
                    break;
                }
            }
        }
        closedir(childdp);
    }
    closedir(dp);
}

/**
 * [print_info description]
 * @author  superliuliuliu1
 * @version         [version]
 * @lastupdate      {[date]}
 * @param      info [description]
 */
void print_info(file_detail info)
{
    struct tm* tm_struct = localtime(info.latest_time);
    char time[100];
    strftime(time, 100, "%h %e %H:%M", tm_struct);
    printf("%s ", info.authority);
    printf("%10s ", info.u_name);
    printf("%10s ", info.g_name);
    printf("%10ld ", info.f_size);
    printf("%s ", time);
    printf("%2ld ", info.link_num);
    printf("%s ", info.f_name);
    memset(time, 0, sizeof(time));
    printf("\n");
}

/**
 * print error message
 * @author superliuliuliu1
 * @version 1.0
 * @param   message   the pointer
 */
void error(char* message) {
    perror(message);
    exit(1);
}

/**
 * 根据uid查找用户名
 * @author superliuliuliu1
 * @version 1.0
 * @param   u1        [description]
 * @return            [description]
 */
char* uid_2uname(uid_t u1)
{
    struct passwd *p;
    p = (struct passwd*)malloc(sizeof(struct passwd));
    p = getpwuid(u1);
    if (p != NULL)
    {
        return p->pw_name;
    }
    else
    {
        char *s;
        s = (char*)malloc(sizeof(char)*5);
        sprintf(s, "%d", u1);
        return s;
    }
}

/**
 * 根据gid查找用户所属组名
 * @author superliuliuliu1
 * @version 1.0
 * @param   g1        [description]
 * @return            [description]
 */
char* gid_2gname(gid_t g1)
{
    struct group *g;
    g = (struct group*)malloc(sizeof(struct group));
    g = getgrgid(g1);
    if (g != NULL)
    {
        return g->gr_name;
    }
    else
    {
        char *s;
        s = (char*)malloc(sizeof(char)*5);
        sprintf(s, "%d", g1);
        return s;
    }
}

/**
 * 将文件的信息填充到文件结构体中，并打印相关信息
 * @author  superliuliuliu1
 * @version              1.0
 * @lastupdate           2018/11/7
 * @param      stat_file [description]
 */
void fill_file_detail(struct stat stat_file, struct dirent *entry)
{
    file_detail info;
    memset(&info, 0, sizeof(info));
    //将文件的类型以及权限信息转换为字符串
    info.authority[0] = '-';
    if (S_ISDIR(stat_file.st_mode))
    {
        info.authority[0] = 'd';
    }
    if (S_ISCHR(stat_file.st_mode))
    {
        info.authority[0] = 'r';
    }
    if (S_ISBLK(stat_file.st_mode))
    {
        info.authority[0] = 'b';
    }
    if (S_ISFIFO(stat_file.st_mode))
    {
        info.authority[0] = 'p';
    }
    if (S_ISLNK(stat_file.st_mode))
    {
        info.authority[0] = 'l';
    }
    if (S_ISSOCK(stat_file.st_mode))
    {
        info.authority[0] = 's';
    }
    //进行位运算从而确定权限
    info.authority[1] = (stat_file.st_mode & S_IRUSR) == S_IRUSR ? 'r': '-';
    info.authority[2] = (stat_file.st_mode & S_IWUSR) == S_IWUSR ? 'w': '-';
    info.authority[3] = (stat_file.st_mode & S_IXUSR) == S_IXUSR ? 'x': '-';

    info.authority[4] = (stat_file.st_mode & S_IRGRP) == S_IRGRP ? 'r': '-';
    info.authority[5] = (stat_file.st_mode & S_IWGRP) == S_IWGRP ? 'w': '-';
    info.authority[6] = (stat_file.st_mode & S_IXGRP) == S_IXGRP ? 'x': '-';

    info.authority[7] = (stat_file.st_mode & S_IROTH) == S_IROTH ? 'r': '-';
    info.authority[8] = (stat_file.st_mode & S_IWOTH) == S_IWOTH ? 'w': '-';
    info.authority[9] = (stat_file.st_mode & S_IXOTH) == S_IXOTH ? 'x': '-';
    info.authority[10] = '\0';

    info.f_name = entry->d_name;
    info.f_size = stat_file.st_size;
    info.latest_time = &(stat_file.st_mtime);
    info.link_num = stat_file.st_nlink;
    info.u_name = uid_2uname(stat_file.st_uid);
    info.g_name = gid_2gname(stat_file.st_gid);
    print_info(info);
}


/**
 * 对目录文件中文件进行copy操作
 * @author  superliuliuliu1
 * @version            [version]
 * @lastupdate         {[date]}
 * @param      catalog [description]
 */
void docopy(struct dirent *entry1)
{
    DIR* childdp;
    struct stat stat_file;
    struct dirent* entry;

    //打开目录  传进来的参数文件名为子目录文件名
    if ((childdp = opendir(entry1->d_name)) == NULL)
    {
        error("Error on open the dir");
    }
    //遍历目录中的文件，并打印相关信息直至到达目录结尾
    while ((entry = readdir(childdp)) != NULL)
    {
        if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
        {
            continue;
        }
        //lstat(entry->d_name, &stat_file);
        char path[50];
        memset(path, 0, sizeof(path));
        getcwd(path, 50);
        char name[100];
        memset(name, 0, sizeof(name));
        sprintf(name, "%s/%s/%s", path, entry1->d_name, entry->d_name);
        //printf("%s\n", name);

        char name2[100];
        memset(name2, 0, sizeof(name2));
        sprintf(name2, "/home/liugaoyang/%s", entry->d_name);

        printf("copy file %s to /home/liugaoyang/ ...\n", entry->d_name);
        execl("/root/linux/copy", "copy", name, name2, NULL);
        printf("exiting child process ----\n");
        printf("test");

    }

    closedir(childdp);
    exit(0);
}
