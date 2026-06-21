#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define WRITER_COUNT 5
#define READER_COUNT 3
#define COUNT (WRITER_COUNT + READER_COUNT)

HANDLE can_read;
HANDLE can_write;
HANDLE bmutex;

LONG read_queue = 0;
LONG write_queue = 0;
LONG active_readers = 0;

char symbol = 'a' - 1;

//volatile int flag = 1;
volatile int cnt = 0;  

//BOOL sigHandler(DWORD signal) {
//    flag = 0;
//    return TRUE;
//}

void start_read() {
    InterlockedIncrement(&read_queue);
    if (write_queue > 0 || WaitForSingleObject(can_write, 0) != WAIT_OBJECT_0)
        WaitForSingleObject(can_read, INFINITE);

    WaitForSingleObject(bmutex, INFINITE);
    active_readers++;
    InterlockedDecrement(&read_queue);
    ReleaseMutex(bmutex);
}

void stop_read() {
    active_readers--;
    if (active_readers == 0) {
        SetEvent(can_write);
    }
}

void start_write() {
    InterlockedIncrement(&write_queue);
    if (active_readers > 0 || WaitForSingleObject(can_read, 0) != WAIT_OBJECT_0) {
        WaitForSingleObject(can_write, INFINITE);
    }
    InterlockedDecrement(&write_queue);
}

void stop_write() {
    ResetEvent(can_write);
    if (read_queue > 0) {
        SetEvent(can_read);
    }
    else {
        SetEvent(can_write);
    }
}

DWORD writer(LPVOID param) {
    srand(GetCurrentThreadId());
    while (cnt < 3) {
        start_write();
        if (symbol == 'z') {
            cnt++;

            if (cnt >= 3)
                GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, 0);

            symbol = 'a';
        }
        else
            symbol++;

        if (cnt < 3)
        {
            printf("%ld write %c\n", GetCurrentThreadId(), symbol);
            stop_write();
        }
    }

    printf("%ld --- finised\n", GetCurrentThreadId());

    return 0;
}

DWORD reader(LPVOID param) {

    while (cnt < 3) {
        start_read();
        printf("%ld read %c\n", GetCurrentThreadId(), symbol);
        stop_read();
    }

    printf("%ld --- finised\n", GetCurrentThreadId());

    return 0;
}

int main() {
    DWORD thread_ids[COUNT];
    HANDLE threads[COUNT];

    if ((can_read = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL) { 
        printf("event error\n");
        ExitProcess(1);
    }

    if ((can_write = CreateEvent(NULL, FALSE, TRUE, NULL)) == NULL) { 
        printf("event error\n");
        ExitProcess(1);
    }

    if ((bmutex = CreateMutex(NULL, FALSE, NULL)) == NULL) {
        printf("mutex error\n");
        ExitProcess(1);
    }

    //if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)sigHandler, TRUE)) {
    //    printf("handler error\n");
    //    ExitProcess(1);
    //}

    for (int i = 0; i < WRITER_COUNT; i++) {
        threads[i] = CreateThread(NULL, 0, writer, NULL, 0, &thread_ids[i]);
        if (threads[i] == NULL) {
            printf("thread error\n");
            ExitProcess(1);
        }
    }

    for (int i = WRITER_COUNT; i < COUNT; i++) {
        threads[i] = CreateThread(NULL, 0, reader, NULL, 0, &thread_ids[i]);
        if (threads[i] == NULL) {
            printf("thread error\n");
            ExitProcess(1);
        }
    }

    WaitForMultipleObjects(COUNT, threads, TRUE, INFINITE);

    for (int i = 0; i < COUNT; i++)
        CloseHandle(threads[i]);

    CloseHandle(can_read);
    CloseHandle(can_write);
    CloseHandle(bmutex);

    return 0;
}
