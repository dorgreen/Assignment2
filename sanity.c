//
// Created by Dor Green
//
#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"
#include "tournament_tree.h"
#include "kthread.h"

#define THREAD_SLEEP_THEN_EXIT(name, id) \
    void name(){ \
        sleep(500); \
        printf(1,"thread %d exiting\n", id ); \
        kthread_exit(); \
    }

#define THREAD_SLEEP_NO_EXIT(name, id) \
    void name(){ \
        sleep(500); \
        printf(1,"thread %d done sleeping..\n", id ); \
    }



#define THREAD_STACK(name) \
    void * name = ((char *) malloc(MAX_STACK_SIZE * sizeof(char)));



/// Test implementation of Threads

// try to create, run and k-thread-exit threads
int t_thread_create(){
    int pid;
    int THREAD_COUNT = 10;
    //int tids[THREAD_COUNT];


    THREAD_SLEEP_THEN_EXIT(threadStart_1, 1)
    THREAD_SLEEP_THEN_EXIT(threadStart_2, 2)
    THREAD_SLEEP_THEN_EXIT(threadStart_3, 3)
    THREAD_SLEEP_THEN_EXIT(threadStart_4, 4)
    THREAD_SLEEP_THEN_EXIT(threadStart_5, 5)
    THREAD_SLEEP_THEN_EXIT(threadStart_6, 6)
    THREAD_SLEEP_THEN_EXIT(threadStart_7, 7)
    THREAD_SLEEP_THEN_EXIT(threadStart_8, 8)
    THREAD_SLEEP_THEN_EXIT(threadStart_9, 9)
    THREAD_SLEEP_THEN_EXIT(threadStart_10, 10)

    THREAD_STACK(threadStack_1)
    THREAD_STACK(threadStack_2)
    THREAD_STACK(threadStack_3)
    THREAD_STACK(threadStack_4)
    THREAD_STACK(threadStack_5)
    THREAD_STACK(threadStack_6)
    THREAD_STACK(threadStack_7)
    THREAD_STACK(threadStack_8)
    THREAD_STACK(threadStack_9)
    THREAD_STACK(threadStack_10)

    void (*threads_stacks[])(void) =
            {threadStack_1,
             threadStack_2,
             threadStack_3,
             threadStack_4,
             threadStack_5,
             threadStack_6,
             threadStack_7,
             threadStack_8,
             threadStack_9,
             threadStack_10};

    void (*threads_starts[])(void) =
            {threadStart_1,
             threadStart_2,
             threadStart_3,
             threadStart_4,
             threadStart_5,
             threadStart_6,
             threadStart_7,
             threadStart_8,
             threadStart_9,
             threadStart_10};

    printf(1, "STARTING T_THREAD_CREATE TEST..\n");

    pid = fork();
    if(pid == 0){
        for(int i = 0;i < THREAD_COUNT;i++){
            kthread_create(threads_starts[i], threads_stacks[i]);
        }
        printf(1,"should create and exit %d threads\n", THREAD_COUNT-1 );
        sleep(10000);
        kthread_exit();
    }

    else{
        wait();
        printf(1,"t_thread_create done...\n");
        for(int i = 0; i < THREAD_COUNT ; i++){
            free(threads_stacks[i]);
        }
        return 0;
    }
    return -1;
}


