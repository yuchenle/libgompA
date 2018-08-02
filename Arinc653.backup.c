#include "Arinc653.h"
#include "libgomp.h"
//#include <assert.h>
#include <string.h>



/*************From Chenle *************/
/*     global variables    */
OPERATING_MODE_TYPE CURRENT_STATE; 						//variable indicating the state of the partition
PARTITION_STATUS_TYPE CURRENT_PARTITION_STATUS; 		//the current status of the partition
MUTEX *EXISTING_MUTEX;									//pointer to existing mutexes
PROCESS*EXISTING_PROCESS;								//pointer to existing processes
long MUTEX_NUMBER;										//the current number of mutex
long PROCESS_NUMBER;									//the current number of process in the partition
int *AVAILABLE_MUTEX;									//tab of int representing the availability of mutexes


/* Allocate spaces to store mutexes, processes
 * and their attributes and names...*/
#ifdef LIBGOMP_H
static void __attribute__((constructor))
Arinc_init(void)
{
	printf("Arinc initialization phase\n");
	MUTEX_NUMBER = 0;
	PROCESS_NUMBER = 0;
	CURRENT_STATE = NORMAL;   //set initial state arbitrarily
	EXISTING_MUTEX = (MUTEX*)malloc(sizeof(MUTEX)*MAX_NUMBER_OF_MUTEXES);
	EXISTING_PROCESS = (PROCESS*)malloc(sizeof(PROCESS)*MAX_NUMBER_OF_PROCESSES);
	AVAILABLE_MUTEX = (int*)malloc(sizeof(int)*MAX_NUMBER_OF_MUTEXES);
	/* As mutexes are not created yet, none is available */
	for(int i=0;i<MAX_NUMBER_OF_MUTEXES;i++)
		AVAILABLE_MUTEX[i] = 0;
	
	
	/* Set the CURRENT_PARTITION_STATUS with some arbitrarily settings */ 	
	CURRENT_PARTITION_STATUS.PERIOD = 10000;
	CURRENT_PARTITION_STATUS.DURATION = 1000;
	CURRENT_PARTITION_STATUS.IDENTIFIER = 1;
	CURRENT_PARTITION_STATUS.LOCK_LEVEL = 2;
	CURRENT_PARTITION_STATUS.OPERATING_MODE = WARM_START;
	CURRENT_PARTITION_STATUS.START_CONDITION = NORMAL_START;
	CURRENT_PARTITION_STATUS.NUM_ASSIGNED_CORES = 5;
	
}

static void __attribute__((destructor)) 
Arinc_destructor(void)
{
	printf("Arinc_destructor called automatically at program's exit\n");
	free(EXISTING_MUTEX);
	free(EXISTING_PROCESS);
	free(AVAILABLE_MUTEX);
}
#endif

/*function tests if the new_name of process is already taken,
 * return the index of corresponding process in EXISTING_PROCESS if it's already in use
 * return NULL_PROCESS_ID(=0) if not*/
int PROCESS_NAME_TAKEN(PROCESS *array, PROCESS_NAME_TYPE new_name)
{
	for(int i=0;i<PROCESS_NUMBER;i++)
	{
		if(!(strncmp(array[i].status.ATTRIBUTES.NAME, new_name, MAX_NAME_LENGTH)))	//if 2 names are equal
			return i;
	}
	return -1;
}

/*function tests if the new_name of the mutex is already taken,
 * return index of corresponding MUTEX in EXISTING_MUTEX if it's in use
 * return 0 if not */
int MUTEX_NAME_TAKEN(MUTEX *array, MUTEX_NAME_TYPE new_name)
{
	 for(int i=0;i<MUTEX_NUMBER;i++)
	 {
		 if(!(strncmp(array[i].name, new_name, MAX_NAME_LENGTH)))
			return i;
	 }
	 return -1;
}


/* Create a thread (or Arinc process if BY_PTHREAD not defined)
 * return non-zero if an error has occured or zero if successfully created
 */
int Arinc_pthread_create(pthread_t *pt, pthread_attr_t *attr, void *fn, void *data)
{
	PROCESS_ATTRIBUTE_TYPE PROCESS_ATTRIBUTES = EXISTING_PROCESS[PROCESS_NUMBER].status.ATTRIBUTES;
		
	/* get thread's priority */
	struct sched_param sp;
	int err = pthread_attr_getschedparam(attr,&sp);
	if(err)
	{
		perror("Arinc_pthread_create: pthread_attr_getschedparam error");
		return 1;
	}
	PROCESS_ATTRIBUTES.BASE_PRIORITY = sp.sched_priority;
	
	/* get thread's stack size */
	size_t sz;
	void *stkaddr;
	err = pthread_attr_getstack(attr, &stkaddr, &sz);
	if(err)
	{
		perror("Arinc_pthread_create: pthread_attr_getstack");
		return 1;
	}
	PROCESS_ATTRIBUTES.STACK_SIZE = sz;

	/* get thread's entry point */
	PROCESS_ATTRIBUTES.ENTRY_POINT = fn;
	
	/* set PROCESS DEADLINE*/	
	PROCESS_ATTRIBUTES.DEADLINE = SOFT;		//setted arbitrarily, to be updated if no more pthread_create;

	/* set PROCESS PERIOD */
	PROCESS_ATTRIBUTES.PERIOD = 1000;			//to be updated

	/* set PROCESS TIME_CAPACITY */
	PROCESS_ATTRIBUTES.TIME_CAPACITY = 3000; 	//to be updated.

	/* set PROCESS NAME */ 
	PROCESS_ATTRIBUTES.NAME[0] = '\0'; 		//set the name to empty string
	strncat(PROCESS_ATTRIBUTES.NAME, "whats your name", MAX_NAME_LENGTH-1); 

#ifdef BY_PTHREAD 		
	printf("pthread create called\n");
	return pthread_create(pt, attr, fn, data);

#else
	/* call CREATE_PROCESS service */
	RETURN_CODE_TYPE *RETURN_CODE;
	PROCESS_ID_TYPE ID = EXISTING_PROCESS[PROCESS_NUMBER].id;
	CREATE_PROCESS(&PROCESS_ATTRIBUTES, &ID, RETURN_CODE);
	
	return *RETURN_CODE;
#endif
}


/* TODO: function that should be completed with Arinc653 services
 * it's a simple wrapper by now
 */
void Arinc_pthread_exit()
{
#ifdef BY_PTHREAD
	pthread_exit(NULL);
#else
	STOP_SELF();
#endif
}

/*Use the first mutex in order to obtain an available mutex
 * return index of available mutex, if any,
 * -1 if no mutex is free*/ 
