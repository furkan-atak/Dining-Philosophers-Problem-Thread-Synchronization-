#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
double tableOpenCost = 99.90;
double priceTableReopen = 19.90;
double riceCost = 20;
int philCount = 0;

#define tableRiceAmount 2000
#define eatingRiceQuantity 100
#define tableCount 10
#define capacity 80

enum {THINKING, EATING};


typedef struct Philosopher
{
    int id;
    int thinkingTime;
    int eatingTime;
    int tableNumber;
    int eatenRiceAmount;
    int status;
    pthread_t philThread;
    pthread_mutex_t lock;
}Philosopher;

typedef struct Table
{
    int id;
    int riceAmount;
    int reOrderAmount;
    int eatenRiceAmount;
    int chairCount;
    int chairRemaining;
    int finishedCount;
    int opened;
    int* philAtChairs;
    double receipt;
    pthread_mutex_t fullLock;
    pthread_mutex_t finishLock;
    pthread_mutex_t orderLock;
    pthread_mutex_t eatLock;
}Table;

pthread_mutex_t printReceiptLock;
pthread_mutex_t sitToTableLock;

sem_t enterRestaurantLock;

Philosopher* philList;
Table* tableList;

int restaurantCapacity;

void* enterTheRestaurant(void*); // done
void eat(int); // done
void think(int); // done
int numOfHungryAtTable(Table); // done
void tableBill(Table); // done
void reOrder(Table*); // done
void openTable(Table*); // done
Table createTable(int); // done
void printPhil(Philosopher); // done
Philosopher createPhil(int); // done
void printTable(Table); // done
int sitToTable(int); // done

void* enterRestaurant(void* i) {
    
    int id = (int*)i;

    printf(" Philosopher:%d  is trying to enter.. \n", id);
    sem_wait(&enterRestaurantLock);  // if under capacity (80) enter the restaurant
    
    pthread_mutex_lock(&sitToTableLock); // lock until finding table
    sleep(0.5);
    int tableId = sitToTable(id); // find table by given philosopher Id
    pthread_mutex_unlock(&sitToTableLock);

    pthread_mutex_lock(&tableList[tableId].fullLock);
    sleep(3);
    printf(" @@@@@@@@%d  CHAIR COUNT@@@@@@@\n", tableList[tableId].chairRemaining); 
    if(tableList[tableId].chairRemaining == 0) {
        
        printTable(tableList[tableId]);
        
        if(!tableList[tableId].opened) {
            openTable(&tableList[tableId]);
            printf("TABLE OPENED\n");
        }
        

        int i;
        for(i = 0; i < tableList[tableId].chairCount; i++){
                  // while has issues.

                sleep(philList[tableList[tableId].philAtChairs[i]].eatingTime);
                pthread_mutex_lock(&tableList[tableId].eatLock);
                eat(tableList[tableId].philAtChairs[i]);
                printf("philosopher:%d ate %d gr rice and eaten %d in the table, %d-table has %d amountrice \n",
                 philList[tableList[tableId].philAtChairs[i]].id, philList[tableList[tableId].philAtChairs[i]].eatenRiceAmount,
                 tableList[tableId].eatenRiceAmount, tableList[tableId].id,tableList[tableId].riceAmount);
                pthread_mutex_unlock(&tableList[tableId].eatLock);
                
                sleep(philList[tableList[tableId].philAtChairs[i]].thinkingTime);
                pthread_mutex_lock(&tableList[tableId].finishLock);
                think(tableList[tableId].philAtChairs[i]);
                pthread_mutex_unlock(&tableList[tableId].finishLock);
                
                // if(numOfHungryAtTable(tableList[tableId]) > 0 ) {
                //     reOrder(&tableList[tableId]);

                // }
            
        }
        
      //  pthread_mutex_unlock(&tableList[tableId].fullLock);
    }
    sem_post(&enterRestaurantLock);
    sleep(5);
}

void eat(int id) {

    Philosopher *phil = &philList[id];
    Table *table = &tableList[phil->tableNumber];
    if(table->riceAmount <= 0) {
        return;
    }
    phil->eatenRiceAmount += 100;
    phil->status = EATING;
    table->riceAmount = table->riceAmount - 100;
    table->eatenRiceAmount += 100;

}

void think(int id) {
    Philosopher *phil = &philList[id];
    phil->eatenRiceAmount += 100;
    phil->status = THINKING;
}

int sitToTable(int id) {
    int theTable = -1;
    int i;
    int j;
    for (i = 0; i < tableCount; i++) {
        for(j = 0; j < tableList->chairCount; j++) {
            if(tableList[i].philAtChairs[j] == 0) {
                tableList[i].chairRemaining = tableList[i].chairRemaining-1 >= 0 ? tableList[i].chairRemaining-1 : -1;
                if (tableList[i].chairRemaining  == -1) { 
                    tableList[i].chairRemaining = 0; 
                    continue;
                }
                tableList[i].philAtChairs[j] = id;
                philList[tableList[i].philAtChairs[j]].tableNumber = tableList[i].id;
                theTable = tableList[i].id;
                printf("Philosopher:%d is sittin to Table:%d and Table has %d chairs more\n", id, theTable, tableList[i].chairRemaining);
                return theTable;
            }
        } 
    }
    
    return theTable; // When Not Found
}

