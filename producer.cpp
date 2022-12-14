#include<stdio.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<errno.h>
#include<unistd.h>
#include <sys/sem.h>
#include <cstdlib>
#include <random>
#include <iostream>
#include <string.h>
#include <queue>

using namespace std;

class Generator{
    default_random_engine generator;
    normal_distribution<double> distribution;
public:
    Generator(double mean, double stddev): distribution(mean, stddev){}
    double get(){
        generator = default_random_engine(time(0));

        return distribution(generator);

    }
};


key_t BIN_SEM_KEY = 160;// ftok("binarysem",60);
key_t EMPTY_KEY = 164;//ftok("emptysem",64);
key_t FULL_KEY = 163;//ftok("fullsem",63);
#define MAX_BUFF 6;

int binary_sem ;
int empty_sem;
int full_sem ;

typedef struct {
    char name[11];
    float price,avg;
    float history[4];
    // bool isUpdated=false;
} ProductPrice;

typedef struct shmseg {
    ProductPrice buffer[20];
    int producerIndex=0,consumerIndex=0;
    ProductPrice GOLD={"GOLD",0,0,{0,0,0,0}};
    ProductPrice SILVER={"SILVER",0,0,{0,0,0,0}};
    ProductPrice CRUDEOIL={"CRUDEOIL",0,0,{0,0,0,0}};
    ProductPrice NATURALGAS={"NATURALGAS",0,0,{0,0,0,0}};
    ProductPrice ALUMINIUM={"ALUMINIUM",0,0,{0,0,0,0}};
    ProductPrice COPPER={"COPPER",0,0,{0,0,0,0}};
    ProductPrice LEAD={"LEAD",0,0,{0,0,0,0}};
    ProductPrice NICKEL={"NICKEL",0,0,{0,0,0,0}};
    ProductPrice ZINC={"ZINC",0,0,{0,0,0,0}};
    ProductPrice MENTHAOIL={"MENTHAOIL",0,0,{0,0,0,0}};
    ProductPrice COTTON={"COTTON",0,0,{0,0,0,0}};
} ProductContainer;


void PRODUCE(ProductContainer *shmp,ProductPrice product, int sleep_time,float mean,float stddev,int bufferSize);

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

    key_t shm_key = 0x1233451; //GOLD;
    // key_t silver_key = 0x1233401; //SILVER;

    //char name[10];
   // strcpy(name, argv[1]);

    int shmid = 0, sleep_time=stoi(argv[4]);
    ProductContainer *shmp;
    //TODO: write code to handle arguments here ie the product name, price and sleep interval
    shmid = shmget(shm_key, sizeof(ProductContainer), 0644 | IPC_CREAT);
    if (shmid == -1) {
        perror("Shared memory");
        return 1;
    }
    ProductPrice temp;
    temp.price=0;
    // for (int j=0;j<20;j++)
    //     shmp->buffer.push(temp);

    shmp = (ProductContainer *) shmat(shmid, nullptr, 0);
    if (shmp == (void *) -1) {
        perror("Shared memory attach");
        return 1;
    }
    // if (shmp->buffer.front().price==0){
    //     for(int j=0;j<20;j++){
    //         shmp->buffer.pop();
    //     }
    // }
        

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
    //sem_attr.val = stoi(argv[5]);        // unlocked
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
    ProductPrice product;
    product.price=0;
    product.avg=0;
    strcpy(product.name, argv[1]);
    // if(strcmp(argv[1],"GOLD")==0)
    //     product=&(shmp->GOLD);
    // else if(strcmp(argv[1],"SILVER")==0)
    //     product=&(shmp->SILVER);
    // else if(strcmp(argv[1],"CRUDEOIL")==0)
    //     product=&(shmp->CRUDEOIL);
    // else if(strcmp(argv[1],"NATURALGAS")==0)
    //     product=&(shmp->NATURALGAS);
    // else if(strcmp(argv[1],"ALUMINIUM")==0)
    //     product=&(shmp->ALUMINIUM);
    // else if(strcmp(argv[1],"COPPER")==0)
    //     product=&(shmp->COPPER);
    // else if(strcmp(argv[1],"LEAD")==0)
    //     product=&(shmp->LEAD);
    // else if(strcmp(argv[1],"NICKEL")==0)
    //     product=&(shmp->NICKEL);
    // else if(strcmp(argv[1],"ZINC")==0)
    //     product=&(shmp->ZINC);
    // else if(strcmp(argv[1],"MENTHAOIL")==0)
    //     product=&(shmp->MENTHAOIL);
    // else if(strcmp(argv[1],"COTTON")==0)
    //     product=&(shmp->COTTON);
    // else
    //     printf("Invalid Product Name\n");

    PRODUCE(shmp,product, sleep_time,stof(argv[2]),stof(argv[3]),stoi(argv[5]));

}

void PRODUCE(ProductContainer *shmp,ProductPrice product, int sleep_T, float mean, float stddev,int bufferSize) {
    int sleep_time = sleep_T;
    int retval;

    while (true) {
        //semaphore on empty places
        retval = WaitSem(empty_sem, EMPTY_KEY);
        if (retval == -1) {
            perror("EMPTY Semaphore Locked: ");
            return;
        }
        //semaphore on shared buffer
        retval = WaitSem(binary_sem, BIN_SEM_KEY);
        if (retval == -1) {
            perror("BINARY Semaphore Locked: ");
            return;
        }
        // VIMP TODO: BOUNDED BUFFER, QUEUE IN SHARED MEMORY
        // PRODUCER ADDS TO QUEUE. CONSUMER POPS AND APPLIES.

        //TODO: place here code to generate number from normal distribution
        Generator generator(mean, stddev);
        product.price = generator.get();
        shmp->buffer[shmp->producerIndex].price=product.price;
        strcpy(shmp->buffer[shmp->producerIndex].name,product.name);
        shmp->producerIndex=(shmp->producerIndex+1)%bufferSize;
        printf("Producer: %s, Price: %f, Index: %d, Buffer Size: %d \n", product.name, product.price,shmp->producerIndex,bufferSize);
        

        // shmp->buffer.push(product);
        // product->avg=product->price;
        // for(int j=0;j<4;j++)
        //     product->avg+=product->history[j];
        
        // for(int j=0; j<4;j++){
        //     if(product->history[j]==0){
        //         product->avg/=j+1;
        //         break;
        //     }
        //     if(j==3)
        //         product->avg/=5;
            
        // }
        // product->history[1] = product->history[0];
        // product->history[2] = product->history[1];
        // product->history[3] = product->history[2];
        // product->history[0] = product->price; 

        //shmp->price += 15.00;

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
    sem_buf.sem_op = 0;
    int retval;
    int semaphore = sem;

    sem_buf.sem_op = -1; /* Allocating the resources */
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