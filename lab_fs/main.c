#include "apue.h"
#include <time.h>

typedef int Myfunc(const char *, int, int);
typedef int DoPath(Myfunc *, int *);

static Myfunc myfunc;
static int myftw_no(char *, Myfunc *, DoPath *);
static int myftw_chdir(char *, Myfunc *, DoPath *);

static int dopath_no(Myfunc *, int *);
static int dopath_chdir(Myfunc *, int *);
void compare(char *);

char print = 1;
int amount_realloc = 0;

int main(int argc, char *argv[])
{
    int ret;
    if (argc != 2)
        err_quit("Использование: ftw <начальный_каталог>");

    ret = myftw_chdir(argv[1], myfunc, dopath_chdir);

    compare(argv[1]);

    exit(ret);
}

#define FTW_F 1
#define FTW_D 2
#define FTW_DNR 3
#define FTW_NS 4

static char *fullpath;
static char *filename;
static size_t path_len;

static int myftw_no(char *pathname, Myfunc *func, DoPath *dopath)
{
    int len = PATH_MAX + 1;
    fullpath = path_alloc(&len);

    strcpy(fullpath, pathname);
    fullpath[len - 1] = 0;

    int level = 0;
    return (dopath(func, &level));
}

static int myftw_chdir(char *pathname, Myfunc *func, DoPath *dopath)
{
    int len = PATH_MAX + 1;
    filename = path_alloc(&len);

    strcpy(filename, pathname);
    filename[len - 1] = 0;

    int level = 0;
    return (dopath(func, &level));
}

static int dopath_no(Myfunc *func, int *level)
{
    struct stat statbuf;
    struct dirent *dirp;
    DIR *dp;
    int ret, n;

    if (lstat(fullpath, &statbuf) < 0)
        return func(fullpath, *level, FTW_NS);

    if (S_ISDIR(statbuf.st_mode) == 0)
        return func(fullpath, *level, FTW_F);

    if ((ret = func(fullpath, *level, FTW_D)) != 0)
        return ret;

    n = strlen(fullpath);

    fullpath[n++] = '/';
    fullpath[n] = 0;

    if ((dp = opendir(fullpath)) == NULL)
        return (func(fullpath, *level, FTW_DNR));

    int stop = 1;
    (*level)++;
    while ((dirp = readdir(dp)) != NULL && stop)
    {
        if (!(strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0))
        {
            strcpy(&fullpath[n], dirp->d_name);

            if ((ret = dopath_no(func, level)) != 0)
                stop = 0;
        }
    }
    (*level)--;

    fullpath[n - 1] = 0;

    if (closedir(dp) < 0)
        err_ret("невозможно закрыть каталог %s", fullpath);

    return (ret);
}

static int dopath_chdir(Myfunc *func, int *level)
{
    struct stat statbuf;
    struct dirent *dirp;
    DIR *dp;
    int ret, n;

    if (lstat(filename, &statbuf) < 0)
        return (func(filename, *level, FTW_NS));

    if (S_ISDIR(statbuf.st_mode) == 0)
        return (func(filename, *level, FTW_F));

    if ((ret = func(filename, *level, FTW_D)) != 0)
        return (ret);

    if ((dp = opendir(filename)) == NULL)
        return (func(filename, *level, FTW_DNR));

    if (chdir(filename) == -1)
        err_sys("не удалось изменить директорию");

    int stop = 1;
    (*level)++;
    while ((dirp = readdir(dp)) != NULL && stop)
    {
        if (!(strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0))
        {
            strcpy(&filename[0], dirp->d_name);

            if ((ret = dopath_chdir(func, level)) != 0)
                stop = 0;
        }
    }
    (*level)--;

    if (chdir("..") == -1)
        err_sys("не удалось изменить директорию");

    if (closedir(dp) < 0)
        err_ret("невозможно закрыть каталог %s", filename);

    return (ret);
}

static int myfunc(const char *pathname, int level, int type)
{
    if (print)
    {
        if (type == FTW_F || type == FTW_D)
        {
            const char *file_name;
            int i = 0;

            for (int i = 0; i < level; ++i)
            {
                if (i != level - 1)
                    printf("│   ");
                else
                    printf("└───");
            }

            if (level > 0)
            {
                file_name = strrchr(pathname, '/');

                if (file_name != NULL)
                    printf("%s\n", file_name + 1);
                else
                    printf("%s\n", pathname);
            }
            else
                printf("%s\n", pathname);
        }
        else if (type == FTW_DNR)
            err_ret("закрыт доступ к каталогу %s", pathname);
        else if (type == FTW_NS)
            err_ret("ошибка вызова функции stat для %s", pathname);
        else
            err_ret("неизвестный тип %d для файла %s", type, pathname);
    }

    return 0;
}

static long long getThreadCpuTimeNs()
{
    struct timespec t;

    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t))
    {
        perror("clock_gettime");
        return 0;
    }

    return t.tv_sec * 1000000000LL + t.tv_nsec;
}

void compare(char *pathname)
{
    print = 0;

    int start, end;
    double time_no_chdir, time_with_chdir;

    start = clock();
    myftw_no(pathname, myfunc, dopath_no);
    end = clock();

    time_no_chdir = (double)(end - start) * 1000 / CLOCKS_PER_SEC;
    printf("\nВремя обхода дерева каталогов без chdir: %f\n", time_no_chdir);

    start = clock();
    myftw_chdir(pathname, myfunc, dopath_chdir);
    end = clock();

    time_with_chdir = (double)(end - start) * 1000 / CLOCKS_PER_SEC;
    printf("Время обхода дерева каталогов с chdir:   %f\n", time_with_chdir);
    double percent = (time_no_chdir - time_with_chdir) / time_no_chdir * 100.0;
    printf("С chdir быстрее на: %.2f%%\n", percent);
}