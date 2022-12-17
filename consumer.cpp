// CONSUMER FILE
// Created by hayam 6207 and adel 6848 on 12/7/22.
//


#include<stdio.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<unistd.h>
#include <sys/sem.h>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <cstring>
#include <map>
#include <csignal>

// the given commodities 
std::string commodities[] = {"ALUMINIUM", "COPPER", "COTTON", "CRUDEOIL", "GOLD", "LEAD", "MENTHAOIL", "NATURALGAS",
                             "NICKEL", "SILVER", "ZINC"};

key_t BIN_SEM_KEY = 160;// ftok("binarysem",60);
key_t EMPTY_KEY = 164;//ftok("emptysem",64);
key_t FULL_KEY = 163;//ftok("fullsem",63);
bool infinite_loop = true;

int binary_sem;
int empty_sem;
int full_sem;
int read_idx;

// a map data structure to store the readings of each commodity
std::unordered_map<std::string, std::vector<double> *> readings_map;

typedef struct shmseg {
    double price;
    char name[10];
} ProductPrice;

timespec timespec{};

void CONSUME(int shmid,ProductPrice *shmp, int sleep_time);

int WaitSem(int sem, key_t sem_key);

int SignalSem(int sem);

// signal handler for ctrl+c SIGINT
void handler(int sig){
    printf("im out\n");
    infinite_loop = false;
}
void PrintTable(ProductPrice *pShmseg);

void InsertTable(ProductPrice *pShmseg);

void PrintTable();

int main(int argc, char **argv) {
    printf("CONSUMER LAUNCHED...\n");
    
    // union for semctl() system call to initialize the semaphore
    union semun {
        int val;
        struct semid_ds *buf;
        ushort array[1];
    } sem_attr;

    // shared memory key using ftok
    key_t shm_key = 0x123333; //ftok("shmfile",65);
    int shmid = 0, sleep_time, size;
    read_idx = 0;
    char *product_name;
    ProductPrice *shmp;

    // Code to handle arguments here ie the product name, price and sleep interval
    size = atoi(argv[1]);

    // create the shared memory segment 
    shmid = shmget(shm_key, sizeof(shmp) * size, 0644 | IPC_CREAT);
    if (shmid == -1) {
        perror("Shared memory");
        return 1;
    }

    shmp = (ProductPrice *) shmat(shmid, nullptr, 0);
    if (shmp == (void *) -1) {
        perror("Shared memory attach");
        return 1;
    }

    // create the main binary semaphore
    if ((binary_sem = semget(BIN_SEM_KEY, 1, IPC_CREAT | 0666)) == -1) {
        perror("Binary Sem Creation: ");
        exit(1);
    }
    // initialize the binary semaphore to 1
    sem_attr.val = 1;        // unlocked
    if (semctl(binary_sem, 0, SETVAL, sem_attr) == -1) {
        perror("binary sem SETVAL");
        exit(1);
    }
    // create the empty semaphore
    if ((empty_sem = semget(EMPTY_KEY, 1, IPC_CREAT | 0666)) == -1) {
        perror("Empty Sem Creation: ");
        exit(1);
    }
    sem_attr.val = size;        // unlocked
    if (semctl(empty_sem, 0, SETVAL, sem_attr) == -1) {
        perror("empty sem SETVAL");
        exit(1);
    }
    // create the full semaphore
    if ((full_sem = semget(FULL_KEY, 1, IPC_CREAT | 0666)) == -1) {
        perror("Full Sem Creation: ");
        _exit(1);
    }
    sem_attr.val = 0;        // unlocked
    if (semctl(full_sem, 0, SETVAL, sem_attr) == -1) {
        perror("full sem SETVAL");
        exit(1);
    }
    // call the infinite consume function
    CONSUME(shmid,shmp, size);

}

