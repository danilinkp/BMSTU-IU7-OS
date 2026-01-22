struct REQUEST
{
    int ticket;
};

program BAKERY_PROG
{
    version BAKERY_VER
    {
        int GET_NUMBER(int) = 1;

        long BAKERY_PROC(struct REQUEST) = 2;
    } = 1;
} = 0x20000001;