int getFreeMutex(MUTEX *EXISTE)
{
	SYSTEM_TIME_TYPE time = 500; 					//setted arbitrarily
	RETURN_CODE_TYPE RETURN_CODE;
	/* Try to acquire the first MUTEX */
	ACQUIRE_MUTEX(0,time, &RETURN_CODE);			
	
	/* if successfully acquired, START OF CRITICAL ZONE*/
	if(RETURN_CODE == NO_ERROR)
	{
		for(int i=1;i<MAX_NUMBER_OF_MUTEXES;i++)
		{
			if(AVAILABLE_MUTEX[i])
			{
				AVAILABLE_MUTEX[i] = 0;				//this mutex becomes unavailable
				
				/* End of CRITICAL ZONE*/
				RELEASE_MUTEX(0, &RETURN_CODE);
				return i;
			}
		}
	}else
	{
		/* No release action needed since the acquisition has failed */
		perror("Error ACQUIRE_MUTEX in getFreeMutex, Arinc653.c");
		exit(EXIT_FAILURE);
	}
	
	/* End of CRITICAL ZONE*/
	RELEASE_MUTEX(0, &RETURN_CODE);	
	return -1;
}

_Bool Arinc_compare_and_swap(unsigned long* ptr, unsigned long oldv, unsigned long newv)
{
	printf("Arinc_compare_and_swap (single) called\n");
#ifdef BY_PTHREAD
	return __sync_bool_compare_and_swap(ptr, oldv, newv);
#else
	/* Get the index of an available mutex */
	static int single_Arinc_mutex = getFreeMutex(EXISTING_MUTEX);
	
	if(single_Arinc_mutex == -1) 
	{
		perror("No mutex available in Arinc_compare_and_swap\n");
		exit(EXIT_FAILURE);
	}
	
	/* Acquire this mutex */
	MUTEX *mtx = EXISTING_MUTEX[single_Arinc_mutex];
	SYSTEM_TIME_TYPE time = 500;				//setted arbitrarily
	RETURN_CODE_TYPE RETURN_CODE;
	ACQUIRE_MUTEX(mtx->id, time, &RETURN_CODE);
	
	/* Start of CRITICAL ZONE */
	if(RETURN_CODE == NO_ERROR)
	{
		if(*ptr == oldv)
		{
			*ptr = newv;
			ret = true;
		}else
		{
			ret = false;
		}
	}else
	{
		perror("ACQUIRE_MUTEX error in Arinc_compare_and_swap");
		exit(EXIT_FAILURE);
	}
	
	RELEASE_MUTEX(mtx->id, &RETURN_CODE);
	AVAILABLE_MUTEX[single_Arinc_mutex] = 1;
	return ret;		
#endif //ifdef BY_THREAD	
}


/* Arinc critical start*/
void Arinc_gomp_mutex_lock(gomp_mutex_t *mtx)
{
	printf("Arinc_gomp_mutex_lock (critical_start) called\n");
#ifdef BY_PTHREAD
	gomp_mutex_lock(mtx);
#else
	/* Get the index of an available mutex */
	static int unname_critical_Arinc_mutex = getFreeMutex(EXISTING_MUTEX);
	
	if(unname_critical_Arinc_mutex == -1)
	{
		perror("No mutex available in Arinc_gomp_mutex_lock in unnamed critical region\n");
		exit(EXIT_FAILURE);
	}
	
	/* Acquire this mutex */
	MUTEX *mtx = EXISTING_MUTEX[unname_critical_Arinc_mutex];
	SYSTEM_TIME_TYPE time = 500;										//setted arbitrarily
	RETURN_CODE_TYPE ret = (RETURN_CODE_TYPE*)malloc(sizeof(RETURN_CODE_TYPE));	
	ACQUIRE_MUTEX(mtx->id, time, ret);
	
	/* Start of CRITICAL ZONE */
	if(*ret != NO_ERROR)
	{
		perror("ACQUIRE_MUTEX in Arinc_compare_and_swap");
		exit(EXIT_FAILURE);
	}
	
	AVAILABLE_MUTEX[single_Arinc_mutex] = 1;
	RELEASE_MUTEX(mtx->id, &ret);
	//assert(*ret == NO_ERROR);											//abort if release mutex fails
	return ret;		
#endif
}

/* Arinc critical end */
void Arinc_gomp_mutex_unlock(gomp_mutex_t *mtx)
{
	printf("Arinc_gomp_mutex_unlock (critical end) called\n");
#ifdef BY_PTHREAD
	gomp_mutex_unlock(mtx);
#else
	/* what to do if Arinc services are demanded 
	 * -> identify the mutex previously locked ->global variable so that each critical may have different mutex locked => performance issue? lead to "famine" of mutex?
	 * -> set its availability to 1
	 * -> release the mutex*/
#endif
}
/*************From Chenle *************/


/********************* Process Services **********************/


void GET_PROCESS_ID (PROCESS_NAME_TYPE PROCESS_NAME, PROCESS_ID_TYPE *PROCESS_ID, RETURN_CODE_TYPE *RETURN_CODE )
{
	int idx = PROCESS_NAME_TAKEN(EXISTING_PROCESS, PROCESS_NAME);
	if(idx==-1)															//if there is no process named PROCESS_NAME
	{
		*RETURN_CODE = INVALID_CONFIG;
		return;
	}
	
	*PROCESS_ID = EXISTING_PROCESS[idx].id;
	*RETURN_CODE = NO_ERROR;		
}

void GET_PROCESS_STATUS(PROCESS_ID_TYPE PROCESS_ID, PROCESS_STATUS_TYPE *PROCESS_STATUS, RETURN_CODE_TYPE *RETURN_CODE)
{
	for(int i=0;i<PROCESS_NUMBER;i++)
	{
		if(EXISTING_PROCESS[i].id == PROCESS_ID)
		{
			*PROCESS_STATUS = EXISTING_PROCESS[i].status;
			*RETURN_CODE = NO_ERROR;
			return;
		}
	}
	
	/* PROCESS_ID doesn't correspond to an existing process */
	*RETURN_CODE = INVALID_PARAM;
}



