/* All new structures and new types are directly
 * copied from Arinc653 part 1 
 * 
 * 
 * Chenle YU*/ 
#ifndef USE_ARINC653
#define USE_ARINC653 	
#define BY_PTHREAD 						//indicates if Arinc functions are made of pthread or not

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>


#ifndef APEX_TYPES
#define APEX_TYPES
/*---------------------------*/
/* Domain limits 			 */
/*---------------------------*/
/* 					Implementation Dependent			 */
/* These values define the domain limits and are implementation dependent. */
#define SYSTEM_LIMIT_NUMBER_OF_PARTITIONS 32				            /* module scope */
#define SYSTEM_LIMIT_NUMBER_OF_MESSAGES 512 							/* module scope */
#define SYSTEM_LIMIT_MESSAGE_SIZE 8192 									/* module scope */
#define SYSTEM_LIMIT_NUMBER_OF_PROCESSES 128 							/* partition scope */
#define SYSTEM_LIMIT_NUMBER_OF_SAMPLING_PORTS 512 						/* partition scope */
#define SYSTEM_LIMIT_NUMBER_OF_QUEUING_PORTS 512 						/* partition scope */
#define SYSTEM_LIMIT_NUMBER_OF_BUFFERS 256 								/* partition scope */
#define SYSTEM_LIMIT_NUMBER_OF_BLACKBOARDS 256 							/* partition scope */
#define SYSTEM_LIMIT_NUMBER_OF_SEMAPHORES 256 							/* partition scope */
#define SYSTEM_LIMIT_NUMBER_OF_EVENTS 256 								/* partition scope */
#define SYSTEM_LIMIT_NUMBER_OF_MUTEXES 256 								/* partition scope */

/*----------------------*/
/* Base APEX types      */
/*----------------------*/
typedef unsigned char APEX_BYTE; 										/* 8-bit unsigned */
typedef long APEX_INTEGER; 												/* 32-bit signed */
typedef unsigned long APEX_UNSIGNED; 									/* 32-bit unsigned */
typedef long long APEX_LONG_INTEGER;									/* 64-bit signed */

/*----------------------*/
/* General APEX types */
/*----------------------*/
#define MAX_NAME_LENGTH 32	
typedef void (* SYSTEM_ADDRESS_TYPE);
typedef APEX_BYTE * MESSAGE_ADDR_TYPE;
typedef APEX_INTEGER MESSAGE_SIZE_TYPE;
typedef APEX_INTEGER MESSAGE_RANGE_TYPE;
typedef enum { SOURCE = 0, DESTINATION = 1 } PORT_DIRECTION_TYPE;
typedef enum { FIFO = 0, PRIORITY = 1 } QUEUING_DISCIPLINE_TYPE;
typedef char NAME_TYPE[MAX_NAME_LENGTH];	
typedef APEX_LONG_INTEGER SYSTEM_TIME_TYPE;								/* 64-bit signed integer with a 1 nanosecond LSB */

#define INFINITE_TIME_VALUE -1

typedef APEX_INTEGER PROCESSOR_CORE_ID_TYPE;

#define CORE_AFFINITY_NO_PREFERENCE -1

typedef enum 
{
		NO_ERROR = 0,
		NO_ACTION = 1,
		NOT_AVAILABLE = 2,
		INVALID_PARAM = 3,
		INVALID_CONFIG = 4,
		INVALID_MODE = 5,
		TIMED_OUT = 6
}RETURN_CODE_TYPE;

#endif  //#ifndef APEX_TYPES

/*----------------------------------------------------------------*/
/* 																  */
/* PROCESS constant and type definitions and management services  */
/* 															      */
/*----------------------------------------------------------------*/

#ifndef APEX_PROCESS
#define APEX_PROCESS

#define MAX_NUMBER_OF_PROCESSES SYSTEM_LIMIT_NUMBER_OF_PROCESSES
#define MIN_PRIORITY_VALUE 1
#define MAX_PRIORITY_VALUE 239											//from chenle: MUTEX has priority between 240 and MAX(APEX_INTEGER)
#define MAX_LOCK_LEVEL 16

typedef NAME_TYPE PROCESS_NAME_TYPE;
typedef APEX_INTEGER PROCESS_ID_TYPE;									//PROCESS_ID_TYPE is not used, cannot identify process with this attribute, declare new struct #TODO
#define NULL_PROCESS_ID 0												//error handler process's ID
#define MAIN_PROCESS_ID -1

