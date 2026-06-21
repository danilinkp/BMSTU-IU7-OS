#include <errno.h>
#include <syslog.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <pthread.h>

#define LOCKFILE "/var/run/daemon.pid"
#define CONFIGFILE "/etc/daemon.conf"
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

sigset_t mask;

int already_running(void)
{
    int fd;
    char buf[16];

    fd = open(LOCKFILE, O_CREAT | O_RDWR | O_EXCL, LOCKMODE);
    if (fd == -1)
    {
        syslog(LOG_ERR, "невозможно повторно запустить демона; %s: %s",
               LOCKFILE, strerror(errno));
        exit(1);
    }
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    if (fcntl(fd, F_SETLK, &fl) == -1)
    {
        if (errno == EACCES || errno == EAGAIN)
        {
            close(fd);
            return 1;
        }
        syslog(LOG_ERR, "fcntl %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }
    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);
    return 0;
}

void daemonize(const char *cmd)
{
    int i, fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;

    /*
     * Сбросить маску режима создания файла.
     */
    umask(0);

    /*
     * Получить максимально возможный номер дескриптора файла.
     */
    if (getrlimit(RLIMIT_NOFILE, &rl) == -1)
    {
        perror("getrlimit");
        printf("errno: %d\n", errno);
        exit(1);
    }

    /*
     * Стать лидером нового сеанса, чтобы утратить управляющий терминал.
     */
    if ((pid = fork()) == -1)
    {
        perror("fork");
        printf("errno: %d\n", errno);
        exit(1);
    }
    else if (pid > 0)
        exit(0);
    if (setsid() == -1)
    {
        perror("setsid");
        printf("errno: %d\n", errno);
        exit(1);
    }
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) == -1)
    {
        perror("sigaction");
        printf("errno: %d\n", errno);
        exit(1);
    }

    /*
     * Назначить корневой каталог текущим рабочим каталогом,
     * чтобы впоследствии можно было отмонтировать файловую систему.
     */
    if (chdir("/") == -1)
    {
        perror("chdir");
        printf("errno: %d\n", errno);
        exit(1);
    }

    /*
     * Закрыть все открытые файловые дескрипторы.
     */
    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    for (i = 0; i < rl.rlim_max; i++)
        close(i);

    /*
     * Присоединить файловые дескрипторы 0, 1 и 2 к /dev/null.
     */
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    /*
     * Инициализировать файл журнала.
     */
    openlog(cmd, LOG_CONS, LOG_DAEMON);
    if (fd0 != 0)
    {
        syslog(LOG_ERR, "stdin");
        exit(1);
    }
    if (fd1 != 1)
    {
        syslog(LOG_ERR, "stdout");
        exit(1);
    }
    if (fd2 != 2)
    {
        syslog(LOG_ERR, "stderr");
        exit(1);
    }
}

void reread(void)
{
    int fd;
    long uid;
    char uname[128];

    char buf[128];

    fd = open(CONFIGFILE, O_RDONLY);
    if (fd == -1)
    {
        syslog(LOG_ERR, "Невозможно открыть конфигурационный файл %s", CONFIGFILE);
        return;
    }

    ssize_t rbytes = read(fd, buf, 128 - 1);
    if (rbytes == -1)
    {
        syslog(LOG_ERR, "Невозможно прочитать конфигурационный файл %s", CONFIGFILE);
        close(fd);
        return;
    }

    buf[rbytes] = '\0';

    if (sscanf(buf, "%ld %s", &uid, uname) == 2)
        syslog(LOG_INFO, "UID: %ld, UNAME: %s", uid, uname);
    else
        syslog(LOG_ERR, "Невозможно прочитать конфигурационный файл %s", CONFIGFILE);

    close(fd);
}

void *thr_fn(void *arg)
{
    int err, signo;
    for (;;)
    {
        err = sigwait(&mask, &signo);
        if (err != 0)
        {
            syslog(LOG_ERR, "ошибка вызова функции sigwait");
            exit(1);
        }
        switch (signo)
        {
        case SIGHUP:
            syslog(LOG_INFO, "Чтение конфигурационного файла");
            reread();
            break;
        case SIGTERM:
            syslog(LOG_INFO, "получен сигнал SIGTERM; завершение потока");
            pthread_exit(NULL);
        default:
            syslog(LOG_INFO, "не получен нужный сигнал %d\n", signo);
        }
    }
}

int main(int argc, char *argv[])
{
    int err;
    pthread_t tid;
    char *cmd;
    struct sigaction sa;
    if ((cmd = strrchr(argv[0], '/')) == NULL)
        cmd = argv[0];
    else
        cmd++;

    /*
     * Перейти в режим демона.
     */
    daemonize(cmd);

    /*
     * Убедиться, что ранее не была запущена другая копия демона.
     */
    if (already_running())
    {
        syslog(LOG_ERR, "демон уже запущен");
        exit(1);
    }

    /*
     * Восстановить действие по умолчанию для сигнала SIGHUP и заблокировать все сигналы.
     */
    sa.sa_handler = SIG_DFL;
    if (sigemptyset(&sa.sa_mask) == -1)
    {
        syslog(LOG_ERR, "sigemptyset");
        exit(1);
    }
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) == -1)
    {
        printf("%s: невозможно восстановить действие SIG_DFL для SIGHUP\n", cmd);
        exit(1);
    }
    if (sigfillset(&mask) == -1)
    {
        syslog(LOG_ERR, "sigfillset");
        exit(1);
    }
    if ((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) == -1)
    {
        printf("ошибка выполнения операции SIG_BLOCK: %s\n", strerror(err));
        exit(1);
    }

    /*

     * Создать поток для обработки SIGHUP и SIGTERM.
     */
    err = pthread_create(&tid, NULL, thr_fn, 0);
    if (err == -1)
    {
        printf("невозможно создать поток: %s", strerror(err));
        exit(1);
    }
    /*
     * Остальная часть программы-демона.
     */
    time_t raw_time;
    struct tm *timeinfo;

    for (;;)
    {
        time(&raw_time);
        timeinfo = localtime(&raw_time);
        syslog(LOG_INFO, "текущее время: %s", asctime(timeinfo));
        sleep(5);
    }
}