void CREATE_PROCESS (PROCESS_ATTRIBUTE_TYPE *ATTRIBUTES, PROCESS_ID_TYPE *PROCESS_ID, RETURN_CODE_TYPE *RETURN_CODE)
{
	/* insufficient storage capacity for the creation */
	// TODO: with A653 memory tests
	
	/* maximum number of processes have been created */
	if(PROCESS_NUMBER == MAX_NUMBER_OF_PROCESSES)
	{
		*RETURN_CODE = INVALID_CONFIG;
		return;
	}
		
	/* if the name has been already in use */
	if(PROCESS_NAME_TAKEN(EXISTING_PROCESS, ATTRIBUTES->NAME) != -1)
	{
		*RETURN_CODE = NO_ACTION;
		return;
	}
		
	/* if the stack size is out of range */
	STACK_SIZE_TYPE max_stack_size = -1;					//unsigned type => -1 is max value
	if(ATTRIBUTES->STACK_SIZE > max_stack_size)
	{
		*RETURN_CODE = INVALID_PARAM;
		return;
	}
		
	/* when the period is out of range */
	SYSTEM_TIME_TYPE max_time = -1;
	if(ATTRIBUTES->PERIOD > max_time)
	{
		*RETURN_CODE = INVALID_PARAM;
		return;
	}
	
	/* when (ATTRIBUTES.PERIOD is finite (i.e., periodic process) and
	 * ATTRIBUTES.PERIOD is not an integer multiple of partition period defined
	 * in the configuration tables)
	 * 
	 * partition period is not gaven, ignore for now */
	
	
	/* when the TIME_CAPACITY out of range */
	if(ATTRIBUTES->TIME_CAPACITY > max_time)
	{
		*RETURN_CODE = INVALID_PARAM;
		return;
	}
	
	/* when the process has a period and the its time capacity 
	 * is greater than the period */
	if(ATTRIBUTES->TIME_CAPACITY > ATTRIBUTES->PERIOD)
	{
		*RETURN_CODE = INVALID_PARAM;
		return;
	}
	
	/* when the partition's state is normal */
	if(CURRENT_STATE == NORMAL)
	{
		*RETURN_CODE = INVALID_MODE;
		return;
	}
	
	/* if everythings is fine */
	
	*PROCESS_ID = PROCESS_NUMBER ++;	
	
	printf("CREATE_PROCESS called, but underlying syscalls are not implemented\n");
	
}

void SET_PRIORITY(PROCESS_ID_TYPE PROCESS_ID, PRIORITY_TYPE PRIORITY, RETURN_CODE_TYPE *RETURN_CODE)
{
	/* if the priority is out of range */
	if(PRIORITY > MAX_PRIORITY_VALUE || PRIORITY < MIN_PRIORITY_VALUE)
	{
		*RETURN_CODE = INVALID_PARAM;
		return;
	}
	
	for(int i=0;i<PROCESS_NUMBER;i++)
	{
		if(EXISTING_PROCESS[i].id == PROCESS_ID)
		{
			/* if the state of the current process is dormant */
			if(EXISTING_PROCESS[i].status.PROCESS_STATE == DORMANT)
			{
				*RETURN_CODE = INVALID_MODE;
				return;
			}
			
			MUTEX_ID_TYPE MUTEX_ID;
			GET_PROCESS_MUTEX_STATE(PROCESS_ID, &MUTEX_ID, RETURN_CODE); // test if the current process is owning a mutex
			/* if the specified process owns no mutex */
			if(MUTEX_ID == NO_MUTEX_OWNED)
				EXISTING_PROCESS[i].status.CURRENT_PRIORITY = PRIORITY;
			else
				EXISTING_PROCESS[i].retained_priority = PRIORITY;
			
			/* Rescheduling processes according to the new priority */
			*RETURN_CODE = NO_ERROR;
			return;
		}			
	}
}


void SUSPEND_SELF(SYSTEM_TIME_TYPE TIME_OUT, RETURN_CODE_TYPE *RETURN_CODE)
{
	PROCESS_ID_TYPE PROCESS_ID;
	GET_MY_ID(&PROCESS_ID, RETURN_CODE); 								//get the ID of current process
	
	MUTEX_ID_TYPE MUTEX_ID;
	GET_PROCESS_MUTEX_STATE(PROCESS_ID, &MUTEX_ID, RETURN_CODE);
	
	/* if current process owns a mutex or is error handler process */
	if(MUTEX_ID != NO_MUTEX_OWNED || PROCESS_ID == NULL_PROCESS_ID)
	{
		*RETURN_CODE = INVALID_MODE;
		return;
	}
	
	/* if the TIME_OUT is out of range */
	SYSTEM_TIME_TYPE lb = 1;
	if(TIME_OUT < lb<<63 || TIMED_OUT > 0x7FFFFFFFFFFFFFFF)
	{
		*RETURN_CODE = INVALID_PARAM;
		return;
	}
	
	PROCESS_STATUS_TYPE PROCESS_STATUS;
	GET_PROCESS_STATUS(PROCESS_ID, &PROCESS_STATUS, RETURN_CODE);
	
	/* if current process is periodic */
	if(PROCESS_STATUS.ATTRIBUTES.PERIOD != APERIODIC)
	{
		*RETURN_CODE = INVALID_MODE;
		return;
	}
	
	if(TIME_OUT == 0)
	{
		*RETURN_CODE = NO_ERROR;
		return;
	}else
	{
		PROCESS_STATUS.PROCESS_STATE = WAITING;
		/* cf.3.3.2.5.
		 * -> if TIME_OUT is not infinite:
		 * 		-> initiate a time counter with duration TIME_OUT;
		 * -> process scheduling
		 * -> if (expiration of time_out period)
		 * 		-> *RETURN_CODE = TIMED_OUT;
		 * -> else //resumed from a RESUME service call from other process
		 * 		-> *RETURN_CODE = NO_ERROR;*/
	 }
}


void SUSPEND(PROCESS_ID_TYPE PROCESS_ID, RETURN_CODE_TYPE *RETURN_CODE)
{
	MUTEX_ID_TYPE MUTEX_ID;
	GET_PROCESS_MUTEX_STATE(PROCESS_ID, &MUTEX_ID, RETURN_CODE);
	/* if the specified process is holding a mutex //TODO: or waiting on a mutex's queue */
	if(MUTEX_ID != NO_MUTEX_OWNED)
	{
		*RETURN_CODE = INVALID_MODE;
		return;
	}
	
	PROCESS_ID_TYPE MY_ID;
	GET_MY_ID(&MY_ID, RETURN_CODE);								
	/* if the specified process is the current process */
	if(PROCESS_ID == MY_ID)
	{
		*RETURN_CODE = INVALID_PARAM;
		return;
	}
	
	for(int i=0;i<PROCESS_NUMBER;i++)
	{
		/* once the specified process found */
		if(EXISTING_PROCESS[i].id == PROCESS_ID)
		{
			PROCESS proc = EXISTING_PROCESS[i];
			/* if the state of specified process is DORMANT or FAULTED */
			if(proc.status.PROCESS_STATE == DORMANT || proc.status.PROCESS_STATE == FAULTED)
			{
				*RETURN_CODE = INVALID_MODE;
				return;
			}
			/* if the specified process is periodic */
			if(proc.status.ATTRIBUTES.PERIOD != APERIODIC)
			{
				*RETURN_CODE = INVALID_MODE;
				return;
			}
			
			/* cf. 3.3.2.6 SUSPEND
			 * ->if the specified process has already been suspended, either through SUSPEND or SUSPEND_SELF
			 * 		-> *RETURN_CODE = NO_ACTION;
			 * 		-> return;
			 * ->else
			 * 		-> *RETURN_CODE = NO_ERROR;
			 * 		-> return;*/
		}
	}
	
	/* if the specified process doesn't correspond to any existing processes */
	*RETURN_CODE = INVALID_PARAM;
}
 
 
void RESUME(PROCESS_ID_TYPE PROCESS_ID, RETURN_CODE_TYPE *RETURN_CODE)
{
	for(int i=0;i<PROCESS_NUMBER;i++)
	{
		if(PROCESS_ID == EXISTING_PROCESS[i].id)
		{
			PROCESS proc = EXISTING_PROCESS[i];
			/* if the state of the specified process is dormant */
			if(proc.status.PROCESS_STATE == DORMANT)
			{
				*RETURN_CODE = INVALID_MODE;
				return;
			}
			/* if the process is periodic and is not FAULTED */
			if(proc.status.ATTRIBUTES.PERIOD != APERIODIC && proc.status.PROCESS_STATE != FAULTED)
			{
				*RETURN_CODE = INVALID_MODE;
				return;
			}
			/*TODO: 
			 * -> if the process is not a suspended process and is not faulted: 
			 * 		//HOW TO identify whether a process is suspended?
			 * ->		*RETURN_CODE = NO_ACTION;
			 * ->		return;
			 * -> if the process was suspended with a time_out
			 * ->		stop the time counter;
			 * -> if the process is not waiting on a process queue or TIMED_WAIT delay
			 * -> 		proc.status.PROCESS_STATE = READY;
			 * ->		rescheduling of processes
			 * ->	    *RETURN_CODE = NO_ERROR;*/
		 }
	 }
	 /* if PROCESS_ID doesn't correspond to any existing process */
	 *RETURN_CODE = INVALID_PARAM;			
}

