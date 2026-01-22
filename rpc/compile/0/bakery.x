const CLIENT_COUNT = 6;

struct BAKERY_RESULT
{
    int is_recieved_time;
    long time;
};

program BAKERY_PROG
{
    version BAKERY_VER
    {
        int GET_NUMBER() = 1;
        struct BAKERY_RESULT BAKERY_PROC(int) = 2;
    } = 1;
} = 0x20000001;