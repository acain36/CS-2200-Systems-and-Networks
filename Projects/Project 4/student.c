// Created by: Ashley Cain

/*
 * student.c
 * Multithreaded OS Simulation for CS 2200
 *
 * This file contains the CPU scheduler for the simulation.
 */

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "student.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/** Function prototypes **/
extern void idle(unsigned int cpu_id);
extern void preempt(unsigned int cpu_id);
extern void yield(unsigned int cpu_id);
extern void terminate(unsigned int cpu_id);
extern void wake_up(pcb_t *process);



/**
 * current is an array of pointers to the currently running processes, 
 * each pointer corresponding to each CPU in the simulation.
 */
static pcb_t **current;
/* rq is a pointer to a struct you should use for your ready queue implementation.*/
static queue_t *rq;

/**
 * current and rq are accessed by multiple threads, so you will need to use 
 * a mutex to protect it (ready queue).
 *
 * The condition variable queue_not_empty has been provided for you
 * to use in conditional waits and signals.
 */
static pthread_mutex_t current_mutex;
static pthread_mutex_t queue_mutex;
static pthread_cond_t queue_not_empty;

/* keeps track of the scheduling alorightem and cpu count */
static sched_algorithm_t scheduler_algorithm;
static unsigned int cpu_count;
static int timeslice = 0;
/** ------------------------Problem 0 & 2-----------------------------------
 * Checkout PDF Section 2 and 4 for this problem
 * 
 * enqueue() is a helper function to add a process to the ready queue.
 *
 * @param queue pointer to the ready queue
 * @param process process that we need to put in the ready queue
 */
void enqueue(queue_t *queue, pcb_t *process)
{
	/*
	* 1. if there is no head or tail, there are no items in list
	*	-> set head and tail to the process
	* 2. otherwise:
	*	2.1 if the algorithm is preemptive
	*	-> check if process priority is higher than head priority
	*		->Yes: iterate through queue while each entry has a lower pririty than current
	*			-> insert process in queue where it belongs
	*		->No: the process becomes new head	
	*	2.2 if algorithm is round robin or FCFS
	*	-> add item to end of list
	* 3. set mutex unlock
	* 4. set queue not empty
	*/
	
	pthread_mutex_lock(&queue_mutex);
	
	if(!(*queue).tail) {
		queue->tail = process;
		queue->head = process;
	} else {
		if(scheduler_algorithm != PR) {
			queue->tail->next = process;
			queue->tail = process;
			queue->tail->next = NULL;
		} else {
			if(process->priority < queue->head->priority){
				process->next = queue->head;
				queue->head = process;
			} else {
				pcb_t *curr = queue->head;
				while (curr->next && process->priority > curr->next->priority) {
					curr = curr->next;
				}
				process->next = curr->next;
				curr->next = process;
			}
		}
	}
	pthread_cond_signal(&queue_not_empty);
	pthread_mutex_unlock(&queue_mutex);
}

/**
 * dequeue() is a helper function to remove a process to the ready queue.
 *
 * @param queue pointer to the ready queue
 */
pcb_t *dequeue(queue_t *queue)
{

	/*
	* 1. set mutex Lock
	* 2. if the queue is empty
	*	-> return 0
	* 3. -> store ref to head in variable
	*	-> set head of queue to head.next
	*	-> ref variable.next set to 0
	* 4. if there is only one item
	*	-> set head and tail to 0
	*	-> set queue empty
	* 5. more than one item
	*	-> nothing else needed
	* 6. set mutex unlock
	* 7. return ref variable to removed
	*/
	
	pcb_t *temp;
	pthread_mutex_lock(&queue_mutex);

	if(is_empty(queue) && !pthread_mutex_unlock(&queue_mutex)) {
		temp = NULL;
		return temp;
	} else {
		temp = queue->head;
		queue->head = queue->head->next;
		if(queue->head == NULL) {
			queue->tail = NULL;
		} 
		temp->next = NULL;
	}

	pthread_mutex_unlock(&queue_mutex);

	return temp;

}

