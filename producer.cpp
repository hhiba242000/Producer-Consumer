// PRODUCER FILE
// Created by hayam 6207 and adel 6848 on 12/7/22.
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
#include <csignal>
#include <unistd.h>

bool infinite_loop = true;
key_t BIN_SEM_KEY = 160;// ftok("binarysem",60);
key_t EMPTY_KEY = 164;//ftok("emptysem",64);
key_t FULL_KEY = 163;//ftok("fullsem",63);
struct timespec timer{};

int binary_sem;
int empty_sem;
int full_sem;
int read_idx, written_idx;

typedef struct shmseg {
    double price;
    char name[10];
} ProductPrice;

typedef struct shmidx {
    int index;
    bool notInitialized = true;
} IndexStruct;

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

// semaphore wait function
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

// semaphore signal function
int SignalSem(int sem) {
    struct sembuf sem_buf{};
    sem_buf.sem_num = 0;
    sem_buf.sem_flg = 0;
    sem_buf.sem_op = 1;
    int retval = semop(sem, &sem_buf, 1);
    return retval;
}
// appropriately handle exit signal by intercepting it.
void handler(int sig) {
    printf("im out\n");
    infinite_loop = false;
}

void PRODUCE(IndexStruct *aShidx, ProductPrice *aShmp, int sleep_T, char *product_N, double mean, double deviation,
             int size) {
    
    // Register the signal handler for SIGINT
    signal(SIGINT, handler);

    int retval;
    ProductPrice *shmp = aShmp;
    IndexStruct *shidx = aShidx;
    double price = 0.0;
    time_t tim;
    struct tm curr;

    // Loop infinitely until the user presses Ctrl+C
    while (infinite_loop) {
        // New price generation according to normal distribution
        Generator generator(mean, deviation);
        price = generator.get();

        // Time stamp code
        struct timespec ms;
        tim = time(nullptr);
        tm curr = *localtime(&tim);
        clock_gettime(CLOCK_REALTIME, &ms);
        long ms_time = (ms.tv_nsec % 10000000);
        int ms_int = (int) ms_time / 10000;

        std::cerr << "[" << curr.tm_hour << ":" << curr.tm_min << ":" << curr.tm_sec << "." << ms_int << " "
                  << curr.tm_mday << "/" << curr.tm_mon + 1 << "/" <<
                  curr.tm_year + 1900 << "] " << product_N << ": generating a new value " << price << std::endl;

        // Time stamp code
        tim = time(nullptr);
        curr = *localtime(&tim);
        clock_gettime(CLOCK_REALTIME, &ms);
        ms_time = (ms.tv_nsec % 10000000);
        ms_int = (int) ms_time / 10000;

        std::cerr << "[" << curr.tm_hour << ":" << curr.tm_min << ":" << curr.tm_sec << "." << ms_int << " "
                  << curr.tm_mday << "/" << curr.tm_mon + 1 << "/" <<
                  curr.tm_year + 1900 << "] " << product_N << ": trying to get mutex on shared buffer" << std::endl;
        
        // Wait on empty semaphore
        retval = WaitSem(empty_sem, EMPTY_KEY);
        if (retval == -1) {
            perror("EMPTY Semaphore Locked: ");
            return;
        }

        // Wait on main binary semaphore
        retval = WaitSem(binary_sem, BIN_SEM_KEY);
        if (retval == -1) {
            perror("BINARY Semaphore Locked: ");
            return;
        }
        // Time stamp code
        tim = time(nullptr);
        curr = *localtime(&tim);
        clock_gettime(CLOCK_REALTIME, &ms);
        ms_time = (ms.tv_nsec % 10000000);
        ms_int = (int) ms_time / 10000;

        std::cerr << "[" << curr.tm_hour << ":" << curr.tm_min << ":" << curr.tm_sec << "." << ms_int << " "
                  << curr.tm_mday << "/" << curr.tm_mon + 1 << "/" <<
                  curr.tm_year + 1900 << "] " << product_N << ": placing " << price << " on shared buffer"
                  << std::endl;
        // Place the new price on the shared buffer

        shmp->price = price;
        strcpy(shmp->name, product_N);

        // Check Initialization and Update the index
        if (shidx->notInitialized) {
            shidx->index = 0;
            shidx->notInitialized = false;
        } else {
            shidx->index++;
            shidx->index = shidx->index % size;
        }
        // Signal the main binary semaphore
        retval = SignalSem(binary_sem);
        if (retval == -1) {
            perror("BINARY Semaphore Locked\n");
            return;
        }
        // Signal the full semaphore
        retval = SignalSem(full_sem);
        if (retval == -1) {
            perror("FULL Semaphore Locked\n");
            return;
        }
        // Update the shared memory pointer
        shmp = aShmp + (shidx->index * sizeof(ProductPrice *));
        
        // Time stamp code
        tim = time(nullptr);
        curr = *localtime(&tim);
        clock_gettime(CLOCK_REALTIME, &ms);
        ms_time = (ms.tv_nsec % 10000000);
        ms_int = (int) ms_time / 10000;
        std::cerr << "[" << curr.tm_hour << ":" << curr.tm_min << ":" << curr.tm_sec << "." << ms_int << " "
                  << curr.tm_mday << "/" << curr.tm_mon + 1 << "/" <<
                  curr.tm_year + 1900 << "] " << product_N << " :sleeping for " << sleep_T << " ms" << std::endl;
        
        // Sleep for the user specified time
        usleep(sleep_T * 1000);
    }

    // Detach from the shared memory after the user presses Ctrl+C
    printf("Producer detaching\n");
    aShidx->notInitialized = true;
    shmdt(aShmp);
    shmdt(aShidx);
}


int main(int argc, char **argv) {
    printf("PRODUCER LAUNCHED...\n");
    
    // union for semctl system call to initialize the semaphores 
    union semun {
        int val;
        struct semid_ds *buf;
        ushort array[1];
    } sem_attr;

    // keys for the semaphores using ftok
    key_t shm_key = 0x123333;
    key_t shm_index = 0x125454;//ftok("shmfile",65);
    int shmid, sleep_time, size;
    char *product_name;
    ProductPrice *shmp;
    IndexStruct *idx;
    read_idx = written_idx = 0;


    // code to handle arguments here ie the product name, price , sleep interval and buffer size
    sleep_time = atoi(argv[4]);
    product_name = argv[1];
    double mean = atof(argv[2]), deviation = atof(argv[3]);
    size = atoi(argv[5]);

    // print the arguments to the console
    printf("sleep time: %d\n product name: %s\n buffer size: %d\n mean:%lf \n deviation:%lf\n", sleep_time,
           product_name, size, mean, deviation);

    // Create the shared memory for the index and the shared memory for the buffer.
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

    // Create the semaphores for the producer and consumer processes 
    // to use for synchronization and mutual exclusion 
    if ((binary_sem = semget(BIN_SEM_KEY, 1, IPC_CREAT | 0666)) == -1) {
        perror("Binary Sem Creation: ");
        exit(1);
    }
    // Initialize the binary semaphore to 1
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
    // call the infinite loop producer function.
    PRODUCE(idx, shmp, sleep_time, product_name, mean, deviation, size);

}

