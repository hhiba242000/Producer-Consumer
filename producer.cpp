// PRODUCER FILE
// Created by hayam on 12/7/22.
//


#include<stdio.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<unistd.h>
#include <sys/sem.h>
#include <cstdlib>
#include <ctime>
#include <iostream>

key_t BIN_SEM_KEY = 160;// ftok("binarysem",60);
key_t EMPTY_KEY = 164;//ftok("emptysem",64);
key_t FULL_KEY = 163;//ftok("fullsem",63);
#define MAX_BUFF 1

int binary_sem ;
int empty_sem;
int full_sem ;

typedef struct shmseg {
    int price = 1;
    std::string name;
} ProductPrice;

timespec timespec{};

void PRODUCE(ProductPrice *shmp,int sleep_time);

int WaitSem(int sem, key_t sem_key);

int SignalSem(int sem);

int main(int argc, char **argv) {
    printf("PRODUCER LAUNCHED...\n");
    union semun
    {
        int val;
        struct semid_ds *buf;
        ushort array [1];
    } sem_attr;

    key_t shm_key = 0x1233333; //ftok("shmfile",65);
    int shmid = 0, sleep_time;
    char* product_name;
    ProductPrice *shmp;

    //TODO: write code to handle arguments here ie the product name, price and sleep interval
    sleep_time = 2;

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
    PRODUCE(shmp, sleep_time);

}

void PRODUCE(ProductPrice *shmp,int sleep_T) {
    int sleep_time = sleep_T;
    int retval;
    time_t timetoday;
    time(&timetoday);
    char* s;
    while (true) {
        retval = WaitSem(empty_sem, EMPTY_KEY);
        if (retval == -1) {
            perror("EMPTY Semaphore Locked: ");
            return;
        }
        retval = WaitSem(binary_sem, BIN_SEM_KEY);
        if (retval == -1) {
            perror("BINARY Semaphore Locked: ");
            return;
        }

        //TODO: place here code to generate number from normal distribution

        shmp->price += 1;

        retval = SignalSem(binary_sem);
        if (retval == -1) {
            perror("BINARY Semaphore Locked\n");
            return;
        }
        retval = SignalSem(full_sem);
        if (retval == -1) {
            perror("FULL Semaphore Locked\n");
            return;
        }
        sleep(sleep_time);
        printf("out of sleep\n");
    }
}

//im getting in
int WaitSem(int sem, key_t sem_key) {
    struct sembuf sem_buf{};
    sem_buf.sem_num = 0;
    sem_buf.sem_flg = 0;
    sem_buf.sem_op = -1;
    int retval;
    int semaphore = sem;

     /* Allocating the resources */
    retval = semop(semaphore, &sem_buf, 1);

    return retval;
}

//im getting out
int SignalSem(int sem) {
    struct sembuf sem_buf{};
    sem_buf.sem_num = 0;
    sem_buf.sem_flg = 0;
    sem_buf.sem_op = 1;
    int retval = semop(sem, &sem_buf, 1);
    return retval;
}