/** ------------------------Problem 0-----------------------------------
 * Checkout PDF Section 2 for this problem
 * 
 * is_empty() is a helper function that returns whether the ready queue
 * has any processes in it.
 * 
 * @param queue pointer to the ready queue
 * 
 * @return a boolean value that indicates whether the queue is empty or not
 */
bool is_empty(queue_t *queue)
{
	/* 
	* 1. return whether the queue has a valid head node 
	*/
	
	return queue->head == NULL;
}

/** ------------------------Problem 1B & 3-----------------------------------
 * Checkout PDF Section 3 and 5 for this problem
 * 
 * schedule() is your CPU scheduler.
 * 
 * Remember to specify the timeslice if the scheduling algorithm is Round-Robin
 * 
 * @param cpu_id the target cpu we decide to put our process in
 */
static void schedule(unsigned int cpu_id)
{
	/*
	* 1. extract first process from ready queue
	*		-> use dequeue on ready queue
	*		-> set state of this extracted process
	*		-> lock mutex
	* 2. determine whether the process is runnable (is there a runnable process?)
	*		2.1 Yes: PCB in context_switch will be process extraxted
	*		2.2 No: PCB in context_switch will be NULL Pointer
	* 3. call context_switch to select process to execute
	* 		-> use the timeslice (from main())
	*/
    
	if(is_empty(rq)){
		pthread_mutex_lock(&current_mutex);

		current[cpu_id] = NULL;

		pthread_mutex_unlock(&current_mutex);
		context_switch(cpu_id, NULL, timeslice);
	} else {
		pcb_t *process = dequeue(rq);


		if (process) {
			process->state = PROCESS_RUNNING;
		}

		pthread_mutex_lock(&current_mutex);

		current[cpu_id] = process;

		pthread_mutex_unlock(&current_mutex);
		context_switch(cpu_id, process, timeslice);
	}
}

/**  ------------------------Problem 1A-----------------------------------
 * Checkout PDF Section 3 for this problem
 * 
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled.
 *
 * @param cpu_id the cpu that is waiting for process to come in
 */
extern void idle(unsigned int cpu_id)
{
	/*
	* 1. lock
	* 2. wait for a process to be scheduled
	*		-> start of waiting is while ready queue is empty of processes
	*		-> same as waiting for there to be something on the queue
	* 3. unlock mutex
	* 4. schedule replacement process for idle CPU
	*/
    	pthread_mutex_lock(&queue_mutex);
    
	while (!(rq->head)) {
		pthread_cond_wait(&queue_not_empty, &queue_mutex);
	}
    
	pthread_mutex_unlock(&queue_mutex);

	schedule(cpu_id);
}

/** ------------------------Problem 2 & 3-----------------------------------
 * Checkout Section 4 and 5 for this problem
 * 
 * preempt() is the handler used in Round-robin and Preemptive Priority 
 * Scheduling
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 * 
 * @param cpu_id the cpu in which we want to preempt process
 */
extern void preempt(unsigned int cpu_id)
{
	/*
	* 1. lock
	* 2. set process at current[cpu_id] to ready
	*		-> place process in ready queue
	* 3. select a new process
	*		-> unlock mutex
	*		-> call schedule with the CPU
	*/
	pthread_mutex_lock(&current_mutex);
	pcb_t *curr = current[cpu_id];

	if (curr) {
		curr->state = PROCESS_READY;
		current[cpu_id] = NULL;
		
	}
	enqueue(rq, curr);
	pthread_mutex_unlock(&current_mutex);
	schedule(cpu_id);
}

/**  ------------------------Problem 1-----------------------------------
 * Checkout PDF Section 3 for this problem
 * 
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * @param cpu_id the cpu that is yielded by the process
 */
extern void yield(unsigned int cpu_id)
{
	/*
	* 1. lock mutex
	* 2. store the currently executing process and set the state to waiting
	* 3. unlock mutex before using schedule() to send cpu to be scheduled for another process
	*/
	pthread_mutex_lock(&current_mutex);

	pcb_t *curr =  current[cpu_id];
	
	if (curr) {
		curr->state = PROCESS_WAITING;
		current[cpu_id] = NULL;
	}

	pthread_mutex_unlock(&current_mutex);
	schedule(cpu_id);
}