/* TODO: function to be implemented with Arinc653 
 * services while respecting its restrictions
 */
void STOP_SELF(void)
{
	PROCESS_ID_TYPE PROCESS_ID;
	MUTEX_ID_TYPE MUTEX_ID;
	RETURN_CODE_TYPE RETURN_CODE;
	
	GET_MY_ID(&PROCESS_ID, &RETURN_CODE);
	GET_PROCESS_MUTEX_STATE(PROCESS_ID, &MUTEX_ID, &RETURN_CODE);
	
	/* if current process owns the preemption lock mutex */
	if(MUTEX_ID == PREEMPTION_LOCK_MUTEX)
	{
		CURRENT_PARTITION_STATUS.LOCK_LEVEL = 0;
		
		//TODO: this call should not perform PREEMPTION_LOCK error check
		RESET_MUTEX(PROCESS_ID, MUTEX_ID, &RETURN_CODE); 	
	}
	
	PROCESS_STATUS_TYPE PROCESS_STATUS;
	GET_PROCESS_STATUS(PROCESS_ID, &PROCESS_STATUS, &RETURN_CODE);
	PROCESS_STATUS.PROCESS_STATE = DORMANT;
	
	/*TODO: cf. 3.3.2.8
	 * -> prevent the specified process from causing a deadline overrun fault
	 * -> check for process rescheduling;
	 * -> A process interrupted by the error handler process may continue running,
	 * -> especially if it had preemption locked*/	
}

void STOP(PROCESS_ID_TYPE PROCESS_ID, RETURN_CODE_TYPE *RETURN_CODE)
{
	PROCESS_ID_TYPE MY_ID;
	GET_MY_ID(&MY_ID, RETURN_CODE);
	
	/* if the specified process is the current process */
	if(PROCESS_ID == MY_ID)
	{
		*RETURN_CODE = INVALID_PARAM;
		return;
	}
	
	for(int i=0;i<PROCESS_NUMBER;i++)
	{
		if(EXISTING_PROCESS[i].id == PROCESS_ID)
		{
			PROCESS proc = EXISTING_PROCESS[i];
			
			/* if the specified process already has DORMANT as state */
			if(proc.status.PROCESS_STATE == DORMANT)
			{
				*RETURN_CODE = NO_ACTION;
				return;
			}
			
			MUTEX_ID_TYPE MUTEX_ID;
			GET_PROCESS_MUTEX_STATE(PROCESS_ID, &MUTEX_ID, RETURN_CODE);
			
			/* if the target process is holding the preemption lock mutex */
			if(MUTEX_ID == PREEMPTION_LOCK_MUTEX)
			{
				CURRENT_PARTITION_STATUS.LOCK_LEVEL = 0;
				// TODO: this call should not perform the PREEMPTION LOCK MUTEX error check
				RESET_MUTEX(PROCESS_ID, MUTEX_ID, RETURN_CODE);
			}
			proc.status.PROCESS_STATE = DORMANT;
			
			/* cf. 3.3.2.9 
			 * -> if specified process is waiting in a process queue
			 * ->		remove the process from the queue
			 * -> stop time counters associated with the specified process
			 * -> prevent the specified process from causing a deadline overrun fault
			 * -> process rescheduling
			 * -> *RETURN_CODE = NO_ERROR;*/
			 
		}
	}
	
	/* if the PROCESS_ID is not any of existing process */
	*RETURN_CODE = INVALID_PARAM;
}


void START(PROCESS_ID_TYPE PROCESS_ID, RETURN_CODE_TYPE *RETURN_CODE)
{
	for(int i=0;i<PROCESS_NUMBER;i++)
	{
		if(EXISTING_PROCESS[i].id == PROCESS_ID)
		{
			PROCESS proc = EXISTING_PROCESS[i];
			
			/* if the state of the specified process is not DORMANT */
			if(proc.status.PROCESS_STATE != DORMANT)
			{
				*RETURN_CODE = NO_ACTION;
				return;
			}
			
			/*cf. 3.3.2.10
			 * -> if the partition is in NORMAL mode, the process's 
			 * 	  deadline expiration time and next release point are calculated
			 * -> if DEADLINE_TIME calculation is out of range
			 * ->  		*RETURN_CODE = INVALID_CONFIG;
			 * -> if the process is an aperiodic process ...
			 * ...
			 * ...
			 */ 
			 
		}
	}
	
	/* if PROCESS_ID is not corresponding to any existing PROCESS */
	*RETURN_CODE = INVALID_PARAM;
}	


