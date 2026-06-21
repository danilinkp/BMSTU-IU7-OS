#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"


void *launchThread(void *args)
{
    int rc2 = 1;
    FILE *fs = (FILE *)args;

    while (rc2 == 1)
    {
        char c;
        if ((rc2 = fscanf(fs, "%c\n", &c)) == 1)
        {
            fprintf(stdout, GREEN "%c", c);
            sleep(1);
        }
    }

    return NULL;
}


int main(void)
{
    int fd = open("alphabet.txt", O_RDONLY); // O_RDONLY - только на чтение.

    FILE *fs1 = fdopen(fd, "r");
    char buf1[20];
    setvbuf(fs1, buf1, _IOFBF, 20); // _IOFBF - блочная буферизация.

    FILE *fs2 = fdopen(fd, "r");
    char buf2[20];
    setvbuf(fs2, buf2, _IOFBF, 20);

    pthread_t thread;
    int rc = pthread_create(&thread, NULL, launchThread, (void *)fs2);

    int rc1 = 1;
    while (rc1 == 1)
    {
        char c;
        rc1 = fscanf(fs1, "%c\n", &c);

        if (rc1 == 1)
        {
            fprintf(stdout, RED "%c", c);
            sleep(1);
        }
    }

    pthread_join(thread, NULL);

    printf("\n");

    return 0;
}
