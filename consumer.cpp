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
    key_t shm_key = 0x1233333; //ftok("shmfile",65);

    int shmid = 0, sleep_time;
    ProductPrice *shmp;
    char *bufptr;

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
    CONSUME(shmp,2,1);

}

void CONSUME(ProductPrice *shmp,int sleep_T,int buffer_S) {
    int sleep_time = sleep_T;

    int retval;
    // int bin_semid = binary_sem,empty_semid = empty_sem,full_semid = full_sem;

    while (true) {
        //binary semaphore to lock the shared memory
        retval = WaitSem(full_sem,FULL_KEY);
        if (retval == -1) {
            perror("Semaphore Locked: ");
            return;
        }
        retval = WaitSem(binary_sem,BIN_SEM_KEY);
        if (retval == -1) {
            perror("Semaphore Locked: ");
            return;
        }

        //TODO: place here code to generate number from normal distribution
        for(int i =0;i<buffer_S;i++)
            printf("%d \n",shmp->price);

        retval = SignalSem(binary_sem);
        if (retval == -1) {
            perror("Semaphore Locked\n");
            return;
        }
//        retval = SignalSem(empty_sem);
//        if (retval == -1) {
//            perror("Semaphore Locked\n");
//            return;
//        }
        sleep(sleep_time);
        printf("out of sleep\n");
    }

}

//im getting in
int WaitSem(int sem,key_t sem_key){
    struct sembuf sem_buf;
    int retval,semnum;
    int semaphore = sem;
//    if(sem == binary_sem)
//        semnum = 1;
//    else if(sem == empty_sem)
//        semnum = MAX_BUFF;
//    else
//        semnum = 0;

    if (sem >= 0) {
        printf("Trying to fetch binary semaphore on shared memory\n");
        sem_buf.sem_op = 1;
        sem_buf.sem_flg = 0;
//        sem_buf.sem_num = semnum;
        retval = semop(semaphore, &sem_buf, 1);
    } else if (errno == EEXIST) {
        int ready = 0;
        printf("Already other process got the binary semaphore..waiting for resource\n");
        semaphore = semget(sem_key, 1, 0);
//        sem_buf.sem_num = 0;
        sem_buf.sem_op = 0;
        sem_buf.sem_flg = SEM_UNDO;
        retval = semop(semaphore, &sem_buf, 1);
    }
//    sem_buf.sem_num = 0;
    sem_buf.sem_op = -1; /* Allocating the resources */
    sem_buf.sem_flg = SEM_UNDO;
    retval = semop(semaphore, &sem_buf, 1);

    return retval;
}

//im getting out
int SignalSem(int sem){
    printf("Releasing the resource\n");
    struct sembuf sem_buf;
    sem_buf.sem_op = 1;
    int retval = semop(sem, &sem_buf, 1);
    return retval;
}