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


bool listall = false;
bool listlong = false;
bool listlink = false;


void traversal_dir(char *dir);
void error(char* message);
void print_info(file_detail info);
char* uid_2uname(uid_t u1);
char* gid_2gname(gid_t g1);


/**
 * 实现自己的ls命令  当前只支持查看当前目录下的文件的相关信息
 * @author superliuliuliu1
 * @version 1.0
 * @param   argc      [description]
 * @param   argv      [description]
 * @return            [description]
 */
int main(int argc, char **argv)
{
    int opt_arg;
    char *current_dir = ".";
    //获取输入的 -x参数确定命令
    //-a 代表显示所有的文件
    //-d 表示显示 . 和 ..
    //-f 表示显示符号链接指向文件的参数
    //无参数的话只显示ls命令下的文件信息
    while ((opt_arg = getopt(argc, argv, "adf")) != -1)
    {
        switch(opt_arg)
        {
            case 'a':
            {
                listall = true;
                break;
            }
            case 'd':
            {
                listlong = true;
                break;
            }
            case 'f':
            {
                listlink = true;
                break;
            }
            case '?':
            {
                printf("Unkown option!\n");
                printf("-a     列出目录下所有文件的相关信息\n");
                printf("-d     列出当前目录.和上级目录..\n");
                printf("-f     列出符号链接指向文件的文件属性\n");
                break;
            }
        }
    }
    traversal_dir(current_dir);
    return 1;
}

void traversal_dir(char *dir)
{
    DIR *dp;
    struct stat stat_file;
    struct dirent *entry;//目录的入口

    //打开目录
    if ((dp = opendir(dir)) == NULL)
    {
        error("Error on open the dir");
    }
    //遍历目录中的文件，并打印相关信息直至到达目录结尾
    while ((entry = readdir(dp)) != NULL)
    {
        //扩展功能 -d 即如果不输入-d 则会跳过. 和 .. 文件
        if (!listlong)
        {
          if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
          {
              continue;
          }
        }
        //扩展功能 -f
        //将文件的数据填充到stat_file中  当listlink为true时使用lstat(返回符号链接文件本身的属性)，否则使用stat函数(返回符号引用文件的属性)
        if (!listlink)
        {
            lstat(entry->d_name, &stat_file);
        }
        else
        {
            stat(entry->d_name, &stat_file);
        }
        if (!listall)
        {
            if (entry->d_name[0] == '.'& strcmp(".", entry->d_name) != 0 & strcmp("..", entry->d_name) != 0)
            {
                continue;
            }
        }

        file_detail info;
        memset(&info, 0, sizeof(info));
        //memset(info.authority, 0, 11);
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
   closedir(dp);

}


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
    if (p!=NULL)
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
    if (g!=NULL)
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
