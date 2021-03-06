/*
* Author: Claudio Santos e Gustavo
  * Problem: Producer and Consumer distributed Problem using openMPI
*/

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>
#include <math.h>

#define MAXBUFFER 10
#define COORDINATORLABEL 0
#define PRODUCERLABEL 1
#define CONSUMERLABEL 2
#define TIMETOWAIT 1

void * ProducerListener();
void * ConsumerListener();
int ProducedItens(int buffer[MAXBUFFER]);
void SendData();
void PrintBuffer();
void WaitFor (unsigned int secs);

// ======================
// FUNCTION FILE MANANGER
// ======================
void WriteOnProducerFile (FILE *f, char *message);
void WriteOnConsumerFile (FILE *f, char *message);

// =============================
// Initialize structures
// =============================

int producerIndex;
int consumerIndex;
int buffer[MAXBUFFER];

// =============================
// Thread structures
// =============================
pthread_t coordinatorProducerListener;
pthread_t coordinatorConsumerListener;
sem_t mutex_produtor; // Up - Down
sem_t mutex_consumidor; // True - False
pthread_mutex_t mutex_buffer = PTHREAD_MUTEX_INITIALIZER; // True - False

//pthread_mutex_init (&mutex_produtor, MAXBUFFER);
//pthread_mutex_init (&mutex_consumidor, 0);
//pthread_mutex_init (&mutex_buffer, 1);

