#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

void print_stat(void)
{
	struct stat st;
	if (stat("3.txt", &st) == -1)
	{
		perror("stat");
		return;
	}
	printf("umode  : %o\n", st.st_mode);
	printf("inode  : %lu\n", (unsigned long)st.st_ino);
	printf("size   : %lld\n", (long long)st.st_size);
	printf("blksize: %ld\n", (long)st.st_blksize);
}

int main(void)
{
	FILE *fd1 = fopen("3.txt", "r+");
	FILE *fd2 = fopen("3.txt", "r+");
	print_stat();
	int curr = 0;
	for (char c = 'a'; c <= 'z'; c++)
	{
		if (c % 2)
		{
			fprintf(fd1, "%c", c);
			print_stat();
		}
		else
		{
			fprintf(fd2, "%c", c);
			print_stat();
		}
	}
	fclose(fd1);
	print_stat();
	fclose(fd2);
	print_stat();
	return 0;
}