/**  ------------------------Problem 1-----------------------------------
 * Checkout PDF Section 3
 * 
 * terminate() is the handler called by the simulator when a process completes.
 * 
 * @param cpu_id the cpu we want to terminate
 */
extern void terminate(unsigned int cpu_id)
{
	/*
	* 1. lock
	* 2. save current process and change state to terminated
	* 3. unlock mutex
	* 4. use schedule() to sent cpu to be scheduled
	*/
	pthread_mutex_lock(&current_mutex);

	pcb_t *curr = current[cpu_id];

	if (curr) {
		curr->state = PROCESS_TERMINATED;
	}

	current[cpu_id] = NULL;
	pthread_mutex_unlock(&current_mutex);

	schedule(cpu_id); 
}

/**  ------------------------Problem 1A & 3---------------------------------
 * Checkout PDF Section 3 and 4 for this problem
 * 
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes. This method will also need to handle priority, 
 * Look in section 5 of the PDF for more info.
 * 
 * @param process the process that finishes I/O and is ready to run on CPU
 */
extern void wake_up(pcb_t *process)
{
	/* 
	* 1. set the state of the process to signal that it is ready to execute
	*		-> enqueue the process onto the ready queue
	* 2. handle priority: we want to find if the process has higher priority (lower number) than anything executing
	*		2.1 do this if the algorithm is preemptive
	*			-> lock mutex
	*			-> iterate through every currently running process on a CPU (current array)
	*			-> determine the highest number priority of all currently runniing for comparison
	*			-> unlock
	*			-> compare with current process's priority
	*			-> if process has higher priority (lower number) than one currently executing, then force_preempt
	*/
	process->state = PROCESS_READY;

	enqueue(rq, process);

	if (scheduler_algorithm == PR) {


		unsigned int cpu = 0;
		unsigned int lowestPriority = 0;
		pthread_mutex_lock(&current_mutex);

		if (current[cpu]) {
			lowestPriority = current[cpu]->priority;
		}

		for(unsigned int i = 0; i < cpu_count; i++) {
			if (current[i] == NULL) {
				cpu = i;
				break;
			}
			
			if (current[i]->priority > lowestPriority) {
				cpu = i;
				lowestPriority = current[i]->priority;
			}
			
		}

		pthread_mutex_unlock(&current_mutex);

		if (process->priority < lowestPriority && current[cpu] != NULL) {
			force_preempt(cpu);
		}
	}
}

/**
 * main() simply parses command line arguments, then calls start_simulator().
 * Add support for -r and -p parameters. If no argument has been supplied, 
 * you should default to FCFS.
 * 
 * HINT:
 * Use the scheduler_algorithm variable (see student.h) in your scheduler to 
 * keep track of the scheduling algorithm you're using.
 */
int main(int argc, char *argv[])
{
	if (argc == 2) {
		scheduler_algorithm = FCFS;
		timeslice = -1;
	} else if ('r' == argv[2][1] && argc == 4) {
		scheduler_algorithm = RR;
		timeslice = atoi(argv[3]);
	} else if ('p'== argv[2][1] && argc == 3) {
		scheduler_algorithm = PR;
		timeslice = -1;
	} else {
		fprintf(stderr, "CS 2200 Project 4 -- Multithreaded OS Simulator\n"
				"Usage: ./os-sim <# CPUs> [ -r <time slice> | -p ]\n"
				"    Default : FCFS Scheduler\n"
				"         -r : Round-Robin Scheduler\n1\n"
				"         -p : Priority Scheduler\n");
		return -1;
	}



	/* Parse the command line arguments */
	cpu_count = strtoul(argv[1], NULL, 0);

	/* Allocate the current[] array and its mutex */
	current = malloc(sizeof(pcb_t *) * cpu_count);
	assert(current != NULL);
	pthread_mutex_init(&current_mutex, NULL);
	pthread_mutex_init(&queue_mutex, NULL);
	pthread_cond_init(&queue_not_empty, NULL);
	rq = (queue_t *)malloc(sizeof(queue_t));
	assert(rq != NULL);

	/* Start the simulator in the library */
	start_simulator(cpu_count);

	return 0;
}

#pragma GCC diagnostic pop