int main(int argc, char **argv)
{
  // if (sem_init(&mutex_produtor, MAXBUFFER, 1 ) != 0)
	// {
  //   exit(1);
	// }
  //
  // if (sem_init(&mutex_consumidor, 0, MAXBUFFER) != 0)
	// {
  //   exit(1);
	// }

  // if (sem_init(&mutex_buffer, 0, 1) != 0)
	// {
  //   exit(1);
	// }

  // =============================
  // Initialize MPI structures
  // =============================
  int myRank;
  int np;
  MPI_Status stats;
  MPI_Init (&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
  MPI_Comm_size(MPI_COMM_WORLD, &np);

  // =============================
  // Coordinator
  // =============================
  if (myRank == 0)
  {
    printf("Coordinator\n");
    if(pthread_create(&coordinatorProducerListener, NULL, ProducerListener,NULL) != 0)
    {
      exit(1);
    }

    if (pthread_create(&coordinatorConsumerListener , NULL, ConsumerListener,NULL) != 0)
    {
      exit(1);
    }

    if (pthread_join(coordinatorProducerListener, NULL) != 0)
    {
      exit(1);
    }

    if (pthread_join(coordinatorConsumerListener, NULL) != 0)
    {
      exit(1);
    }

  }
  // =============================
  // Producer
  // =============================
  else if (myRank % 2 == 0)
  {
    while (1 != 0)
    {
      int message = -101;
      MPI_Send (&message, 1, MPI_INT, COORDINATORLABEL, PRODUCERLABEL, MPI_COMM_WORLD );

      int answer;
      MPI_Recv (&answer, 1, MPI_INT, COORDINATORLABEL, COORDINATORLABEL, MPI_COMM_WORLD, &stats);
      // printf("%d\n",answer );

      if (answer == -400)
      {
        int randomicNumber = rand() % 100;
        // printf("Data Produced |%d\n",randomicNumber );
        MPI_Send (&randomicNumber, 1, MPI_INT, COORDINATORLABEL, PRODUCERLABEL, MPI_COMM_WORLD);
      }
      else
      {
        /**
          WAIT FOR A SHORT PERIOD OF TIME AND THEN REQUEST TIMESPACE AGAIN
        */
        WaitFor (TIMETOWAIT + 1);
      }
    }
  }
  // =============================
  // Consumer
  // =============================
  else if (myRank % 2 != 0)
  {
    while (1 != 0)
    {
      int message = -102;
      MPI_Send (&message, 1, MPI_INT, COORDINATORLABEL, CONSUMERLABEL, MPI_COMM_WORLD );

      int answer;
      MPI_Recv (&answer, 1, MPI_INT, COORDINATORLABEL, COORDINATORLABEL, MPI_COMM_WORLD, &stats);
      // printf("%d\n",answer );
      if (answer == -400)
      {
        int data;
        MPI_Recv (&data, 1, MPI_INT, COORDINATORLABEL, COORDINATORLABEL, MPI_COMM_WORLD, &stats);
        // printf("Data Consumed |%d\n",data );
      }
      else
      {

        /**
          WAIT FOR A SHORT PERIOD OF TIME AND THEN REQUEST TIMESPACE AGAIN
        */
        WaitFor (TIMETOWAIT + 1);
      }
    }
  }


  MPI_Finalize();
  return 0;
}

// =============================
// FUNCTIONS
// =============================

void * ProducerListener()
{
  MPI_Status stats;
  printf("ProducerListener\n");
  // File *f = fopen ("producer_log_file.txt");
  while (1 != 0)
  {
    int message,
        hasToWait = 1,
        temp;


    MPI_Recv (&message, 1, MPI_INT, MPI_ANY_SOURCE, PRODUCERLABEL, MPI_COMM_WORLD, &stats);

    printf("%d|Producer|Producer receive message| %d | from| %d\n",(int)time(NULL),message, stats.MPI_SOURCE );
    printf("%d|Producer|Producer Index|%d|Consumer Index |%d|\n",(int)time(NULL), producerIndex, consumerIndex );
    // sem_wait (&mutex_buffer);
    // PrintBuffer();
    // sem_post (&mutex_buffer);

    //1. If Message is Producer Request
    if (message == -101)
    {
      //1.2. Check if buffer is not full

      pthread_mutex_lock (&mutex_buffer);
      if (ProducedItens (buffer) < MAXBUFFER && (buffer[producerIndex] == 0))
      {
        pthread_mutex_unlock (&mutex_buffer);
        //1.2.1 Free to send Data
        hasToWait = 0;
        SendData (-400, stats);

        int data;
        MPI_Recv (&data, 1, MPI_INT, stats.MPI_SOURCE, PRODUCERLABEL, MPI_COMM_WORLD, &stats);
        printf("%d|Producer|Data Produced |%d| put on position|%d|\n",(int)time(NULL),data, producerIndex );
        //1. Lock buffer mutex
        // sem_wait(&mutex_buffer);
        //2. In the producerIndex on buffer put the data
        pthread_mutex_lock (&mutex_buffer);
        buffer[producerIndex] = data;
        pthread_mutex_unlock (&mutex_buffer);
        //3. increment producerIndex
        producerIndex ++;
        if (producerIndex == MAXBUFFER) producerIndex = 0;
        //4. Unlock buffer mutex
        // sem_post (&mutex_buffer);
        pthread_mutex_lock (&mutex_buffer);
        PrintBuffer();
      }
      else if (ProducedItens (buffer) < MAXBUFFER && (buffer[producerIndex] != 0))
      {
        hasToWait = 1;
        producerIndex ++;
        if (producerIndex == MAXBUFFER) producerIndex = 0;
      }
      pthread_mutex_unlock (&mutex_buffer);
    }

    if (hasToWait)
    {
      SendData (-401, stats);
    }
  }
  // fclose (f);
}

void * ConsumerListener()
{
  MPI_Status stats;
  printf("ConsumerListener\n");
  // FILE *f = fopen ("producer_log_file.txt");
  while (1 != 0)
  {
    int message,
        hasToWait = 1,
        temp;

    MPI_Recv (&message, 1, MPI_INT, MPI_ANY_SOURCE, CONSUMERLABEL, MPI_COMM_WORLD, &stats);
    printf("%d|ConsumerMessage|Receive message| %d | from| %d\n",(int)time(NULL),message, stats.MPI_SOURCE );
    printf("%d|ConsumerMessage|Consumer Index |%d| Producer Index| %d\n",(int)time(NULL),consumerIndex, producerIndex );
    // sem_wait (&mutex_buffer);
    // PrintBuffer();
    // sem_post (&mutex_buffer);

    //1. If message is a consumer request.
    if (message == -102)
    {
      //1.1 If has data to consume
      // sem_post (&mutex_produtor);
      // sem_wait (&mutex_consumidor);
      // printf("consumindo\n");
      pthread_mutex_lock (&mutex_buffer);
      if (ProducedItens(buffer))
      {
        //1.1.1 If the consumer index isn't equal to producer index
        if (consumerIndex != producerIndex || (consumerIndex == producerIndex && ProducedItens(buffer) == MAXBUFFER))
        {
          //1.1.1.1 Lock mutex_buffer to access buffer Data
          //1.1.1.2 If has data in the current consumerIndex is different from zeor
          if (buffer[consumerIndex])
          {
            pthread_mutex_unlock (&mutex_buffer);

            SendData (-400, stats);

            hasToWait = 0;
            //1.1.1.2.1 access buffer and get data in consumerIndex
            temp = buffer[consumerIndex];
            buffer[consumerIndex] = 0;

            SendData (temp, stats);
            //1.1.1.2.1 increment consumerIndex

            consumerIndex ++;
            if (consumerIndex == MAXBUFFER) consumerIndex = 0;

            pthread_mutex_lock (&mutex_buffer);
            PrintBuffer();
          }
          //1.1.1.4 Unlock buffer
        }
      }
      pthread_mutex_unlock (&mutex_buffer);
    }

    if (hasToWait)
    {
      SendData (-401, stats);
    }
    // sem_wait (&mutex_buffer);
    //
    // sem_post (&mutex_buffer);
  }
  // fclose (f);
}

void SendData(int message, MPI_Status stats)
{
	MPI_Send (&message, 1, MPI_INT, stats.MPI_SOURCE, COORDINATORLABEL, MPI_COMM_WORLD);
	printf("%d|SendingData|Sending|%d|To|%d\n",(int)time(NULL), message, stats.MPI_SOURCE);
  WaitFor (TIMETOWAIT);
}

int ProducedItens(int buffer[MAXBUFFER])
{
	int i, total = 0;

	for(i=0; i<MAXBUFFER; i++)
	{
		if(buffer[i])
		{
			total++;
		}
	}

	return total;
}

void PrintBuffer()
{
  int i;
  printf("ZZZZZZZZZ| ");
  for (i = 0; i < MAXBUFFER; i ++ )
  {
    printf("%d\t", buffer[i] );
  }
  printf("\n");
}

void WaitFor (unsigned int secs) {
    int retTime;
    retTime = time(0) + secs;     // Get finishing time.
    while (time(0) < retTime);    // Loop until it arrives.
}