void DELAYED_START(PROCESS_ID_TYPE PROCESS_ID, SYSTEM_TIME_TYPE DELAY_TIME, RETURN_CODE_TYPE *RETURN_CODE)
{
	SYSTEM_TIME_TYPE lb = 1;
	
	/*if DELAY_TIME is out of range */
	if(DELAY_TIME < lb<<63 || DELAY_TIME > 0x7FFFFFFFFFFFFFFF)
	{
		*RETURN_CODE = INVALID_PARAM;
		return;
	}
	
	/* if the DELAY_TIME is infinite */
	if(DELAY_TIME == INFINITE_TIME_VALUE)
	{
		*RETURN_CODE = INVALID_PARAM;
		return;
	}
	
	for(int i=0;i<PROCESS_NUMBER;i++)
	{
		if(EXISTING_PROCESS[i].id == PROCESS_ID)
		{
			PROCESS proc = EXISTING_PROCESS[i];
			
			/* if the process's current state is not DORMANT */
			if(proc.status.PROCESS_STATE != DORMANT)
			{
				*RETURN_CODE = NO_ACTION;
				return;
			}
			
			/* if the process is periodic and the delay is greater or equal to the process's period */
			if(proc.status.ATTRIBUTES.PERIOD != APERIODIC && DELAY_TIME >= proc.status.ATTRIBUTES.PERIOD)
			{
				*RETURN_CODE = INVALID_PARAM;
				return;
			}
			
			/* TODO: cf.3.3.2.11
			 * -> if the partition is in NORMAL mode,
			 * ->	 calculate the DEADLINE_TIME 
			 * -> if the calculation is out of range
			 * ->	 *RETURN_CODE = INVALID_CONFIG;*/
			 
			 if(proc.status.ATTRIBUTES.PERIOD == APERIODIC)
			 {
				 proc.status.CURRENT_PRIORITY = proc.status.ATTRIBUTES.BASE_PRIORITY;
				 
				 // TODO: reset context and stack
				  if (CURRENT_PARTITION_STATUS.OPERATING_MODE == NORMAL)
				  {
					  if(DELAY_TIME == 0)
					  {
						  proc.status.PROCESS_STATE = READY;
						  //TODO: proc.status.DEADLINE_TIME = CURRENT_SYSTEM_TIME + TIME_CAPACITY
					  }else
					  {
						  proc.status.PROCESS_STATE = WAITING;
						  //TODO: proc.status.DEADLINE_TIME = CURRENT_SYSTEL_TIME + TIME_CAPACITY
						  //TODO: create a time counter initialized to DELAY_TIME
					  }
				  }
				  /* TODO: 
				   * -> process rescheduling
				   * -> if CURRENT_PARTITION_MODE == COLD_START or WARM START
				   * -> ...
				   * -> ...
				   */ 
			 }
		  }
	}
	
	/* if the PROCESS_ID doesn't identify an existing process */
	*RETURN_CODE = INVALID_PARAM;
}


void LOCK_PREEMPTION(LOCK_LEVEL_TYPE *LOCK_LEVEL, RETURN_CODE_TYPE *RETURN_CODE)
{
	PROCESS_ID_TYPE PROCESS_ID;
	GET_MY_ID(&PROCESS_ID, RETURN_CODE);
	
	/* if OPERATING_MODE is not NORMAL or current process is error handler process */
	if(CURRENT_PARTITION_STATUS.OPERATING_MODE != NORMAL || PROCESS_ID == NULL_PROCESS_ID)
	{
		*LOCK_LEVEL = CURRENT_PARTITION_STATUS.LOCK_LEVEL;
		*RETURN_CODE = NO_ACTION;
		return;
	}else
	{
		/* TODO: This call should ignore PREEMPTION_LOCK_MUTEX error check */
		ACQUIRE_MUTEX(PREEMPTION_LOCK_MUTEX, INFINITE_TIME_VALUE, RETURN_CODE);
		MUTEX_STATUS_TYPE MUTEX_STATUS;
		
		/* cf.3.3.2.12
		 * the RETURN_CODE is what returned in ACQUIRE_MUTEX service
		 * that's why we create new TMP_RETURN_CODE*/
		RETURN_CODE_TYPE TMP_RETURN_CODE;
		
		/* TODO: This call should ignore PREEMPTION_LOCK_MUTEX error check */
		GET_MUTEX_STATUS(PREEMPTION_LOCK_MUTEX, &MUTEX_STATUS, &TMP_RETURN_CODE);
		
		CURRENT_PARTITION_STATUS.LOCK_LEVEL = MUTEX_STATUS.LOCK_COUNT;
		*LOCK_LEVEL = CURRENT_PARTITION_STATUS.LOCK_LEVEL;
	}
}

void UNLOCK_PREEMPTION(LOCK_LEVEL_TYPE *LOCK_LEVEL, RETURN_CODE_TYPE *RETURN_CODE)
{
	PROCESS_ID_TYPE PROCESS_ID;
	GET_MY_ID(&PROCESS_ID, RETURN_CODE);
	
	/* if the OPERATING_MODE is not NORMAL or the current process 
	 * is the error handler process or 
	 * TODO: no process has preemption locked */
	 if(CURRENT_PARTITION_STATUS.OPERATING_MODE != NORMAL || PROCESS_ID == NULL_PROCESS_ID)
	 {
		 *RETURN_CODE = NO_ACTION;
		 return;
	 }else
	 {
		 MUTEX_ID_TYPE MUTEX_ID;
		 PROCESS_ID_TYPE PROCESS_ID;
		 
		 /* get the ID of the current process */
		 GET_MY_ID(&PROCESS_ID, RETURN_CODE);
		 
		 /* get the MUTEX owned by the current process */
		 GET_PROCESS_MUTEX_STATE(PROCESS_ID, &MUTEX_ID, RETURN_CODE);
		 
		 /* if the MUTEX owned by the current process is not the preemption lock mutex */
		 if(MUTEX_ID != PREEMPTION_LOCK_MUTEX)
		 {
			 *LOCK_LEVEL = CURRENT_PARTITION_STATUS.LOCK_LEVEL;
			 *RETURN_CODE = INVALID_MODE;
			 return;
		 }else
		 {
			 /* TODO: This call should not perform PREEMPTION_LOCK_MUTEX error check */
			 RELEASE_MUTEX(PREEMPTION_LOCK_MUTEX, RETURN_CODE);
			
			 MUTEX_STATUS_TYPE MUTEX_STATUS;
			 /* cf.3.3.2.13
			  * The RETURN_CODE has value of what returned by the RELEASE_MUTEX call 
			  * So TMP_RETURN_CODE is needed*/
			 RETURN_CODE_TYPE TMP_RETURN_CODE;	
			  		  	 
			 /* TODO: This call should not perform PREEMPTION_LOCK_MUTEX error check */
			 GET_MUTEX_STATUS(PREEMPTION_LOCK_MUTEX, &MUTEX_STATUS, &TMP_RETURN_CODE);
			 CURRENT_PARTITION_STATUS.LOCK_LEVEL = MUTEX_STATUS.LOCK_COUNT;
			 *LOCK_LEVEL = CURRENT_PARTITION_STATUS.LOCK_LEVEL;
		 }
	 } 
}


void GET_MY_ID (PROCESS_ID_TYPE *PROCESS_ID, RETURN_CODE_TYPE *RETURN_CODE)
{
	/* if the current process is the error handler */
	if(PROCESS_ID == NULL_PROCESS_ID)
	{
		*RETURN_CODE = INVALID_MODE;
		return;
	}
	
	/* TODO: cf.3.3.2.14
	 * ->	*PROCESS_ID = identify the current process;
	 * -> 	*RETURN_CODE = NO_ERROR;*/	 
}