typedef APEX_INTEGER LOCK_LEVEL_TYPE;
typedef APEX_UNSIGNED STACK_SIZE_TYPE;
typedef APEX_INTEGER WAITING_RANGE_TYPE;
typedef APEX_INTEGER PRIORITY_TYPE;		


typedef enum
{
	DORMANT  = 0,
	READY	 = 1,
	RUNNING  = 2,
	WAITING  = 3,
	FAULTED  = 4
}PROCESS_STATE_TYPE;

typedef enum { SOFT = 0, HARD = 1 } DEADLINE_TYPE;

typedef	struct 
{
	SYSTEM_TIME_TYPE PERIOD;
	SYSTEM_TIME_TYPE TIME_CAPACITY;
	SYSTEM_ADDRESS_TYPE ENTRY_POINT;
	STACK_SIZE_TYPE STACK_SIZE;
	PRIORITY_TYPE BASE_PRIORITY;
	DEADLINE_TYPE DEADLINE;
	PROCESS_NAME_TYPE NAME;
	
} PROCESS_ATTRIBUTE_TYPE;

typedef	struct 
{
	SYSTEM_TIME_TYPE DEADLINE_TIME;
	PRIORITY_TYPE CURRENT_PRIORITY;
	PROCESS_STATE_TYPE PROCESS_STATE;
	PROCESS_ATTRIBUTE_TYPE ATTRIBUTES;
} PROCESS_STATUS_TYPE;

