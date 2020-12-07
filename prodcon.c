//==============================================================================
// Author      : Zhenia Steger
// Purpose     : Demonstrate use of semaphores and threads.
// =============================================================================

// INCLUDE FILES
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// GLOBAL VARIABLE DECLARATIONS
char *shmPointer = NULL;      // global shared memory pointer
int memsize = 0;              //
int ntimes = 0;               // global memsize and ntimes
int err;
pthread_t tid[2];             //
pthread_mutex_t mutex;        // mutex semaphore
sem_t empty, full, count;     // create semaphores

// PROTOTYPE FUCNTIONS
unsigned short checksum(char* data);

// PRODUCER THREAD - WRITES RANDOM DATA TO 32 BYTE BUFFER AND COMPUTES CHECKSUM
void *producer()
{
  char localBuffer[32];              // LOCAL BUFFER SO WE DONT HAVE TO WAIT AS LONG
  int i = 0;
  int j = 0;
  unsigned short int sum = 0;        // FOR THE CHECKSUM
  // LOOPS NTIMES
  for(i = 0; i < ntimes; i++) {
    for(j = 0; j < 30; j++)
    {
      localBuffer[j] = (char)(rand()%256);
    }
    sum = checksum(localBuffer);
    // ASSIGN THE CHECKSUM TO THE LAST 2 BYTES OF THE LOCAL BUFFER
    localBuffer[25] = (uint8_t)((sum >> 8) & 0xff);
    localBuffer[26] = (uint8_t)(sum & 0xff);

    for(j = 0; i < 32; i++)
    {
      sem_wait(&empty);
      /* Begin the critical section */
      pthread_mutex_lock(&mutex);
      shmPointer[i] = localBuffer[i];
      sem_post(&count);
      pthread_mutex_unlock(&mutex);
      /* end the critical section */
      sem_post(&full);
    }
  }
}

// CONSUMER THREAD - READS RANDOM DATA FROM THE 32 BYTE BUFFER AND CHEKS CHECKSUM
void *consumer()
{
  char localBuffer[32];               // LOCAL BUFFER SO WE DONT HAVE TO WAIT AS LONG
  int i = 0;
  int j = 0;
  unsigned short int sumRead = 0;     // FOR THE CHECKSUM
  unsigned short int sumVerify = 0;   // CHECKSUM
  // LOOPS NTIMES
    for(j = 0; i < 32; i++)
    {
      sem_wait(&full);
      /* Begin the critical section */
      pthread_mutex_lock(&mutex);
      localBuffer[i] = shmPointer[i];
      sem_wait(&count);
      pthread_mutex_unlock(&mutex);
      /* end the critical section */
      sem_post(&empty);
    }
    // CHECK THE CHECKSUM OF THE DATA COMING IN
    sumVerify = checksum(localBuffer);
    // READ THE CHECKSUM FROM THE LAST 2 BYTES
    uint8_t firstByte, secondByte;
    firstByte = localBuffer[25];
    secondByte = localBuffer[26];
    sumRead =  (firstByte << 8) | secondByte;

    // IF THE CHECKSUMS DO NOT MATCH PRINT AN ERROR
    if (sumRead != sumVerify)
    {
      printf("\n Error: Checksum does not match.\n");
    }
}

// INTERNET CHECKSUM FUNCTION
unsigned short checksum(char* data) {
  int i = 0;
  // Initialise the accumulator.
  unsigned short acc=0xffff;

  // Handle complete 16-bit blocks.
  for (i = 0; i+1<30; i += 2) {
    uint16_t word;
    memcpy(&word,data+i,2);
    acc+=word;
    if (acc>0xffff) {
      acc-=0xffff;
    }
  }
  // Return the checksum
  return acc;
}

// MAIN METHOD RUNS THE PROGRAM - REQUIRES TWO ARGUMENTS
// memsize | is the amount of memory to pass
// ntimes  | how many times the producer writes and the consumer reads from the buffer
int main(int argc, char const *argv[]) {

  // VARIABLE AND SEMAPHORE DECLARATIONS
  //pthread_mutex_lock(&mutex);  // mutex semaphore declaration
  if(sem_init(&empty, 0, memsize)) {perror("Semaphore create error."); exit(0);}
  if(sem_init(&full, 0, memsize)) {perror("Semaphore create error."); exit(0);}
  if(sem_init(&count, 0, memsize)) {perror("Semaphore create error."); exit(0);}

  /* ERROR CHECKING */
  if (argc !=3) {fprintf(stderr, "Error. Incorrect number of arguments (Needed 2)\n"), exit(0);}
  if (atoi(argv[1]) == 0) {fprintf(stderr, "memsize is not an integer.\n"); exit(0);}
  if (atoi(argv[1]) == 0) {fprintf(stderr, "ntimes is not an integer.\n"); exit(0);}
  memsize = atoi(argv[1]);    // Assigns the value to memsize from the prog argument
  if (memsize < 0) {fprintf(stderr, "memsize cannot be negative.\n"); return -1;}
  if (memsize >= 65) {fprintf(stderr, "memsize cannot be greater than 64.\n"); return -1;}
  if (memsize % 32 != 0) {fprintf(stderr, "memsize must be a multiple of 32.\n"); return -1;}
  ntimes = atoi(argv[2]);     // Assigns the value to ntimes from the prog argument
  if (ntimes < 0) {fprintf(stderr, "ntimes cannot be negative.\n"); return -1;}
  /* END ERROR CHECKING */

  shmPointer = malloc(memsize);

  // CREATING MUTEX SEMAPHORE
  if (pthread_mutex_init(&mutex, NULL) != 0) {
    printf("\nError creating mutex\n");
    return -1;
  }
  // CREATING THE PRODUCER THREAD
  err = pthread_create(&(tid[0]), NULL, &producer, NULL);
  if (err != 0) {
    printf("\ncan't create thread 2 :[%s]", strerror(err));
  }
  // CREATING THE CONSUMER THREAD
  err = pthread_create(&(tid[1]), NULL, &consumer, NULL);
  if (err != 0) {
    printf("\ncan't create thread 2 :[%s]", strerror(err));
  }

  // JOINING THE THREADS
  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);

  // DESTROY THE SEMAPHORE REFERENCE
  pthread_mutex_destroy(&mutex);
  return 0;
}