void reOrder(Table* table) {
    table->riceAmount = tableRiceAmount; // reordering 
    table->receipt += riceCost;
    table->receipt += priceTableReopen;
    table->eatenRiceAmount += 2;
    table->reOrderAmount += 1;
}

Philosopher createPhil(int id) {
    Philosopher phil;
    phil.eatenRiceAmount = 0;
    phil.eatingTime = (rand() % 5) + 1; 
    phil.thinkingTime = (rand() % 5) + 1;
    phil.id = id;
    phil.status = -1;
    phil.tableNumber = -1;
    pthread_mutex_init(&phil.lock, NULL); // philosopher lock initializing.
    return phil;
}

Table createTable(int id) {
    Table t;
    t.id = id;
    t.eatenRiceAmount = 0;
    t.chairRemaining = 8;
    t.finishedCount = 0;
    t.opened = 0;
    t.receipt = 0.0;
    t.reOrderAmount = 0;
    t.riceAmount = tableRiceAmount;
    t.chairCount = 8;
    t.philAtChairs = (int*)calloc(8, sizeof(int));
    pthread_mutex_init(&t.finishLock, NULL); // lock until everyone eat initializing.
    pthread_mutex_init(&t.fullLock, NULL); // lock until table full initializing.
    pthread_mutex_init(&t.orderLock, NULL); // make wait each phil when ordering init.
    pthread_mutex_init(&t.eatLock, NULL); // wait when one eating initializing.
    return t;
}

void openTable(Table* table) {
    table->chairRemaining = 8;
    table->riceAmount = 2000;
    table->opened = 1;
    table->reOrderAmount = 0;
    table->finishedCount = 0;
    table->receipt = tableOpenCost + riceCost;

}

int numOfHungryAtTable(Table table){
    int i;
    int numOfHungry = 0;
    for(i = 0; i < table.chairCount; i++){
        Philosopher phil = philList[table.philAtChairs[i]];
        if(phil.eatenRiceAmount == 0){
            numOfHungry++;
        }
    }
    return numOfHungry;
}

void printPhil(Philosopher phil) {
    printf("%d-Philosopher | thinkingTime:%d | tableNumber:%d | status:%d | eatenRice:%d \n",
    phil.id, phil.thinkingTime, phil.tableNumber, phil.status, phil.eatenRiceAmount);
}

void printTable(Table table) {
    printf("%d-Table | chairCount:%d | riceAmount:%d | chairRemaining:%d \n", table.id, table.chairCount,
    table.riceAmount, table.chairRemaining);
}

void tableBill (Table table) {
    pthread_mutex_lock(&printReceiptLock);
    printf("*** Table %d Receipt ***\n", table.id);
    int i = 0;
    for(i = 0; i < table.chairCount; i++) {
        Philosopher phil = philList[table.philAtChairs[i]];
        printf("Philosopher: %d eat: %d \n", phil.id, phil.eatenRiceAmount);
    }    
    printf("Eaten Rice In Table: %d g \n", table.eatenRiceAmount);
    printf("reOrder Count: %d \n", table.reOrderAmount);
    printf("Total cost: %f \n", table.receipt);
    printf("------------ ||| ------------\n");
    pthread_mutex_unlock(&printReceiptLock);
}

int main(int argc, char const *argv[])
{
    srand(time(NULL));
    printf("Please enter group count: ");
    int philGroupCount = 0;
    scanf("%d", &philGroupCount);  // restoran kapasitesini hesapla, daha fazlasını almamak için
    
    sem_init(&enterRestaurantLock, 0, capacity); // tek seferde restoran kapasitesi kadar kişi içeri girer
    
    pthread_mutex_init(&sitToTableLock, NULL);
    pthread_mutex_init(&printReceiptLock, NULL);
    
    tableList = (Table*)calloc(sizeof(Table), tableCount);
    int i;
    for(i = 0; i < tableCount; i++) {
        *(tableList+i) = createTable(i);
        printTable(tableList[i]);
    }
    philCount = philGroupCount * 8;
    philList = (Philosopher*)calloc(philCount, sizeof(Philosopher));
    int j;
    for(j = 0; j < philCount; j++) {
        *(philList+j) = createPhil(j);
        printPhil(philList[j]);
    }

    int k;
    for(k = 0; k < philCount; k++){
        pthread_create(&philList[k].philThread, NULL, enterRestaurant, (void*)k);
    }
    int l;

    for(l = 0; l < philCount; l++){
        pthread_join(philList[l].philThread, NULL);
    }
    

    /* code */
    return 0;
}
