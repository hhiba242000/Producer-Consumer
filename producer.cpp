// PRODUCER FILE
// Created by hayam on 12/7/22.
//


#include<stdio.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<errno.h>
#include<unistd.h>
#include <sys/sem.h>

key_t BIN_SEM_KEY = 160;// ftok("binarysem",60);
key_t EMPTY_KEY = 164;//ftok("emptysem",64);
key_t FULL_KEY = 163;//ftok("fullsem",63);
#define MAX_BUFF 1

int binary_sem = semget(BIN_SEM_KEY, 1, IPC_CREAT | 0666);
int empty_sem = semget(EMPTY_KEY, MAX_BUFF, IPC_CREAT | 0666);
int full_sem = semget(FULL_KEY, 0, IPC_CREAT | IPC_EXCL | 0666);

typedef struct shmseg {
    int price = 1;
    char name[10]{};
} ProductPrice;

void PRODUCE(ProductPrice *shmp, int sleep_time);

int WaitSem(int sem, key_t sem_key);

int SignalSem(int sem);

int main(int argc, char **argv) {
    printf("PRODUCER LAUNCHED...\n");
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
    //shmp->price = 1;
    PRODUCE(shmp, 2);

}

void PRODUCE(ProductPrice *shmp, int sleep_T) {
    int sleep_time = sleep_T;
    int retval;

    while (true) {
        //binary semaphore to lock the shared memory
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
    struct sembuf sem_buf;
    int retval, semnum;
    int semaphore = sem;
    union semun {
        int val;
        struct semid_ds *buf;
        ushort *array;
    } arg;
    int sem_num= semctl(semaphore, 0, GETVAL, arg);

    if (sem >= 0) {
        printf("Trying to fetch binary semaphore on shared memory\n");
        sem_buf.sem_op = 1;
        sem_buf.sem_flg = 0;
        sem_buf.sem_num = sem_num;
        retval = semop(semaphore, &sem_buf, 1);
    } else if (errno == EEXIST) {
        int ready = 0;
        printf("Already other process got the binary semaphore..waiting for resource\n");
        semaphore = semget(sem_key, 1, 0);
        sem_num= semctl(semaphore, 0, GETVAL, arg);
        sem_buf.sem_num = sem_num;
        sem_buf.sem_op = 0;
        sem_buf.sem_flg = SEM_UNDO;
        retval = semop(semaphore, &sem_buf, 1);
    }
    sem_buf.sem_num = sem_num;
    sem_buf.sem_op = -1; /* Allocating the resources */
    sem_buf.sem_flg = SEM_UNDO;
    retval = semop(semaphore, &sem_buf, 1);

    return retval;
}

//im getting out
int SignalSem(int sem) {
    printf("Releasing the resource\n");
    struct sembuf sem_buf;
    sem_buf.sem_op = 1;
    int retval = semop(sem, &sem_buf, 1);
    return retval;
}