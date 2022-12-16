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

int binary_sem ;
int empty_sem;
int full_sem ;
int read_idx,written_idx;

typedef struct shmseg {
    double price ;
    char name[10];
    bool isUpdated=true;
} ProductPrice;

class Generator{
    std::default_random_engine generator;
    std::normal_distribution<double> distribution;
public:
    Generator(double mean, double stddev): distribution(mean, stddev){}
    double get(){
        generator = std::default_random_engine(time(0));

        return distribution(generator);

    }
};

timespec timespec{};

void PRODUCE(ProductPrice *shmp,int sleep_time,char* product_name,double mean,double deviation,int size);

int WaitSem(int sem, key_t sem_key);

int SignalSem(int sem);

double GeneratePrice(double mean, double deviation);

int main(int argc, char **argv) {
    printf("PRODUCER LAUNCHED...\n");
    union semun
    {
        int val;
        struct semid_ds *buf;
        ushort array [1];
    } sem_attr;

    key_t shm_key = 0x123333; //ftok("shmfile",65);
    int shmid = 0, sleep_time,size;
    char* product_name;
    ProductPrice *shmp;
    read_idx = written_idx = 0;


    //TODO: write code to handle arguments here ie the product name, price and sleep interval
    sleep_time = atoi(argv[4]); product_name = argv[1];
    double mean =  atof(argv[2]),deviation =  atof(argv[3]);
    size = atoi(argv[5]);
    ProductPrice buff_array[size];

    printf("%d %s %d\n",sleep_time,product_name,size);
    shmid = shmget(shm_key+0x01, sizeof(int), IPC_CREAT|0644);
    if (shmid == -1) {
        perror("Shared memory");
        return 1;
    }
    written_idx= *(int *) shmat(shmid, nullptr, 0);
    if (&written_idx == (void *) -1) {
        perror("Shared memory attach");
        return 1;
    }


    shmid = shmget(shm_key, sizeof(shmp)*size, IPC_CREAT|0644);
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
    sem_attr.val = size;        // unlocked
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
    PRODUCE(shmp, sleep_time,product_name,mean,deviation,size);

}

void PRODUCE(ProductPrice *aShmp,int sleep_T,char* product_N,double mean,double deviation,int size) {
    int sleep_time = sleep_T;
    int retval,offset = written_idx;
    ProductPrice * shmp = aShmp;
    time_t timetoday;
    time(&timetoday);
    double price = 0.0;
    char* s;
    while (true) {
        //price = GeneratePrice(mean,deviation);
        Generator generator(mean,deviation);
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

        //TODO: place here code to generate number from normal distribution
        if(shmp->isUpdated)
        {

        
        shmp->price = price;
        strcpy(shmp->name,product_N) ;
        shmp->isUpdated = false;
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
        written_idx++;
        written_idx = written_idx % size;
        shmp=aShmp+(written_idx*sizeof(ProductPrice*));
        sleep(sleep_time);
        printf("out of sleep\n");
    }
}

double GeneratePrice(double mean, double deviation) {
    std::default_random_engine generator;
    std::normal_distribution<double> distribution(mean,deviation);
    double number = distribution(generator);
    return number;
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