// create threads, then perform an EXEC. makes sure they all die
int t_fork_with_threads(){
    int pid;
    int THREAD_COUNT = 10;
    //int tids[THREAD_COUNT];

    THREAD_SLEEP_THEN_EXIT(threadStart_1, 1)
    THREAD_SLEEP_THEN_EXIT(threadStart_2, 2)
    THREAD_SLEEP_THEN_EXIT(threadStart_3, 3)
    THREAD_SLEEP_THEN_EXIT(threadStart_4, 4)
    THREAD_SLEEP_THEN_EXIT(threadStart_5, 5)
    THREAD_SLEEP_THEN_EXIT(threadStart_6, 6)
    THREAD_SLEEP_THEN_EXIT(threadStart_7, 7)
    THREAD_SLEEP_THEN_EXIT(threadStart_8, 8)
    THREAD_SLEEP_THEN_EXIT(threadStart_9, 9)
    THREAD_SLEEP_THEN_EXIT(threadStart_10, 10)

    THREAD_STACK(threadStack_1)
    THREAD_STACK(threadStack_2)
    THREAD_STACK(threadStack_3)
    THREAD_STACK(threadStack_4)
    THREAD_STACK(threadStack_5)
    THREAD_STACK(threadStack_6)
    THREAD_STACK(threadStack_7)
    THREAD_STACK(threadStack_8)
    THREAD_STACK(threadStack_9)
    THREAD_STACK(threadStack_10)

    void (*threads_stacks[])(void) =
            {threadStack_1,
             threadStack_2,
             threadStack_3,
             threadStack_4,
             threadStack_5,
             threadStack_6,
             threadStack_7,
             threadStack_8,
             threadStack_9,
             threadStack_10};

    void (*threads_starts[])(void) =
            {threadStart_1,
             threadStart_2,
             threadStart_3,
             threadStart_4,
             threadStart_5,
             threadStart_6,
             threadStart_7,
             threadStart_8,
             threadStart_9,
             threadStart_10};

    printf(1,"STARTING FORK WITH THREADS TEST\n", THREAD_COUNT-1 );

    pid = fork();
    if(pid == 0){
        for(int i = 0;i < THREAD_COUNT;i++){
            kthread_create(threads_starts[i], threads_stacks[i]);
        }
        printf(1,"should create and kill threads before they print\n");
        char * command;
        char *args[4];

        args[0] = "/echo";
        args[1] = "killing other threads?";
        args[2] = 0;
        command = "/echo";
        exec(command,args);

    }

    else{
        wait();
        printf(1,"t_fork_with_threads done... shouldn't print the starting line of each\n");
        for(int i = 0; i < THREAD_COUNT ; i++){
            free(threads_stacks[i]);
        }
        return 0;
    }
    return -1;
}


// See that k_exit really closes the proc
int t_kexit_quit(){
    int pid;
    int THREAD_COUNT = 10;
    //int tids[THREAD_COUNT];

    THREAD_SLEEP_THEN_EXIT(threadStart_1, 1)
    THREAD_SLEEP_THEN_EXIT(threadStart_2, 2)
    THREAD_SLEEP_THEN_EXIT(threadStart_3, 3)
    THREAD_SLEEP_THEN_EXIT(threadStart_4, 4)
    THREAD_SLEEP_THEN_EXIT(threadStart_5, 5)
    THREAD_SLEEP_THEN_EXIT(threadStart_6, 6)
    THREAD_SLEEP_THEN_EXIT(threadStart_7, 7)
    THREAD_SLEEP_THEN_EXIT(threadStart_8, 8)
    THREAD_SLEEP_THEN_EXIT(threadStart_9, 9)
    THREAD_SLEEP_THEN_EXIT(threadStart_10, 10)

    THREAD_STACK(threadStack_1)
    THREAD_STACK(threadStack_2)
    THREAD_STACK(threadStack_3)
    THREAD_STACK(threadStack_4)
    THREAD_STACK(threadStack_5)
    THREAD_STACK(threadStack_6)
    THREAD_STACK(threadStack_7)
    THREAD_STACK(threadStack_8)
    THREAD_STACK(threadStack_9)
    THREAD_STACK(threadStack_10)

    void (*threads_stacks[])(void) =
            {threadStack_1,
             threadStack_2,
             threadStack_3,
             threadStack_4,
             threadStack_5,
             threadStack_6,
             threadStack_7,
             threadStack_8,
             threadStack_9,
             threadStack_10};

    void (*threads_starts[])(void) =
            {threadStart_1,
             threadStart_2,
             threadStart_3,
             threadStart_4,
             threadStart_5,
             threadStart_6,
             threadStart_7,
             threadStart_8,
             threadStart_9,
             threadStart_10};

    printf(1,"STARTING THREAD_K_EXIT TEST\n", THREAD_COUNT-1 );

    pid = fork();
    if(pid == 0){
        for(int i = 0;i < THREAD_COUNT;i++){
            kthread_create(threads_starts[i], threads_stacks[i]);
        }
        printf(1,"Created threads, now killing each with kthread_exit");
        kthread_exit();
    }

    else{
        wait();
        printf(1,"t_k_exit done... should successfully wait as all threads killed also killed the proc\n");
        for(int i = 0; i < THREAD_COUNT ; i++){
            free(threads_stacks[i]);
        }
        return 0;
    }
    return -1;
}