void INITIALIZE_PROCESS_CORE_AFFINITY(PROCESS_ID_TYPE PROCESS_ID, PROCESSOR_CORE_ID_TYPE PROCESSOR_CORE_ID, RETURN_CODE_TYPE *RETURN_CODE)
{
	/* if current operating mode is NORMAL */
	if(CURRENT_PARTITION_STATUS.OPERATING_MODE == NORMAL)
	{
		*RETURN_CODE = INVALID_MODE;
		return;
	}
	
	/*TODO: cf.3.3.2.15
	 * ->	if PROCESSOR_CORE_ID doesn't identify a processor core assigned to this partition
	 * ->		*RETURN_CODE = INVALID_CONFIG;
	 * ->		return;*/
	 
	for(int i=0;i<PROCESS_NUMBER;i++)
	{
		if(EXISTING_PROCESS[i].id == PROCESS_ID)
		{
			/* TODO: 
			 * ->	Set the process's core affinity to PROCESSOR_CORE_ID 
			 * ->	return;*/
		}
	}
	
	/* if PROCESS_ID corresponds to none of EXISTING_PROCESS */
	*RETURN_CODE = INVALID_PARAM;
}


void GET_MY_PROCESSOR_CORE_ID(PROCESSOR_CORE_ID_TYPE *PROCESSOR_CORE_ID, RETURN_CODE_TYPE *RETURN_CODE)
{
	/*TODO: cf.3.3.2.16
	 * ->	*PROCESSOR_CORE_ID = the ID of the processor core that the current process is executing on
	 * ->	*RETURN_CODE = NO_ERROR;*/
}



/********************* Partition Services **********************/

void SET_PARTITION_MODE (OPERATING_MODE_TYPE OPERATING_MODE, RETURN_CODE_TYPE *RETURN_CODE )
{
		//if the OPERATING MODE is not one of existnig modes
		if(OPERATING_MODE < IDLE || OPERATING_MODE > NORMAL)
		{
			*RETURN_CODE = INVALID_PARAM;
			return;
		}
		//action is ignored if set to NORMAL from NORMAL
		if(OPERATING_MODE == NORMAL && CURRENT_STATE == NORMAL)
		{
			*RETURN_CODE = NO_ACTION;
			return;
		}
		//cannot set to warm_start from cold_start cf.2.3.1.4
		if(OPERATING_MODE == WARM_START && CURRENT_STATE == COLD_START)
		{
			*RETURN_CODE = INVALID_MODE;
			return;
		}
		
		// If everything goes wells
		CURRENT_STATE = OPERATING_MODE;
		
		/* cf. 3.2.2.2 of A653p1-4		
		 * if(OPERATING_MODE == IDLE)
		 * 		shut down the partition;
		 * if(OPERATING_MODE == WARM_START || OPERATING_MODE == COLD_START)
		 * 		inhibit process scheduling and switch back to initialization mode;
		 * if(OPERATING_MODE == NORMAL)
		 * {
		 * 		-> set to READY all previously started (not delayed) aperiodic processes
		 *		   (unless the process was suspended);
		 *		-> set release point of all previously delay started aperiodic processes
		 *		   to the system clock time plus their delay times;
		 *		-> set first release points of all previously started (not delayed) periodic
		 *		   processes to the partition’s next periodic processing start;
		 *		-> set first release points of all previously delay started periodic processes
		 *		   to the partition’s next periodic processing start plus their delay times;
		 *		   -- at their release points, the processes are set to READY (if not DORMANT)
		 *		      calculate the DEADLINE_TIME of all non-dormant processes in the partition;
		 *		   -- a DEADLINE_TIME calculation may cause an overflow of the underlying clock.
		 * 		      If this occurs, HM is invoked with an illegal request error code
         *            set the partition’s lock level to zero;
         * }
         * if(error handler process has been created)
         * 		enable the error handler process for execution and fault processing;
         * activate the process scheduling;
         */
         
         *RETURN_CODE = NO_ERROR; 
}



void GET_PARTITION_STATUS(PARTITION_STATUS_TYPE *PARTITION_STATUS, RETURN_CODE_TYPE *RETURN_CODE)
{
	//always successful
	*PARTITION_STATUS = CURRENT_PARTITION_STATUS;
	*RETURN_CODE = NO_ERROR;
}



/************************ MUTEX services***************************/

/* Initial number of mutex */

void CREATE_MUTEX(MUTEX_NAME_TYPE MUTEX_NAME, PRIORITY_TYPE MUTEX_PRIORITY, QUEUING_DISCIPLINE_TYPE QUEUING_DISCIPLINE, MUTEX_ID_TYPE *MUTEX_ID, RETURN_CODE_TYPE *RETURN_CODE)
{
	/* if max number of mutexes has been created */
	if(MUTEX_NUMBER >= MAX_NUMBER_OF_MUTEXES)
		{
			*RETURN_CODE = INVALID_CONFIG;
			return;
		}
		
	/* if a mutex named MUTEX_NAME has already been created */
	if(MUTEX_NAME_TAKEN(EXISTING_MUTEX, MUTEX_NAME) != -1)
		{
			*RETURN_CODE = NO_ACTION;
			return;
		}
		
	/* if MUTEX_PRIORITY is out of range */
	if(MUTEX_PRIORITY < MIN_PRIORITY_VALUE || MUTEX_PRIORITY > MAX_PRIORITY_VALUE)
		{
			*RETURN_CODE = INVALID_PARAM;
			return;
		}
		
	/* if the QUEUING_DISCIPLINE is invalid */
	if(QUEUING_DISCIPLINE != FIFO && QUEUING_DISCIPLINE != PRIORITY)
		{
			*RETURN_CODE = INVALID_PARAM;
			return;
		}

	/* if the current operating mode is NORMAL */
	if(CURRENT_STATE == NORMAL)
		{
			*RETURN_CODE = INVALID_MODE;
			return;
		}
		
	/* if everything is fine */
	*MUTEX_ID = MUTEX_NUMBER++;
	MUTEX mtx = EXISTING_MUTEX[*MUTEX_ID];
	mtx.id = *MUTEX_ID;
	AVAILABLE_MUTEX[mtx.id] = 1;						//the mutex became availableg
	mtx.name[0] = '\0';									//in order to set the name to empty string
	strncat(mtx.name, MUTEX_NAME, MAX_NAME_LENGTH-1);
	mtx.discipline = QUEUING_DISCIPLINE;
	mtx.status.MUTEX_PRIORITY = MUTEX_PRIORITY;
	mtx.status.LOCK_COUNT = 0;
	mtx.status.MUTEX_STATE = AVAILABLE;	
	*RETURN_CODE = NO_ERROR;
}


