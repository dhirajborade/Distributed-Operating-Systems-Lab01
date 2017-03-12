/*  main.c  - main */

#include <xinu.h>

pid32 producer_id;
pid32 consumer_id;
pid32 timer_id;

int32 consumed_count = 0;
const int32 CONSUMED_MAX = 100;

/* Define your circular buffer structure and semaphore variables here */

#define Buffer_Size 10

/*Circular Buffer Structure*/

typedef struct circularQueue_s
{
	int32 front;
    	int32 rear;
    	int32 BufferItems;
    	int32 data[Buffer_Size];
} circularQueue_t;

circularQueue_t myQueue;

process consumer(sid32, sid32, sid32), producer(sid32, sid32, sid32);
void initializeQueue(circularQueue_t *theQueue);
int32 isEmpty(circularQueue_t *theQueue);
int32 putItem(circularQueue_t *theQueue, int32 theItemValue);
int32 getItem(circularQueue_t *theQueue, int32 *theItemValue);

/*Circular Buffer Initialization*/

void initializeQueue(circularQueue_t *theQueue)
{
    	int32 j;
    	theQueue->BufferItems  =  0;
    	theQueue->front       =  0;
    	theQueue->rear        =  0;
    	for(j=0; j<Buffer_Size; j++)
    	{
        	theQueue->data[j] = 0;
    	}        
    	return;
}

/*Check whether Buffer is Empty*/

int32 isEmpty(circularQueue_t *theQueue)
{
    	if(theQueue->BufferItems==0)
        	return(1);
    	else
        	return(0);
}

/*Insert item in the rear of the Buffer*/

int32 putItem(circularQueue_t *theQueue, int32 theItemValue)
{
    	if(theQueue->BufferItems>=Buffer_Size)
    	{
        	kprintf("The Circular Queue Buffer is Full\n");
        	return(-1);
    	}
    	else
    	{
        	theQueue->BufferItems++;
        	theQueue->data[theQueue->rear] = theItemValue;
        	theQueue->rear = (theQueue->rear+1)%Buffer_Size;
    	}
}

/*Remove item from the front of the Buffer*/

int32 getItem(circularQueue_t *theQueue, int32 *theItemValue)
{
    	if(isEmpty(theQueue))
    	{
        	kprintf("\nThe Circular Queue Buffer is Empty\n");
        	return(-1);
    	}
    	else
    	{
        	*theItemValue=theQueue->data[theQueue->front];
        	theQueue->front=(theQueue->front+1)%Buffer_Size;
        	theQueue->BufferItems--;
        	return(0);
    	}
}

/* Place your code for entering a critical section here */
void mutex_acquire(sid32 mutex)
{
	wait(mutex);
}

/* Place your code for leaving a critical section here */
void mutex_release(sid32 mutex)
{
	signal(mutex);	
}

/* Place the code for the buffer producer here */
process producer(sid32 cirQueueFull, sid32 cirQueueEmpty, sid32 process_mutex)
{
	int32 i=0;
	while(1)
	{
		mutex_acquire(cirQueueEmpty);		
		mutex_acquire(process_mutex);
		putItem(&myQueue, i);
		i++;
		kprintf("Value Produced = %d\n", i);
		mutex_release(process_mutex);
		mutex_release(cirQueueFull);
			
	}
	return OK;
}

/* Place the code for the buffer consumer here */
process consumer(sid32 cirQueueFull, sid32 cirQueueEmpty, sid32 process_mutex)
{
	int32 n;
	while(1)
	{
		mutex_acquire(cirQueueFull);		
		mutex_acquire(process_mutex);
		getItem(&myQueue, &n);
		kprintf("Value Consumed = %d\n", n);
		consumed_count += 1;
		mutex_release(process_mutex);
		mutex_release(cirQueueEmpty);	
	}	

	/* Every time your consumer consumes another buffer element,
	 * make sure to include the statement:
	 *   consumed_count += 1;
	 * this will allow the timing function to record performance */
	/* */

	return OK;
}


/* Timing utility function - please ignore */
process time_and_end(void)
{
	int32 times[5];
	int32 i;

	for (i = 0; i < 5; ++i)
	{
		times[i] = clktime_ms;
		yield();

		consumed_count = 0;
		while (consumed_count < CONSUMED_MAX * (i+1))
		{
			yield();
		}

		times[i] = clktime_ms - times[i];
	}

	kill(producer_id);
	kill(consumer_id);

	for (i = 0; i < 5; ++i)
	{
		kprintf("TIME ELAPSED (%d): %d\n", (i+1) * CONSUMED_MAX, times[i]);
	}
}

process	main(void)
{	
	recvclr();

	/* Create the shared circular buffer and semaphores here */
	
	initializeQueue(&myQueue);

	sid32 cirQueueFull, cirQueueEmpty, process_mutex;
	cirQueueFull = semcreate(0);
	cirQueueEmpty = semcreate(Buffer_Size);
	process_mutex = semcreate(1);
	
	producer_id = create(producer, 4096, 50, "producer", 3, cirQueueFull, cirQueueEmpty, process_mutex);
	consumer_id = create(consumer, 4096, 50, "consumer", 3, cirQueueFull, cirQueueEmpty, process_mutex);
	timer_id = create(time_and_end, 4096, 50, "timer", 0);

	resched_cntl(DEFER_START);
	resume(producer_id);
	resume(consumer_id);
	/* Uncomment the following line for part 3 to see timing results */
	resume(timer_id);
	resched_cntl(DEFER_STOP);

	return OK;
}

/*

Timing Data:

TIME ELAPSED (100): 339
TIME ELAPSED (200): 713
TIME ELAPSED (300): 1066
TIME ELAPSED (400): 1410
TIME ELAPSED (500): 1852

*/