// fork, create many threads, exit without killing them
// should not finish the run on each thread
int t_exit(){
    int pid;
    int THREAD_COUNT = 10;
    //int tids[THREAD_COUNT];

    THREAD_SLEEP_NO_EXIT(threadStart_1, 1)
    THREAD_SLEEP_NO_EXIT(threadStart_2, 2)
    THREAD_SLEEP_NO_EXIT(threadStart_3, 3)
    THREAD_SLEEP_NO_EXIT(threadStart_4, 4)
    THREAD_SLEEP_NO_EXIT(threadStart_5, 5)
    THREAD_SLEEP_NO_EXIT(threadStart_6, 6)
    THREAD_SLEEP_NO_EXIT(threadStart_7, 7)
    THREAD_SLEEP_NO_EXIT(threadStart_8, 8)
    THREAD_SLEEP_NO_EXIT(threadStart_9, 9)
    THREAD_SLEEP_NO_EXIT(threadStart_10, 10)

    THREAD_STACK(threadStack_1)
    THREAD_STACK(threadStack_2)
    THREAD_STACK(threadStack_3)
    THREAD_STACK(threadStack_4)
    THREAD_STACK(threadStack_5)
    THREAD_STACK(threadStack_6)
    THREAD_STACK(threadStack_7)
    THREAD_STACK(threadStack_8)
    THREAD_STACK(threadStack_9)
    THREAD_STACK(threadStack_10)

    void (*threads_stacks[])(void) =
            {threadStack_1,
             threadStack_2,
             threadStack_3,
             threadStack_4,
             threadStack_5,
             threadStack_6,
             threadStack_7,
             threadStack_8,
             threadStack_9,
             threadStack_10};

    void (*threads_starts[])(void) =
            {threadStart_1,
             threadStart_2,
             threadStart_3,
             threadStart_4,
             threadStart_5,
             threadStart_6,
             threadStart_7,
             threadStart_8,
             threadStart_9,
             threadStart_10};

    printf(1,"STARTING T_EXIT TEST\n", THREAD_COUNT-1 );

    pid = fork();
    if(pid == 0){
        for(int i = 0;i < THREAD_COUNT;i++){
            kthread_create(threads_starts[i], threads_stacks[i]);
        }
        printf(1,"should create and kill threads before they print\n");
        exit();
    }

    else{
        wait();
        printf(1,"t_exit done... should successfully wait as all threads were killed on exit\n");
        for(int i = 0; i < THREAD_COUNT ; i++){
            free(threads_stacks[i]);
        }
        return 0;
    }
    return -1;
}


// see that we can join on created threads
int t_join(){
    int pid;
    int THREAD_COUNT = 10;
    int tids[THREAD_COUNT];
    int status = 0;


    THREAD_SLEEP_THEN_EXIT(threadStart_1, 1)
    THREAD_SLEEP_THEN_EXIT(threadStart_2, 2)
    THREAD_SLEEP_THEN_EXIT(threadStart_3, 3)
    THREAD_SLEEP_THEN_EXIT(threadStart_4, 4)
    THREAD_SLEEP_THEN_EXIT(threadStart_5, 5)
    THREAD_SLEEP_THEN_EXIT(threadStart_6, 6)
    THREAD_SLEEP_THEN_EXIT(threadStart_7, 7)
    THREAD_SLEEP_THEN_EXIT(threadStart_8, 8)
    THREAD_SLEEP_THEN_EXIT(threadStart_9, 9)
    THREAD_SLEEP_THEN_EXIT(threadStart_10, 10)

    THREAD_STACK(threadStack_1)
    THREAD_STACK(threadStack_2)
    THREAD_STACK(threadStack_3)
    THREAD_STACK(threadStack_4)
    THREAD_STACK(threadStack_5)
    THREAD_STACK(threadStack_6)
    THREAD_STACK(threadStack_7)
    THREAD_STACK(threadStack_8)
    THREAD_STACK(threadStack_9)
    THREAD_STACK(threadStack_10)

    void (*threads_stacks[])(void) =
            {threadStack_1,
             threadStack_2,
             threadStack_3,
             threadStack_4,
             threadStack_5,
             threadStack_6,
             threadStack_7,
             threadStack_8,
             threadStack_9,
             threadStack_10};

    void (*threads_starts[])(void) =
            {threadStart_1,
             threadStart_2,
             threadStart_3,
             threadStart_4,
             threadStart_5,
             threadStart_6,
             threadStart_7,
             threadStart_8,
             threadStart_9,
             threadStart_10};

    printf(1,"STARTING THREADS JOIN TEST\n", THREAD_COUNT-1 );

    pid = fork();
    if(pid == 0){
        for(int i = 0;i < THREAD_COUNT;i++){
            tids[i] = kthread_create(threads_starts[i], threads_stacks[i]);
        }
        printf(1,"Created all threads, now join them..\n");

        for(int i = 0;i < THREAD_COUNT;i++){
            status = kthread_join(tids[i]);
            printf(1, "joined thread %d with status %d\n", tids[i], status);
        }
        exit();


    }

    else{
        wait();
        printf(1,"t_join done... should succesfully join all threads\n");
        for(int i = 0; i < THREAD_COUNT ; i++){
            free(threads_stacks[i]);
        }
        return 0;
    }
    return -1;
}


