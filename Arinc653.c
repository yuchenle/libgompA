/* Need a global cleanup for single/critical/parallel directive and Arinc files*/

#include "Arinc653.h"
#include "libgomp.h"
//#include <assert.h>
#include <string.h>


#define MUTEX_NUMBER 10

/*     global variables    */

pthread_mutex_t *EXISTING_MUTEX;									//pointer to existing mutexes
int *AVAILABLE_MUTEX;												//tab of int representing the availability of mutexes: 1 is available, 0 means already owned by
pthread_mutex_t SINGLE_MUTEX;	  									//the 2nd mutex of EXISTING_MUTEX is dedicated to single directive
pthread_mutex_t CRITICAL_MUTEX;										//the 3nd mutex of EXISTING_MUTEX is dedicated to single directive
/* Allocate spaces to store mutexes, processes
 * and their attributes and names...*/
#ifdef LIBGOMP_H
static void __attribute__((constructor))
Arinc_init(void)
{
	printf("Arinc initialization phase\n");
	EXISTING_MUTEX = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)*MUTEX_NUMBER);
	AVAILABLE_MUTEX = (int*)malloc(MUTEX_NUMBER*sizeof(int));
	SINGLE_MUTEX = EXISTING_MUTEX[1];
	/* Initialise the array of pthread_mutex_t */
	for(int i=0;i<MUTEX_NUMBER;i++)
	{
		AVAILABLE_MUTEX[i] = 1;
		pthread_mutex_init(EXISTING_MUTEX+i,NULL);
	}
}

static void __attribute__((destructor)) 
Arinc_destructor(void)
{
	printf("Arinc_destructor called automatically at program's exit\n");
	free(EXISTING_MUTEX);
	free(AVAILABLE_MUTEX);
}
#endif

/* Create a thread (or Arinc process if BY_PTHREAD not defined)
 * return non-zero if an error has occured or zero if successfully created
 */
int Arinc_pthread_create(pthread_t *pt, pthread_attr_t *attr, void *fn, void *data)
{
			
	/* get thread's priority */
	struct sched_param sp;
	int err = pthread_attr_getschedparam(attr,&sp);
	if(err)
	{
		perror("Arinc_pthread_create: pthread_attr_getschedparam error");
		return 1;
	}
	
	/* TODO: affect the priority to A653 process's priority */
	
	/* get thread's stack size */
	size_t sz;
	void *stkaddr;
	err = pthread_attr_getstack(attr, &stkaddr, &sz);
	if(err)
	{
		perror("Arinc_pthread_create: pthread_attr_getstack");
		return 1;
	}
	
	/* TODO: affect the stack size to A653 process's stack size */

	/* get thread's entry point */
	//PROCESS_ATTRIBUTES.ENTRY_POINT = fn;
	
	/* set PROCESS DEADLINE*/	
	//PROCESS_ATTRIBUTES.DEADLINE = SOFT;		//setted arbitrarily, to be updated if no more pthread_create;

	/* set PROCESS PERIOD */
	//PROCESS_ATTRIBUTES.PERIOD = 1000;			//to be updated

	/* set PROCESS TIME_CAPACITY */
	//PROCESS_ATTRIBUTES.TIME_CAPACITY = 3000; 	//to be updated.

	/* set PROCESS NAME */ 
	//PROCESS_ATTRIBUTES.NAME[0] = '\0'; 		//set the name to empty string
	//strncat(PROCESS_ATTRIBUTES.NAME, "whats your name", MAX_NAME_LENGTH-1); 

#ifdef BY_PTHREAD 		
	printf("pthread create called\n");
	return pthread_create(pt, attr, fn, data);

#else
	/* call Arinc's CREATE_PROCESS service */
	
#endif
}


void Arinc_pthread_exit()
{
#ifdef BY_PTHREAD
	pthread_exit(NULL);
#else

#endif
}

/* Use the first mutex in order to obtain an available mutex
 * return index of available mutex, if any,
 * -1 if no mutex is free*/ 
static int __attribute__((unused))
Get_Free_Mutex() 
{
	/* start of mutually exclusive region */
	pthread_mutex_lock(EXISTING_MUTEX);						// wait untill successfully lock the first mutex of the array
	
	for(int i=3;i<MUTEX_NUMBER;i++)							//index starts from 3: 0 is used to distribute mutexes, and 1 is for single directive, 2 is for critical directive
	{
		if(AVAILABLE_MUTEX[i])
		{
			AVAILABLE_MUTEX[i] = 0;
			printf("get_free_mutex has been called and the mutex will be used is %d\n", i);
			pthread_mutex_unlock(EXISTING_MUTEX);
			return i;
		}
	}
	printf("No mutex is free in Get_Free_Mutex function\n");
	pthread_mutex_unlock(EXISTING_MUTEX);
	/* End of mutually exclusive region */
	
	return -1;
}

/* Arinc single start */
/* cf. gccSourcePath/libgo/runtime/thread.c */
_Bool Arinc_compare_and_swap(unsigned long* ptr, unsigned long oldv, unsigned long newv)
{
#ifdef BY_PTHREAD	
	_Bool ret;

	/* start of mutually exclusive region */
	pthread_mutex_lock(&SINGLE_MUTEX);
	if(*ptr != oldv)
		ret = 0;
	else
	{
		*ptr = newv;
		ret = 1;
	}
		
	pthread_mutex_unlock(&SINGLE_MUTEX);
	/* end of mutually exclusive region */
		
	return ret;
	
#else
	
#endif 
}


/* Arinc critical start*/
void Arinc_gomp_mutex_lock()
{
#ifdef BY_PTHREAD
	pthread_mutex_lock(&CRITICAL_MUTEX);
#else
	
#endif
}

/* Arinc critical end */
void Arinc_gomp_mutex_unlock(gomp_mutex_t *mtx)
{
#ifdef BY_PTHREAD
	pthread_mutex_unlock(&CRITICAL_MUTEX);
#else
	/* what to do if Arinc services are demanded 
	 * -> identify the mutex previously locked ->global variable so that each critical may have different mutex locked => performance issue? lead to "famine" of mutex?
	 * -> set its availability to 1
	 * -> release the mutex*/
#endif
}

