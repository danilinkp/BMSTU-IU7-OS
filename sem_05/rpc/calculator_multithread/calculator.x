/*
 * filename: calculator.x
 * function: Define constants, non-standard data types and the calling process in remote calls
 */

const ADD = 0;
const SUB = 1;
const MUL = 2;
const DIV = 3;
const PROC_COUNT = 8;

struct CALCULATOR
{
    int op;
    float arg1;
    float arg2;
};

program CALCULATOR_PROG
{
    version CALCULATOR_VER
    {
        float CALCULATOR_PROC(struct CALCULATOR) = 1;
    } = 1; /* Версия интерфейса */
} = 0x20000001; /* Номер программы RPC */
