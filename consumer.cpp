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
#include <queue>
using namespace std;

key_t BIN_SEM_KEY = 160;// ftok("binarysem",60);
key_t EMPTY_KEY = 164;//ftok("emptysem",64);
key_t FULL_KEY = 163;//ftok("fullsem",63);
#define MAX_BUFF 1

int binary_sem = semget(BIN_SEM_KEY, 1, IPC_CREAT  | 0666);
int empty_sem = semget(EMPTY_KEY, 0,0);
int full_sem = semget(FULL_KEY, MAX_BUFF, IPC_CREAT  | 0666);

typedef struct {
    char name[11];
    float price,avg;
    float history[4];
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

void CONSUME(ProductContainer *shmp, int sleep_time,int buffer_size);
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

    key_t shm_key = 0x1233451; //ftok("shmfile",65);

    int shmid = 0, sleep_time;
    ProductContainer *shmp;
    //TODO: write code to handle arguments here ie the product name, price and sleep interval

    shmid = shmget(shm_key, sizeof(ProductContainer), 0644 | IPC_CREAT);
    if (shmid == -1) {
        perror("Shared memory");
        return 1;
    }

    shmp = (ProductContainer *) shmat(shmid, nullptr, 0);
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
    CONSUME(shmp,8,6);

}

void CONSUME(ProductContainer *shmp,int sleep_T,int buffer_S) {
    int sleep_time = sleep_T;
    float avg=0;
    int retval;
    ProductPrice product;
    ProductPrice *product_ptr;
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
        // VIMP TODO: BOUNDED BUFFER, QUEUE IN SHARED MEMORY
        // PRODUCER ADDS TO QUEUE. CONSUMER POPS AND APPLIES.

        //TODO: place here code to generate number from normal distribution
        //printf("COMMODITY %s :\t",shmp->name);
        // for(int i =0;i<buffer_S;i++)
        // product=shmp->buffer.front();
        // shmp->buffer.pop();
        product=shmp->buffer[shmp->consumerIndex];
        shmp->consumerIndex=(shmp->consumerIndex+1)%buffer_S;
        if(strcmp(product.name,"GOLD"))
        {
            product_ptr=&shmp->GOLD;
        }
        else if(strcmp(product.name,"SILVER"))
        {
            product_ptr=&shmp->SILVER;
        }
        else if(strcmp(product.name,"CRUDEOIL"))
        {
            product_ptr=&shmp->CRUDEOIL;
        }
        else if(strcmp(product.name,"NATURALGAS"))
        {
            product_ptr=&shmp->NATURALGAS;
        }
        else if(strcmp(product.name,"ALUMINIUM"))
        {
            product_ptr=&shmp->ALUMINIUM;
        }
        else if(strcmp(product.name,"COPPER"))
        {
            product_ptr=&shmp->COPPER;
        }
        else if(strcmp(product.name,"LEAD"))
        {
            product_ptr=&shmp->LEAD;
        }
        else if(strcmp(product.name,"NICKEL"))
        {
            product_ptr=&shmp->NICKEL;
        }
        else if(strcmp(product.name,"ZINC"))
        {
            product_ptr=&shmp->ZINC;
        }
        else if(strcmp(product.name,"MENTHAOIL"))
        {
            product_ptr=&shmp->MENTHAOIL;
        }
        else if(strcmp(product.name,"COTTON"))
        {
            product_ptr=&shmp->COTTON;
        }
        product_ptr->price=product.price;
        product_ptr->avg=product_ptr->price;
        for(int j=0;j<4;j++)
            product_ptr->avg+=product_ptr->history[j];
        
        for(int j=0; j<4;j++){
            if(product_ptr->history[j]==0){
                product_ptr->avg/=j+1;
                break;
            }
            if(j==3)
                product_ptr->avg/=5;
            
        }
        product_ptr->history[1] = product_ptr->history[0];
        product_ptr->history[2] = product_ptr->history[1];
        product_ptr->history[3] = product_ptr->history[2];
        product_ptr->history[0] = product_ptr->price; 

        
        printf("\e[1;1H\e[2J");
        printf("+-------------------------------------+\n");
        printf("| Currency\t|  Price  |  AvgPrice |\n");
        printf("+-------------------------------------+\n");
        printf("| ALUMINIUM\t| %7.2f |  %7.2f  |\n",shmp->ALUMINIUM.price,shmp->ALUMINIUM.avg);
        printf("| COPPER\t| %7.2f |  %7.2f  |\n",shmp->COPPER.price,shmp->COPPER.avg);
        printf("| COTTON\t| %7.2f |  %7.2f  |\n",shmp->COTTON.price,shmp->COTTON.avg);
        printf("| CRUDEOIL\t| %7.2f |  %7.2f  |\n",shmp->CRUDEOIL.price,shmp->CRUDEOIL.avg);
        printf("| GOLD\t\t| %7.2f |  %7.2f  |\n",shmp->GOLD.price,shmp->GOLD.avg);
        printf("| LEAD\t\t| %7.2f |  %7.2f  |\n",shmp->LEAD.price,shmp->LEAD.avg);
        printf("| MENTHAOIL\t| %7.2f |  %7.2f  |\n",shmp->MENTHAOIL.price,shmp->MENTHAOIL.avg);
        printf("| NATURALGAS\t| %7.2f |  %7.2f  |\n",shmp->NATURALGAS.price,shmp->NATURALGAS.avg);
        printf("| NICKEL\t| %7.2f |  %7.2f  |\n",shmp->NICKEL.price,shmp->NICKEL.avg);
        printf("| SILVER\t| %7.2f |  %7.2f  |\n",shmp->SILVER.price,shmp->SILVER.avg);
        printf("| ZINC\t\t| %7.2f |  %7.2f  |\n",shmp->ZINC.price,shmp->ZINC.avg);
        
        // printf("| COPPER\t|  %7.2f | %7.2f |\n",shmp->COPPER.price,shmp->COPPER.avg);
        // printf("| COTTON\t|  %7.2f | %7.2f |\n",shmp->COTTON.price,shmp->COTTON.avg);
        // printf("| CRUDEOIL\t|  %7.2f | %7.2f |\n",shmp->CRUDEOIL.price,shmp->CRUDEOIL.avg);
        // printf("| GOLD\t\t|  %7.2f | %7.2f |\n",shmp->GOLD.price,shmp->GOLD.avg);
        // printf("| LEAD\t\t|  %7.2f | %7.2f |\n",shmp->LEAD.price,shmp->LEAD.avg);
        // printf("| MENTHAOIL\t|  %7.2f | %7.2f |\n",shmp->MENTHAOIL.price,shmp->MENTHAOIL.avg);
        // printf("| NATURALGAS\t|  %7.2f | %7.2f |\n",shmp->NATURALGAS.price,shmp->NATURALGAS.avg);
        // printf("| NICKEL\t|  %7.2f  | %7.2f |\n",shmp->NICKEL.price,shmp->NICKEL.avg);
        // printf("| SILVER\t|  %7.2f  | %7.2f |\n",shmp->SILVER.price,shmp->SILVER.avg);
        // printf("| ZINC\t\t|  %7.2f  | %7.2f |\n",shmp->ZINC.price,shmp->ZINC.avg);
        printf("+-------------------------------------+\n");
        printf("%s has been updated\n",product.name);

            
            

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
        sleep(sleep_time);
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

////im getting in
//int WaitSem(int sem,key_t sem_key){
//    struct sembuf sem_buf;
//    int retval,semnum;
//    int semaphore = sem;
////    if(sem == binary_sem)
////        semnum = 1;
////    else if(sem == empty_sem)
////        semnum = MAX_BUFF;
////    else
////        semnum = 0;
//
//    if (sem >= 0) {
//        printf("Trying to fetch binary semaphore on shared memory\n");
//        sem_buf.sem_op = 1;
//        sem_buf.sem_flg = 0;
////        sem_buf.sem_num = semnum;
//        retval = semop(semaphore, &sem_buf, 1);
//    } else if (errno == EEXIST) {
//        int ready = 0;
//        printf("Already other process got the binary semaphore..waiting for resource\n");
//        semaphore = semget(sem_key, 1, 0);
////        sem_buf.sem_num = 0;
//        sem_buf.sem_op = 0;
//        sem_buf.sem_flg = SEM_UNDO;
//        retval = semop(semaphore, &sem_buf, 1);
//    }
////    sem_buf.sem_num = 0;
//    sem_buf.sem_op = -1; /* Allocating the resources */
//    sem_buf.sem_flg = SEM_UNDO;
//    retval = semop(semaphore, &sem_buf, 1);
//
//    return retval;
//}
//
////im getting out
//int SignalSem(int sem){
//    printf("Releasing the resource\n");
//    struct sembuf sem_buf;
//    sem_buf.sem_op = 1;
//    int retval = semop(sem, &sem_buf, 1);
//    return retval;
//}