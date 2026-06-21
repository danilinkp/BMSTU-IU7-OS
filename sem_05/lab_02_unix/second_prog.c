#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

int input_array(int a[], size_t n)
{
    for (size_t i = 0; i < n; i++)
        if (scanf("%d", &a[i]) != 1)
        {
            printf("(PID = %d) Ошибка ввода элементов", getpid());
            return 1;
        }
    return 0;
}

void print_array(int a[], size_t n)
{
    for (size_t i = 0; i < n; i++)
        printf("%d ", a[i]);
    printf("\n");
}

int is_full_square(int number)
{
    if ((int)sqrt(number) * (int)sqrt(number) == number)
        return 1;
    return 0;
}

size_t remove_full_square(int a[], size_t alen)
{
    size_t c = 0;
    for (size_t i = 0; i < alen; i++)
        if (is_full_square(a[i]))
            c++;
        else
            a[i - c] = a[i];
    return alen - c;
}

int main(void)
{
    size_t alen;
    int a[10];

    printf("(PID = %d) Введите размер массива: ", getpid());
    if (scanf("%zu", &alen) != 1 || alen > 10)
    {
        return 1;
    }
    printf("(PID = %d) Введите элементы массива: ", getpid());
    if (input_array(a, alen) != 0)
        return 1;

    size_t size = remove_full_square(a, alen);
    if (size < 1)
    {
        return 1;
    }

    printf("(PID = %d) Result: ", getpid());
    print_array(a, size);
    return 0;
}