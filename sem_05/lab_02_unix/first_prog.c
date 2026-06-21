#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>

uint32_t cyclic_shift_left(uint32_t a, int n)
{
    return (a << n) | (a >> (32 - n));
}

void print_bin(uint32_t num)
{
    for (int i = 32 - 1; i >= 0; i--)
        printf("%d", (num >> i) & 1);
    printf("\n");
}

int main(void)
{
    uint32_t a;
    int n;

    printf("(PID = %d) Введите число a: ", getpid());
    if (scanf("%" SCNu32, &a) != 1)
    {
        printf("(PID: %d) Ошибка ввода a\n", getpid());
        return 1;
    }

    printf("(PID = %d) Введите n: ", getpid());
    if (scanf("%d", &n) != 1 || n < 0)
    {
        return 1;
    }

    uint32_t result = cyclic_shift_left(a, n);

    printf("(PID = %d) Результат: ", getpid());
    print_bin(result);

    return 0;
}