void CONSUME(int shmid,ProductPrice *aShmp, int size) {
    // register the signal handler for SIGINT
    signal(SIGINT,handler);

    int retval;
    time_t timetoday;
    time(&timetoday);
    double dummy_val = 0.0;
    ProductPrice *shmp = aShmp;
    ProductPrice *temp = new ProductPrice;

    // loop infinitely until the user presses ctrl+c
    while (infinite_loop) {
        // wait for the full semaphore to be unlocked
        retval = WaitSem(full_sem, EMPTY_KEY);
        if (retval == -1) {
            perror("EMPTY Semaphore Locked: ");
            return;
        }
        // wait for the binary semaphore to be unlocked
        retval = WaitSem(binary_sem, BIN_SEM_KEY);
        if (retval == -1) {
            perror("BINARY Semaphore Locked: ");
            return;
        }
        
        // copy data from the shared memory to the local variable
        strcpy(temp->name, shmp->name);
        temp->price = shmp->price;

        retval = SignalSem(binary_sem);
        if (retval == -1) {
            perror("BINARY Semaphore Locked\n");
            return;
        }
        retval = SignalSem(empty_sem);
        if (retval == -1) {
            perror("FULL Semaphore Locked\n");
            return;
        }

        printf("\e[1;1H\e[2J");
        printf("+--------------------------------------+\n");
        printf("| Currency\t|  Price  |  AvgPrice  |\n");
        printf("+--------------------------------------+\n");
        
        // insert the data into the map
        InsertTable(temp);

        // print the table
        PrintTable();

        // increment the read index
        read_idx++;
        read_idx = read_idx % size;

        // increment the shared memory pointer
        shmp = aShmp + (read_idx * sizeof(ProductPrice *));
    }

    // detach the shared memory after the user presses ctrl+c
    printf("im detaching\n");
    shmdt(aShmp);
}

void PrintTable() {
    std::vector<double> *readings;
    double sum = 0.0;
    int flag = 0, total;
    std::string name;
    double price , avg;
    for (auto s: commodities) {
        name= s;
        
        // check if the commodity is present in the map
        if (readings_map.find(s.c_str()) == readings_map.end()) {
        price = 0.0; avg = 0.0;
        flag=0;
        }
        else{
            
            // get the vector of prices for the commodity
            readings = readings_map.at(s);
            sum = 0.0;
            flag= 0,total = 0.0;
            total = readings->size();
            price = readings->at(total-1);
            
            // calculate the average price
            for (int i = 0; i < total; i++) {
                sum += readings->at(i);
            }
            avg = sum / total;

            // check if the price is increasing or decreasing
            if (readings->size() == 1) {
                flag = 2;

            } else {
                int x = total - 1;
                if (readings->at(x - 1) < readings->at(x))
                    flag = 2;
                else if (readings->at(x - 1) > readings->at(x))
                    flag = 1;
                else
                    flag = 0;
            }
        }


        // print the table with the appropriate color
        //red 31  green 32   blue 34
        if (flag == 2) {
                printf("| %-14s| \033[0;32m%-7.2f↑\033[0m|  \033[0;32m%-7.2f↑\033[0m  |\n", name.c_str(), price,
                       avg);
        } else if (flag == 1) {
                printf("| %-14s| \033[0;31m%-7.2f↓\033[0m|  \033[0;31m%-7.2f↓\033[0m  |\n", name.c_str(), price,
                       avg);
        } else {
                printf("| %-14s|  \033[0;34m%-7.2f\033[0m|   \033[0;34m%-7.2f\033[0m  |\n", name.c_str(), price,
                       avg);
        }
        printf("+--------------------------------------+\n");
    }
}

void InsertTable(ProductPrice *pShmseg) {
   
    // insert the data into the map
    std::vector<double> *readings;

    // check if the commodity is present in the map
    if (readings_map.find(pShmseg->name) == readings_map.end()) {
        readings = new std::vector<double>;
        readings->emplace_back(pShmseg->price);
        readings_map.insert({pShmseg->name, readings});
    } else {
        readings = readings_map.at(pShmseg->name);
        readings->emplace_back(pShmseg->price);
    }
    // keep the most recent 5 prices
    // current price, 4 previous prices
    if (readings->size() > 5)
        readings->erase(readings->begin());
}


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