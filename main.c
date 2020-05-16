#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define CUT_TIME 1 /* Time to use for hair cutting */

/* semaphores */
sem_t barbers;  /* semaphore for barber */
sem_t customers; /* semaphore for customers */
sem_t mutex;      /* lock for barber's chair */

/* function */
void ip_barber(void *count);
void ip_customer(void *count);
void someSleep();

/* variable */
int numberOfBarberChair = 0;          /* number of barber chair */
int numberOfCustomer = 0;         /* number of customers */
int numberOfChair = 0;        /* number of chairs in waiting room */
int numberOfEmptyChair = 0;     /* number of empty chairs in waiting room */
int activeCustomer = 0; /* active customer id */
int chairId = 0;    /* chair id */
int *chair;                   /* identity exchange between the barber and customer */

int main(int argc, char **args)
{
   if (argc != 2)
   {
      printf("\nFail!!! \nTry again like this : sh start.sh\n\n");
      return EXIT_FAILURE;
   }

   numberOfCustomer = atoi(args[1]);
   numberOfChair = 5;
   numberOfBarberChair = 1;
   numberOfEmptyChair = numberOfChair;
   chair = (int *)malloc(sizeof(int) * numberOfChair);

   printf("\n\nNumber of customer entered:\t\t%d", numberOfCustomer);
   printf("\nWaiting room size:\t%d", numberOfChair);
   printf("\nNumber of barber:\t%d\n\n", numberOfBarberChair);

   pthread_t barber[numberOfBarberChair], customer[numberOfCustomer]; /*threads */

   /* create semaphore */
   sem_init(&barbers, 0, 0);
   sem_init(&customers, 0, 0);
   sem_init(&mutex, 0, 1);

   printf("\nBarber shop opened.\n\n");

   /* create barber threads */
   for (int i = 0; i < numberOfBarberChair; i++)
   {
      pthread_create(&barber[i], NULL, (void *)ip_barber, (void *)&i);
      sleep(1);
   }

   /* create customers threads */
   for (int i = 0; i < numberOfCustomer; i++)
   {
      pthread_create(&customer[i], NULL, (void *)ip_customer, (void *)&i);
      someSleep(); /* create customers at random intervals  */
   }

   /* take care with all customers before closing shop */
   for (int i = 0; i < numberOfCustomer; i++)
   {
      pthread_join(customer[i], NULL);
   }

   numberOfEmptyChair = -1;
   sleep(CUT_TIME);
   printf("\nAll customers are done. Shop closed. Barber leaving.\n\n");
   
   return EXIT_SUCCESS;
}

void ip_barber(void *count)
{
   int s = *(int *)count + 1;
   int nextCustomer, customer_identity;

   printf("[Barber]\tcomes to shop.\n");
   //printf("[Barber]\tuyumaya gitti.\n\n");

   while (1)
   {

      if (numberOfEmptyChair == 5 && numberOfEmptyChair != -1)
      {
         printf("[Barber]\tgoes to sleep.\n\n");
      }

      sem_wait(&barbers); /* barber do sleep */
      sem_wait(&mutex);     /* lock the chair */

      /* choice customer for cut hair */
      activeCustomer = (++activeCustomer) % numberOfChair;
      nextCustomer = activeCustomer;
      customer_identity = chair[nextCustomer];
      chair[nextCustomer] = pthread_self();

      sem_post(&mutex);      /* unlock chair */
      sem_post(&customers); /* take care of selected customer */

      printf("[Barber]\t%d. cutting.\n\n", customer_identity);
      sleep(CUT_TIME);
      printf("[Barber]\t%d. done job.\n\n", customer_identity);
   }
}

void ip_customer(void *count)
{
   int s = *(int *)count + 1;
   int activeChair, barber_identity;

   sem_wait(&mutex); /* lock chair */

   printf("[Customer: %d]\tcomes to shop.\n", s);

   /* if there are empty chairs in the waiting room*/
   if (numberOfEmptyChair > 0)
   {
      numberOfEmptyChair--;

      printf("[Customer: %d]\twaiting in waiting room.\n\n", s);

      /* select one empty chair and sit */
      chairId = (++chairId) % numberOfChair;
      activeChair = chairId;
      chair[activeChair] = s;

      sem_post(&mutex);     /* unlock chair */
      sem_post(&barbers); /* wake up barber */

      sem_wait(&customers); /* join the waiting customers tail */
      sem_wait(&mutex);      /* lock chair */

      /* sit barber chair */
      barber_identity = chair[activeChair];
      numberOfEmptyChair++;

      sem_post(&mutex); /* unlock chair */
   }
   else
   {
      sem_post(&mutex); /* unlock access chair */
      printf("[Customer: %d]\tempty chair not found in waiting room. Customer leaves the room.\n\n", s);
   }
   pthread_exit(0);
}

void someSleep()
{
   srand((unsigned int)time(NULL));
   usleep(rand() % (250000 - 50000 + 1) + 50000); /* 50000 - 250000 ms */
}
