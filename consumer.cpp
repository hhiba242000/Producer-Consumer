// CONSUMER FILE
// Created by hayam on 12/7/22.
//


#include<stdio.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/types.h>
#include<string.h>
#include<errno.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <sys/sem.h>
#include <iostream>

key_t BIN_SEM_KEY = 160;// ftok("binarysem",60);
key_t EMPTY_KEY = 164;//ftok("emptysem",64);
key_t FULL_KEY = 163;//ftok("fullsem",63);
#define MAX_BUFF 1

int binary_sem = semget(BIN_SEM_KEY, 1, IPC_CREAT  | 0666);
int empty_sem = semget(EMPTY_KEY, 0,0);
int full_sem = semget(FULL_KEY, MAX_BUFF, IPC_CREAT  | 0666);

typedef struct shmseg {
    int price;
    char name[10];
} ProductPrice;

void CONSUME(ProductPrice *shmp, int sleep_time,int buffer_size);
int WaitSem(int sem,key_t sem_key);
int SignalSem(int sem);

int main(int argc, char **argv) {
    printf("CONSUMER LAUNCHED...\n");
    union semun
    {
        int val;
        struct semid_ds *buf;
        ushort array [1];
    } sem_attr;

    key_t shm_key = 0x1233333; //ftok("shmfile",65);

    int shmid = 0, sleep_time;
    ProductPrice *shmp;

    //TODO: write code to handle arguments here ie the product name, price and sleep interval

    shmid = shmget(shm_key, sizeof(ProductPrice), 0644 | IPC_CREAT);
    if (shmid == -1) {
        perror("Shared memory");
        return 1;
    }

    shmp = (ProductPrice *) shmat(shmid, nullptr, 0);
    if (shmp == (void *) -1) {
        perror("Shared memory attach");
        return 1;
    }

    if((binary_sem = semget(BIN_SEM_KEY, 1, IPC_CREAT | 0666)) == -1){
        perror("Binary Sem Creation: ");
        exit(1);
    }
    sem_attr.val = 1;        // unlocked
    if (semctl (binary_sem, 0, SETVAL, sem_attr) == -1) {
        perror ("binary sem SETVAL"); exit (1);
    }

    if((empty_sem = semget(EMPTY_KEY, 1, IPC_CREAT | 0666))==-1){
        perror("Empty Sem Creation: ");
        exit(1);
    }
    sem_attr.val = MAX_BUFF;        // unlocked
    if (semctl (empty_sem, 0, SETVAL, sem_attr) == -1) {
        perror ("empty sem SETVAL"); exit (1);
    }

    if((full_sem = semget(FULL_KEY, 1, IPC_CREAT  | 0666))==-1){
        perror("Full Sem Creation: ");
        _exit(1);
    }
    sem_attr.val = 0;        // unlocked
    if (semctl (full_sem, 0, SETVAL, sem_attr) == -1) {
        perror ("full sem SETVAL"); exit (1);
    }
    //shmp->price = 1;
    CONSUME(shmp,2,1);

}

void CONSUME(ProductPrice *shmp,int sleep_T,int buffer_S) {
    int sleep_time = sleep_T;
    time_t timetoday;
    time(&timetoday);
    int retval;
    // int bin_semid = binary_sem,empty_semid = empty_sem,full_semid = full_sem;

    while (true) {
         retval = WaitSem(full_sem, FULL_KEY);
        if (retval == -1) {
            perror("Semaphore Locked: ");
            return;
        }
        retval = WaitSem(binary_sem, BIN_SEM_KEY);
        if (retval == -1) {
            perror("Semaphore Locked: ");
            return;
        }

        //TODO: place here code to generate number from normal distribution
        for (int i = 0; i < buffer_S; i++) {
        printf("price: %d\tproduct: %s \n", shmp->price,shmp->name);
    }

        retval = SignalSem(binary_sem);
        if (retval == -1) {
            perror("Semaphore Locked\n");
            return;
        }
        retval = SignalSem(empty_sem);
        if (retval == -1) {
            perror("Semaphore Locked\n");
            return;
        }
       // sleep(sleep_time);
        printf("no sleep\n");
    }

}

int WaitSem(int sem, key_t sem_key) {
    struct sembuf sem_buf{};
    sem_buf.sem_num = 0;
    sem_buf.sem_flg = 0;
    sem_buf.sem_op = 0;
    int retval;
    int semaphore = sem;

    union semun {
        int val;
        struct semid_ds *buf;
        ushort *array;
    } arg;
    int sem_num= semctl(semaphore, 0, GETVAL, arg);

    // sem_buf.sem_num = sem_num;
    sem_buf.sem_op = -1; /* Allocating the resources */
//    sem_buf.sem_flg = SEM_UNDO;
    retval = semop(semaphore, &sem_buf, 1);

    return retval;
}

//im getting out
int SignalSem(int sem) {
    printf("Releasing the resource\n");

    struct sembuf sem_buf{};
    sem_buf.sem_num = 0;
    sem_buf.sem_flg = 0;
    sem_buf.sem_op = 0;

    sem_buf.sem_op = 1;
    int retval = semop(sem, &sem_buf, 1);
    return retval;
}