void ACQUIRE_MUTEX (MUTEX_ID_TYPE MUTEX_ID, SYSTEM_TIME_TYPE TIME_OUT, RETURN_CODE_TYPE *RETURN_CODE )
{
	/* if the MUTEX_ID corresponds to PREEMTION_LOCK_MUTEX */
	if(MUTEX_ID == PREEMPTION_LOCK_MUTEX)
	{
		*RETURN_CODE = INVALID_PARAM;
		return;
	}
	
	/* if the TIME_OUT is out of range */
	SYSTEM_TIME_TYPE lb = 1;
	if(TIME_OUT < lb<<63 || TIMED_OUT > 0x7FFFFFFFFFFFFFFF)
	{
		*RETURN_CODE = INVALID_PARAM;
		return;
	}
	
	/* get the current process ID in order to get its MUTEX_ID,if any */
	PROCESS_ID_TYPE PROCESS_ID;
	GET_MY_ID(&PROCESS_ID, RETURN_CODE);
	
	
	MUTEX_ID_TYPE CURRENT_MUTEX_ID;
	GET_PROCESS_MUTEX_STATE(PROCESS_ID, &CURRENT_MUTEX_ID, RETURN_CODE);
	
	/* if current process already owns another MUTEX */
	if(CURRENT_MUTEX_ID != NO_MUTEX_OWNED && CURRENT_MUTEX_ID != MUTEX_ID)
	{
		*RETURN_CODE = INVALID_MODE;
		return;
	}
	
	/* if current process is error handler process */
	if(PROCESS_ID == NULL_PROCESS_ID)
	{
		*RETURN_CODE = INVALID_MODE;
		return;
	}
	
	
	PROCESS_STATUS_TYPE PROCESS_STATUS;
	MUTEX_STATUS_TYPE MUTEX_STATUS;
	GET_PROCESS_STATUS(PROCESS_ID, &PROCESS_STATUS, RETURN_CODE);
	GET_MUTEX_STATUS(MUTEX_ID, &MUTEX_STATUS, RETURN_CODE);
	
	/* if current process's priority is greater than the mutex's priority */
	if(PROCESS_STATUS.CURRENT_PRIORITY > MUTEX_STATUS.MUTEX_PRIORITY)
	{
		*RETURN_CODE = INVALID_MODE;
		return;
	}
	
	for(int i=0;i<MUTEX_NUMBER;i++)
	{
		if(EXISTING_MUTEX[i].id == MUTEX_ID)
		{
			MUTEX mtx = EXISTING_MUTEX[i];
		
			/* if the specified mutex is not owned yet */
			if(mtx.status.MUTEX_STATE == AVAILABLE)
			{
				
				mtx.status.MUTEX_STATE = OWNED;
				AVAILABLE_MUTEX[i] = 0;
				mtx.status.MUTEX_OWNER = PROCESS_ID;
				mtx.status.LOCK_COUNT++;
				
					
				/* get the index of the current process so that we have access to retained_priority */
				for(int i=0;i<PROCESS_NUMBER;i++)
				{
					
					if(PROCESS_ID == EXISTING_PROCESS[i].id)
					{
						/* keep the current priority of process */
						EXISTING_PROCESS[i].retained_priority = PROCESS_STATUS.CURRENT_PRIORITY;
					}
				}
				
				PROCESS_STATUS.CURRENT_PRIORITY = MUTEX_STATUS.MUTEX_PRIORITY;
				/*TODO: cf.3.7.2.5.2
				 * -> positioning the process as being in the ready state for 
				 * 	  the longest elapsed time at that priority
				 * -> *RETURN_CODE = NO_ERROR*/
				return;
			}else if(mtx.status.MUTEX_OWNER == PROCESS_ID)
			{
				if(mtx.status.LOCK_COUNT == MAX_LOCK_LEVEL)
				{
					*RETURN_CODE = INVALID_CONFIG;
					return;
				}else
				{
					mtx.status.LOCK_COUNT++;
					*RETURN_CODE = NO_ERROR;
					return;
				}
			}else if(TIME_OUT == 0)
			{
				*RETURN_CODE = NOT_AVAILABLE;
				return;
			}else if(TIME_OUT == INFINITE_TIME_VALUE)
			{
				PROCESS_STATUS.PROCESS_STATE = WAITING;
				/* TODO: cf.3.7.2.5.2
				 * -> insert current process in the mutex's process queue 
				 * 	  at the position specified by the queuing discipline
				 * -> process rescheduling*/
				 *RETURN_CODE = NO_ERROR;
				 return;
			 }else
			 {
				 PROCESS_STATUS.PROCESS_STATE = WAITING;
				/* TODO: cf.3.7.2.5.2
				 * -> insert current process in the mutex's process queue 
				 * 	  at the position specified by the queuing discipline
				 * -> initiate a time counter for the current process with duration TIME_OUT
				 * -> process rescheduling
				 * -> if(expiration of TIME_OUT)
				 * ->		*RETURN_CODE = TIMED_OUT;
				 * -> else
				 * ->		*RETURN_CODE = NO_ERROR;*/
				 return;
			 }
		 }
	 }
	 
	 /* if MUTEX_ID corresponds to none of EXISTING_MUTEX */
	 *RETURN_CODE = INVALID_PARAM;
}


void RELEASE_MUTEX (MUTEX_ID_TYPE MUTEX_ID, RETURN_CODE_TYPE *RETURN_CODE )
{
	/* if the MUTEX_ID corresponds to the partition's PREEMPTION_LOCK_MUTEX */
	if(MUTEX_ID == PREEMPTION_LOCK_MUTEX)
	{
		*RETURN_CODE = INVALID_PARAM;
		return;
	}
	
	
	PROCESS_ID_TYPE PROCESS_ID;
	PROCESS proc;
	GET_MY_ID(&PROCESS_ID, RETURN_CODE);
	for(int i=0;i<PROCESS_NUMBER;i++)
	{
		if(PROCESS_ID == EXISTING_PROCESS[i].id)
			proc = EXISTING_PROCESS[i];
	}
	
	MUTEX_ID_TYPE CURRENT_MUTEX_ID;
	GET_PROCESS_MUTEX_STATE(PROCESS_ID, &CURRENT_MUTEX_ID, RETURN_CODE);
	
	/* if the current process doesn't own the specified mutex */
	if(CURRENT_MUTEX_ID != MUTEX_ID)
	{
		*RETURN_CODE = INVALID_MODE;
		return;
	}
	
	for(int i=0;i<MUTEX_NUMBER;i++)
	{
		if(EXISTING_MUTEX[i].id == MUTEX_ID)
		{
			MUTEX mtx = EXISTING_MUTEX[i];
			mtx.status.LOCK_COUNT--;
			if(mtx.status.LOCK_COUNT == 0)
			{
				mtx.status.MUTEX_STATE = AVAILABLE;
				mtx.status.MUTEX_OWNER = NULL_PROCESS_ID;
				proc.status.CURRENT_PRIORITY = proc.retained_priority;
				/* TODO:cf.3.7.2.5.3.
				 * ->	process rescheduling
				 * ->	if(one or more processes are waiting...
				 * ->		...*/
			 }
		 }
		 *RETURN_CODE = NO_ERROR;
		 return;
	 }
	 
	 /* if MUTEX_ID not corresponding to any EXISTING_MUTEX */
	 *RETURN_CODE = INVALID_PARAM;
}


