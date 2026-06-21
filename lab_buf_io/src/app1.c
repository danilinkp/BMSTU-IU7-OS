#include <linux/fs.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int main()
{
	int fd = open("alphabet.txt", O_RDONLY);

	FILE *fs1 = fdopen(fd, "r");
	char buf1[20];
	setvbuf(fs1, buf1, _IOFBF, 20);

	FILE *fs2 = fdopen(fd, "r");
	char buf2[20];
	setvbuf(fs2, buf2, _IOFBF, 20);

	int rc1 = 1, rc2 = 1;

	while (rc1 == 1 || rc2 == 1)
	{
		char c;
		rc1 = fscanf(fs1, "%c", &c);

		if (rc1 == 1)
		{
			fprintf(stdout, "%c", c);
		}

		rc2 = fscanf(fs2, "%c", &c);

		if (rc2 == 1)
		{
			fprintf(stdout,"%c", c);
		}
	}

	printf("\n");
	return 0;
}
