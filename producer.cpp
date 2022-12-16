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
#include <cstring>
#include <iostream>
#include <random>

key_t BIN_SEM_KEY = 160;// ftok("binarysem",60);
key_t EMPTY_KEY = 164;//ftok("emptysem",64);
key_t FULL_KEY = 163;//ftok("fullsem",63);
#define MAX_BUFF 1

int binary_sem;
int empty_sem;
int full_sem;
int read_idx, written_idx;

typedef struct shmseg {
    double price;
    char name[10];
} ProductPrice;

typedef struct shmidx{
    int index;
    bool notInitialized = true;
}IndexStruct;

class Generator {
    std::default_random_engine generator;
    std::normal_distribution<double> distribution;
public:
    Generator(double mean, double stddev) : distribution(mean, stddev) {}

    double get() {
        generator = std::default_random_engine(time(0));

        return distribution(generator);

    }
};

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

void PRODUCE(IndexStruct * aShidx,ProductPrice *aShmp, int sleep_T, char *product_N, double mean, double deviation, int size) {
    int sleep_time = sleep_T;
    int retval;
    ProductPrice *shmp = aShmp;
    IndexStruct * shidx = aShidx;
    time_t timetoday;
    time(&timetoday);
    double price = 0.0;
    char *s;
    while (true) {
        Generator generator(mean, deviation);
        price = generator.get();
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


        shmp->price = price;
        strcpy(shmp->name, product_N);
        if(shidx->notInitialized){
            shidx->index = 0;
            shidx->notInitialized = false;
        }
        else{
            shidx->index++;
            shidx->index =  shidx->index % size;
        }

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

        shmp = aShmp + (shidx->index * sizeof(ProductPrice *));
        sleep(sleep_time);
        printf("out of sleep\n");
    }
}


int main(int argc, char **argv) {
    printf("PRODUCER LAUNCHED...\n");
    union semun {
        int val;
        struct semid_ds *buf;
        ushort array[1];
    } sem_attr;

    key_t shm_key = 0x123333;
    key_t shm_index = 0x125454;//ftok("shmfile",65);
    int shmid, sleep_time, size;
    char *product_name;
    ProductPrice *shmp;
    IndexStruct *idx;
    read_idx = written_idx = 0;


    //TODO: write code to handle arguments here ie the product name, price and sleep interval
    sleep_time = atoi(argv[4]);
    product_name = argv[1];
    double mean = atof(argv[2]), deviation = atof(argv[3]);
    size = atoi(argv[5]);


    printf("sleep time: %d\n product name: %s\n buffer size: %d\n mean:%lf \n deviation:%lf\n", sleep_time,
           product_name, size, mean, deviation);

    shmid = shmget(shm_index, sizeof(idx), IPC_CREAT | 0644);
    if (shmid == -1) {
        perror("Shared memory");
        return 1;
    }
    idx = (IndexStruct *) shmat(shmid, nullptr, 0);
    if (&written_idx == (void *) -1) {
        perror("Shared memory attach");
        return 1;
    }


    shmid = shmget(shm_key, sizeof(shmp) * size, IPC_CREAT | 0644);
    if (shmid == -1) {
        perror("Shared memory");
        return 1;
    }

    shmp = (ProductPrice *) shmat(shmid, nullptr, 0);
    if (shmp == (void *) -1) {
        perror("Shared memory attach");
        return 1;
    }

    if ((binary_sem = semget(BIN_SEM_KEY, 1, IPC_CREAT | 0666)) == -1) {
        perror("Binary Sem Creation: ");
        exit(1);
    }
    sem_attr.val = 1;        // unlocked
    if (semctl(binary_sem, 0, SETVAL, sem_attr) == -1) {
        perror("binary sem SETVAL");
        exit(1);
    }

    if ((empty_sem = semget(EMPTY_KEY, 1, IPC_CREAT | 0666)) == -1) {
        perror("Empty Sem Creation: ");
        exit(1);
    }
    sem_attr.val = size;        // unlocked
    if (semctl(empty_sem, 0, SETVAL, sem_attr) == -1) {
        perror("empty sem SETVAL");
        exit(1);
    }

    if ((full_sem = semget(FULL_KEY, 1, IPC_CREAT | 0666)) == -1) {
        perror("Full Sem Creation: ");
        _exit(1);
    }
    sem_attr.val = 0;        // unlocked
    if (semctl(full_sem, 0, SETVAL, sem_attr) == -1) {
        perror("full sem SETVAL");
        exit(1);
    }
    PRODUCE(idx,shmp, sleep_time, product_name, mean, deviation, size);

}

