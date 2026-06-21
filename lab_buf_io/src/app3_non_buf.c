#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

void print_stat(void)
{
    struct stat st;
    if (stat("3.txt", &st) == -1) {
        perror("stat");
        return;
    }
    printf("umode  : %o\n",  st.st_mode);
    printf("inode  : %lu\n", (unsigned long)st.st_ino);
    printf("size   : %lld\n", (long long)st.st_size);
    printf("blksize: %ld\n", (long)st.st_blksize);
}

int main(void)
{
    int fd1 = open("3.txt", O_RDWR);
    int fd2 = open("3.txt", O_RDWR);
    print_stat();
    int curr = 0;
    for (char c = 'a'; c <= 'z'; c++)
    {
        if (c % 2)
        {
            write(fd1, &c, 1);
            print_stat();
        }
        else
        {
            write(fd2, &c, 1);
            print_stat();
        }
    }
    close(fd1);
    print_stat();
    close(fd2);
    print_stat();
    return 0;
}