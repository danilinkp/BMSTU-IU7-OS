program BAKERY_PROG
{
    version BAKERY_VER
    {
        void REGISTER_PROC(int) = 1;
        int GET_NUMBER_PROC(int) = 2;
        long RESULT_PROC(int) = 3;
    } = 1;
} = 0x20000001;