extern void CREATE_PROCESS (
/*in */ PROCESS_ATTRIBUTE_TYPE *ATTRIBUTES,
/*out*/ PROCESS_ID_TYPE *PROCESS_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void SET_PRIORITY (
/*in */ PROCESS_ID_TYPE PROCESS_ID,
/*in */ PRIORITY_TYPE PRIORITY,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void SUSPEND_SELF (
/*in */ SYSTEM_TIME_TYPE TIME_OUT,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void SUSPEND (
/*in */ PROCESS_ID_TYPE PROCESS_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void RESUME (
/*in */ PROCESS_ID_TYPE PROCESS_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void STOP_SELF (void);

extern void STOP (
/*in */ PROCESS_ID_TYPE PROCESS_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void START (
/*in */ PROCESS_ID_TYPE PROCESS_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void DELAYED_START (
/*in */ PROCESS_ID_TYPE PROCESS_ID,
/*in */ SYSTEM_TIME_TYPE DELAY_TIME,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void LOCK_PREEMPTION (
/*out*/ LOCK_LEVEL_TYPE *LOCK_LEVEL,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void UNLOCK_PREEMPTION (
/*out*/ LOCK_LEVEL_TYPE *LOCK_LEVEL,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void GET_MY_ID (
/*out*/ PROCESS_ID_TYPE *PROCESS_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void GET_PROCESS_ID (
/*in */ PROCESS_NAME_TYPE PROCESS_NAME,
/*out*/ PROCESS_ID_TYPE *PROCESS_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void GET_PROCESS_STATUS (
/*in */ PROCESS_ID_TYPE PROCESS_ID,
/*out*/ PROCESS_STATUS_TYPE *PROCESS_STATUS,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void INITIALIZE_PROCESS_CORE_AFFINITY (
/*in */ PROCESS_ID_TYPE PROCESS_ID,
/*in */ PROCESSOR_CORE_ID_TYPE PROCESSOR_CORE_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void GET_MY_PROCESSOR_CORE_ID (
/*out*/ PROCESSOR_CORE_ID_TYPE *PROCESSOR_CORE_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

#endif	//#ifndef APEX_PROCESS

/*----------------------------------------------------------------*/
/* 																  */
/* PARTITION constant and type definitions and management services*/
/* 																  */
/*----------------------------------------------------------------*/

#ifndef APEX_PARTITION
#define APEX_PARTITION

#define MAX_NUMBER_OF_PARTITIONS SYSTEM_LIMIT_NUMBER_OF_PARTITIONS

typedef APEX_INTEGER PARTITION_ID_TYPE;
typedef APEX_UNSIGNED NUM_CORES_TYPE;

typedef enum {
	IDLE = 0,
	COLD_START = 1,
	WARM_START = 2,
	NORMAL = 3
} OPERATING_MODE_TYPE;

typedef APEX_INTEGER PARTITION_ID_TYPE;
typedef APEX_UNSIGNED NUM_CORES_TYPE;

typedef enum {
	NORMAL_START = 0,
	PARTITION_RESTART = 1,
	HM_MODULE_RESTART = 2,
	HM_PARTITION_RESTART = 3
} START_CONDITION_TYPE;

typedef struct {
	SYSTEM_TIME_TYPE PERIOD;
	SYSTEM_TIME_TYPE DURATION;
	PARTITION_ID_TYPE IDENTIFIER;
	LOCK_LEVEL_TYPE LOCK_LEVEL;
	OPERATING_MODE_TYPE OPERATING_MODE;
	START_CONDITION_TYPE START_CONDITION;
	NUM_CORES_TYPE NUM_ASSIGNED_CORES;
} PARTITION_STATUS_TYPE;

extern void GET_PARTITION_STATUS (
/*out*/ PARTITION_STATUS_TYPE *PARTITION_STATUS,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void SET_PARTITION_MODE (
/*in */ OPERATING_MODE_TYPE OPERATING_MODE,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

#endif	//#ifndef APEX_PARTITION

/*----------------------------------------------------------------*/
/* 																  */
/* MUTEX constant and type definitions and management services    */
/* 															      */
/*----------------------------------------------------------------*/
#ifndef APEX_MUTEX
#define APEX_MUTEX

#define MAX_NUMBER_OF_MUTEXES SYSTEM_LIMIT_NUMBER_OF_MUTEXES

typedef NAME_TYPE MUTEX_NAME_TYPE;
typedef APEX_INTEGER MUTEX_ID_TYPE;

#define NO_MUTEX_OWNED 		  -2
#define PREEMPTION_LOCK_MUTEX -3

typedef APEX_INTEGER LOCK_COUNT_TYPE;
typedef enum { AVAILABLE = 0, OWNED = 1 } MUTEX_STATE_TYPE;

typedef struct {
	MUTEX_STATE_TYPE MUTEX_STATE;
	PROCESS_ID_TYPE MUTEX_OWNER;
	PRIORITY_TYPE MUTEX_PRIORITY;										//from chenle: MUTEX has priority between 240 and max(APEX_INTEGER)
	LOCK_COUNT_TYPE LOCK_COUNT;
	WAITING_RANGE_TYPE WAITING_PROCESSES;
} MUTEX_STATUS_TYPE;



extern void CREATE_MUTEX (
/*in */ MUTEX_NAME_TYPE MUTEX_NAME,
/*in */ PRIORITY_TYPE MUTEX_PRIORITY,
/*in */ QUEUING_DISCIPLINE_TYPE QUEUING_DISCIPLINE,
/*out*/ MUTEX_ID_TYPE *MUTEX_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void ACQUIRE_MUTEX (
/*in */ MUTEX_ID_TYPE MUTEX_ID,
/*in */ SYSTEM_TIME_TYPE TIME_OUT,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void RELEASE_MUTEX (
/*in */ MUTEX_ID_TYPE MUTEX_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void RESET_MUTEX (
/*in */ MUTEX_ID_TYPE MUTEX_ID,
/*in */ PROCESS_ID_TYPE PROCESS_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void GET_MUTEX_ID (
/*in */ MUTEX_NAME_TYPE MUTEX_NAME,
/*out*/ MUTEX_ID_TYPE *MUTEX_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void GET_MUTEX_STATUS (
/*in */ MUTEX_ID_TYPE MUTEX_ID,
/*out*/ MUTEX_STATUS_TYPE *MUTEX_STATUS,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

extern void GET_PROCESS_MUTEX_STATE (
/*in */ PROCESS_ID_TYPE PROCESS_ID,
/*out*/ MUTEX_ID_TYPE *MUTEX_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE );

#endif 			//define MUTEX




/*****************From Chenle*****************/
//A process, partition... is aperiodic is its PERIOD value is equal to APERIODIC
#define APERIODIC 0

typedef struct{
	MUTEX_ID_TYPE id;
	MUTEX_NAME_TYPE name;
	MUTEX_STATUS_TYPE status;
	QUEUING_DISCIPLINE_TYPE discipline;
}MUTEX;

typedef struct{
	PROCESS_STATUS_TYPE status;
	PRIORITY_TYPE retained_priority;
	PROCESS_ID_TYPE id;
}PROCESS;

int Arinc_pthread_create (pthread_t *pt, pthread_attr_t *attr, void *fn, void *data);

void Arinc_pthread_exit(void);

_Bool Arinc_compare_and_swap(unsigned long *, unsigned long, unsigned long);

int __attribute__((unused))
Get_Free_Mutex();
/*****************From Chenle*****************/

#endif			//define USE_ARINC653