void RESET_MUTEX(MUTEX_ID_TYPE MUTEX_ID, PROCESS_ID_TYPE PROCESS_ID, RETURN_CODE_TYPE *RETURN_CODE)
{
	/* if MUTEX_ID corresponds to the partition's PREEMPTION_LOCK_MUTEX */
	if(MUTEX_ID == PREEMPTION_LOCK_MUTEX)
	{
		*RETURN_CODE  = INVALID_PARAM;
		return;
	}
	
	PROCESS *proc = NULL;
	for(int i=0;i<PROCESS_NUMBER;i++)
	{
		if(EXISTING_PROCESS[i].id == PROCESS_ID)
			*proc = EXISTING_PROCESS[i];
	}
	
	/* if PROCESS_ID corresponds to none of EXISTING_PROCESS */
	if(proc == NULL)
	{
		*RETURN_CODE = INVALID_PARAM;
		return;
	}
	
	MUTEX_ID_TYPE CURRENT_MUTEX_ID;
	GET_PROCESS_MUTEX_STATE(PROCESS_ID, &CURRENT_MUTEX_ID, RETURN_CODE);
	
	/* if the specified process doesn't own the specified mutex */
	if(CURRENT_MUTEX_ID != MUTEX_ID)
	{
		*RETURN_CODE = INVALID_MODE;
		return;
	}
	
	for(int i=0;i<MUTEX_NUMBER;i++)
	{
		if(EXISTING_MUTEX[i].id == MUTEX_ID)
		{
			EXISTING_MUTEX[i].status.LOCK_COUNT = 0;
			EXISTING_MUTEX[i].status.MUTEX_STATE = AVAILABLE;
			EXISTING_MUTEX[i].status.MUTEX_OWNER = NULL_PROCESS_ID;
			proc->status.CURRENT_PRIORITY = proc->retained_priority;
			/* TODO: cf.3.7.2.5.4
			 * -> process rescheduling
			 * -> if any process is waiting for this MUTEX, remove
			 * 	  the first from queue 
			 * ->		if this process waits with a TIME_OUT counter
			 * ->			stop the counter
			 * ->		set mutex's state to OWNED
			 * ->		increment the mutex's lock count 
			 * -> 		...
			 * ->		...*/
			*RETURN_CODE = NO_ERROR;
			return;
		}
	}
	
	/* if the MUTEX_ID doesn't correspond to any existing MUTEX */
	*RETURN_CODE = INVALID_PARAM;
}


void GET_MUTEX_ID(MUTEX_NAME_TYPE MUTEX_NAME, MUTEX_ID_TYPE *MUTEX_ID, RETURN_CODE_TYPE *RETURN_CODE)
{
	int idx = MUTEX_NAME_TAKEN(EXISTING_MUTEX, MUTEX_NAME);
	
	/* if there is no MUTEX named MUTEX_NAME in the current partition */
	if(idx == -1)
	{
		*RETURN_CODE = INVALID_CONFIG;
		return;
	}
	
	*MUTEX_ID = EXISTING_MUTEX[idx].id;
	*RETURN_CODE = NO_ERROR;
}

void GET_MUTEX_STATUS(MUTEX_ID_TYPE MUTEX_ID, MUTEX_STATUS_TYPE *MUTEX_STATUS, RETURN_CODE_TYPE *RETURN_CODE)
{
	/* if the MUTEX_ID is the partition preemption lock mutex */
	if(MUTEX_ID == PREEMPTION_LOCK_MUTEX)
	{
		*RETURN_CODE = INVALID_PARAM;
		return;
	}
	
	for(int i=0;i<MUTEX_NUMBER;i++)
	{
		if(EXISTING_MUTEX[i].id == MUTEX_ID)
		{
			MUTEX mtx = EXISTING_MUTEX[i];
			MUTEX_STATUS->MUTEX_STATE = mtx.status.MUTEX_STATE;
			MUTEX_STATUS->MUTEX_PRIORITY = mtx.status.MUTEX_PRIORITY;
			MUTEX_STATUS->LOCK_COUNT = mtx.status.LOCK_COUNT;
			MUTEX_STATUS->WAITING_PROCESSES = mtx.status.WAITING_PROCESSES;
			
			if(mtx.status.MUTEX_STATE == AVAILABLE)
				MUTEX_STATUS->MUTEX_OWNER = NULL_PROCESS_ID;
			else if(mtx.status.MUTEX_OWNER == MAIN_PROCESS_ID)
				MUTEX_STATUS->MUTEX_OWNER = MAIN_PROCESS_ID;
			else
				MUTEX_STATUS->MUTEX_OWNER = mtx.status.MUTEX_OWNER;
			*RETURN_CODE = NO_ERROR;
			return;
		}
	}
	
	/* if MUTEX_ID doesn't correspond to any existing mutex */
	*RETURN_CODE = INVALID_PARAM;
}

void GET_PROCESS_MUTEX_STATE(PROCESS_ID_TYPE PROCESS_ID, MUTEX_ID_TYPE *MUTEX_ID, RETURN_CODE_TYPE *RETURN_CODE)
{
	PROCESS *proc = NULL;
	for(int i=0;i<PROCESS_NUMBER;i++)
	{
		if(PROCESS_ID == EXISTING_PROCESS[i].id)
			*proc = EXISTING_PROCESS[i];
	}
	
	/* if PROCESS_ID doesn't correspond to any existing process */
	if(proc == NULL)
	{
		*RETURN_CODE = INVALID_PARAM;
		return;
	}
	
	/* TODO: cf.3.7.2.5.7
		 * ->	if PROCESS_ID identifies a process that currently owning the preemption lock mutex
		 * ->			*MUTEX_ID = PREEMPTION_LOCK_MUTEX;
		 * ->			*RETURN_CODE = NO_ERROR;
		 * ->			return;*/
		 
	for(int i=0;i<MUTEX_NUMBER;i++)
	{
		/* if the specified process is owner of any mutex */
		if(EXISTING_MUTEX[i].status.MUTEX_OWNER == PROCESS_ID)
			{
				*MUTEX_ID = EXISTING_MUTEX[i].id;
				*RETURN_CODE = NO_ERROR;
				return;
			}
	}
	
	/* the specified process is neither owner of PREEMPTION_LOCK_MUTEX nor owner of any existing mutex */
	*MUTEX_ID = NO_MUTEX_OWNED;
	*RETURN_CODE = NO_ERROR;
}