// see that we can't join an already-joined thread
int t_join_neg(){
        int pid;
        int THREAD_COUNT = 10;
        int tids[THREAD_COUNT];
        int status = 0;

        THREAD_SLEEP_THEN_EXIT(threadStart_1, 1)
        THREAD_SLEEP_THEN_EXIT(threadStart_2, 2)
        THREAD_SLEEP_THEN_EXIT(threadStart_3, 3)
        THREAD_SLEEP_THEN_EXIT(threadStart_4, 4)
        THREAD_SLEEP_THEN_EXIT(threadStart_5, 5)
        THREAD_SLEEP_THEN_EXIT(threadStart_6, 6)
        THREAD_SLEEP_THEN_EXIT(threadStart_7, 7)
        THREAD_SLEEP_THEN_EXIT(threadStart_8, 8)
        THREAD_SLEEP_THEN_EXIT(threadStart_9, 9)
        THREAD_SLEEP_THEN_EXIT(threadStart_10, 10)

        THREAD_STACK(threadStack_1)
        THREAD_STACK(threadStack_2)
        THREAD_STACK(threadStack_3)
        THREAD_STACK(threadStack_4)
        THREAD_STACK(threadStack_5)
        THREAD_STACK(threadStack_6)
        THREAD_STACK(threadStack_7)
        THREAD_STACK(threadStack_8)
        THREAD_STACK(threadStack_9)
        THREAD_STACK(threadStack_10)

        void (*threads_stacks[])(void) =
                {threadStack_1,
                 threadStack_2,
                 threadStack_3,
                 threadStack_4,
                 threadStack_5,
                 threadStack_6,
                 threadStack_7,
                 threadStack_8,
                 threadStack_9,
                 threadStack_10};

        void (*threads_starts[])(void) =
                {threadStart_1,
                 threadStart_2,
                 threadStart_3,
                 threadStart_4,
                 threadStart_5,
                 threadStart_6,
                 threadStart_7,
                 threadStart_8,
                 threadStart_9,
                 threadStart_10};

        printf(1,"STARTING THREADS JOIN NEG TEST\n", THREAD_COUNT-1 );

        pid = fork();
        if(pid == 0){
            for(int i = 0;i < THREAD_COUNT;i++){
                tids[i] = kthread_create(threads_starts[i], threads_stacks[i]);
            }
            printf(1,"Created all threads, now join them..\n");

            for(int i = 0;i < THREAD_COUNT;i++){
                status = kthread_join(tids[i]);
                printf(1, "joined thread %d with status %d\n", tids[i], status);
            }

            printf(1,"Joined all threads, now try join again... PRINT THEM IF DIDN'T FAIL AS EXPECTED!\n");

            for(int i = 0;i < THREAD_COUNT;i++){
                status = kthread_join(tids[i]);
                if(status != -1){
                    printf(1, "ERROR: rejoined thread %d with status %d\n", tids[i], status);
                }
            }
            exit();
        }

        else{
            wait();
            printf(1,"t_join_neg done... should succesfully join all threads and not print ERRORs.\n");
            for(int i = 0; i < THREAD_COUNT ; i++){
                free(threads_stacks[i]);
            }
            return 0;
        }
    return -1;
}




//
///// Test implementation of mutex primitive
//// try to alloc and dealloc 30 mutexes
//int m_alloc_test(){
//
//}
//
//// alloc NMUTEXES and dealloc them
//// then, try to dealloc more (should fail!)
//int m_dealloc_test(){
//
//}
//
//// see that we can lock them
//// unlock and deallocate
//int m_lock(){
//
//}
//
//// see that we can't dual-lock the same thread and lock
//int m_lock2(){
//
//}
//
//// see that we can't unlock an unlocked thread
//int m_unlock(){
//
//}
//
//// see that we can't unlock a lock we didn't lock
//int m_unlock2(){
//
//}
//
//
///// Test implementation of mutex tree user program

int main(int argc, char *argv[]){
    t_thread_create();
    t_fork_with_threads();
    t_kexit_quit();
    t_exit();
    t_join();
    t_join_neg();
    exit();
}