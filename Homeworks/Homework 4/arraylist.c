/**
 * Name: Ashley Cain
 * GTID: 903576477
 * V5.0
 */

/*  PART 2: A CS-2200 C implementation of the arraylist data structure.
    Implement an array list.
    The methods that are required are all described in the header file. Description for the methods can be found there.

    Hint 1: Review documentation/ man page for malloc, calloc, and realloc.
    Hint 2: Review how an arraylist works.
    Hint 3: You can use GDB if your implentation causes segmentation faults.a
*/
#include "arraylist.h"

/* Student code goes below this point */

/**
 * Create an arraylist data structure with a backing  array of type char **
 * (an array of char *). Both the backing array and the struct arraylist
 * must be freed after use.
 * Backing array must be located in the heap!
 *(1) allocate memory for the arrayList and the backing array
		(1a) the arrayList will be a pointer to the to the struct with a size of the sizeOf arrayList_t
 		(1b) the backing array will be created after the struct and will allocate a size = capacity * sizeOf(char *)
 *		(1*) If the arrayList or backing_Array are null, free the space on the heap using free() or destroy()
 *	(2) set capacity and size
 *	(3) return the pointer to the list
 * @param capacity the intial length of the backing array
 * @return pointer to the newly created struct arraylist
 */
arraylist_t *create_arraylist(uint capacity){

	arraylist_t *myList = malloc(sizeof(arraylist_t));	
	
	if (myList == NULL){
		return NULL;								
	}
	
	myList -> size = 0;
	myList -> capacity = capacity;
	
	char **newBacking = calloc(capacity, sizeof(char *));
	
	if (newBacking == NULL){
		//free(myList);
		return NULL;								
	}
	
	myList -> backing_array = newBacking;
	return myList;							
}


//(1) check if all parameter information is valid
		//(1a) Check thaat arrayList is not NULL
		//(1b) check that data is not NULL
		//(1c) Check that index is not less than 0
		//(1d) check that index is not greater than size (size, NOT capacity, since we are maintaining contigious nature)
	//(2) check the size of the backing array and resize with resize() if necesary
	//(3) add character at the new index
		//(3a) move all of the data at indexes >= index to the right by 1 in order to create room
		//(3b) add at desired index
		//(3c) increment size
/**
 * Add a char * at the specified index of the arraylist.
 * Backing array must be resized as indexing outside of the array will cause a segmentation fault.
 *
 * @param arraylist the arraylist to be modified
 * @param data a pointer to the data that will be added
 * @param index the location that data will be placed in the arraylist
 */
void add_at_index(arraylist_t *arraylist, char *data, int index){
	
	if (data == NULL){
		return;						
	} else if (index < 0){
		return;						
	}
	
	if (arraylist -> capacity == arraylist -> size){
		resize(arraylist);
							
	}
	
	for (int i = arraylist -> size; i > index; i--) {
		arraylist -> backing_array[i] = arraylist -> backing_array[i - 1];	
	}
	
	arraylist -> backing_array[index] = data;		
	
	arraylist -> size = (arraylist -> size) + 1;		
}

/**
 * Append a char pointer to the end of the arraylist.
 * Backing array must be resized as indexing outside of the array will cause a segmentation fault
 *
 * @param arraylist the arraylist to be modified
 * @param data a pointer to the data that will be added
 */
void append(arraylist_t *arraylist, char *data){
	 
	if (data == NULL){
		return;						
	} 
	
	if (arraylist -> capacity == arraylist -> size){
		resize(arraylist);						
	}
	
	arraylist -> backing_array[arraylist -> size] = data;
	
	arraylist -> size = (arraylist -> size) +1;
}



	//(1) return 0 if 
		//(1a) the arrayList is Null 
		//(1b) if the index < 0 
		//(1c) if the index >= size 
		//(1d) if size < 1
	//(2) create a holder for the removed item at the index in the arrayList
	//(3) store backingArray[index] in the holder
		//(3a) clear that index in the backingArray
	//(4) move all items at indices >= index to the left by 1 in order to have them remain contigious
	//(5) set the element at the index = size - 1 to be NULL since it is removed
	//(6) decrement size
	//(7) return holder
/**
 * Remove a char * from arraylist at specified index.
 * @param arraylist the arraylist to be modified
 * @param index the location that data will be removed from in the arraylist
 * @return the char * that was removed
 */
char *remove_from_index(arraylist_t *arraylist, int index){
    	
	if (index < 0) {
		return NULL;				
	} else if (index > (arraylist -> size) - 1){
		return NULL;				
	}
	
	char *removed = arraylist -> backing_array[index];	
	
	//when the index to remove is the last item
	if (index == ((arraylist -> size) - 1)){
		arraylist -> backing_array[index] = NULL;
		arraylist -> size--;
		return removed;
	}

	//when the index to remove is NOT the last index
	for (int i = index; i < (arraylist -> size); i++) {
		arraylist -> backing_array[i] = arraylist -> backing_array[i+1];
	}

	arraylist -> size--;
	arraylist -> backing_array[(arraylist -> size) - 1] = NULL;

	return removed;
	
	/*if (!(arraylist -> size < 1)){
		for (int i = index; i < ((arraylist -> size) - 1); i++) {
			arraylist -> backing_array[i] = arraylist -> backing_array[i+1];
		}
	}
	
	if (arraylist -> size == 0){
		return removed;
	} else {
		arraylist -> backing_array[(arraylist -> size) - 1] = NULL;
		arraylist -> size = (arraylist -> size) - 1;
	}
	
	return removed;				*/	//7

}


//(1) If the arrayList is NUll, don't do anything
	//(2) Calculate the new capacity of the arrayList to be 2 * sizeOf(char*) * current Capacity
	//(3) Use realloc() method to reallocate the backingArray to two times the amount of space in memory
	//(4) Check if realloc was successful by checking is backingArray is equal to NULL and if so, return
	//(5) Update the capacity of the Arraylist to the new capacity calculated
	//(6) set the backingArray of the arraylist to the newly created bigger arraylist

/**
 * OPTIONAL: This method does not need to be implemented. This is a useful helper method that could be handy
 * if you need to resize your arraylist internally. However, this method is not used ouside of the arraylist.c file.
 * Resize the backing array to hold arraylist->capacity * 2 elements.
 * @param arraylist the arraylist to be resized
 */
void resize(arraylist_t *arraylist){

	arraylist -> capacity = 2 * (arraylist -> capacity);

	arraylist -> backing_array = (char **) realloc(arraylist -> backing_array, (arraylist -> capacity) * sizeof(char*));

	
	if (arraylist -> backing_array == NULL){
		//free(arraylist -> backing_array);	
		return;								//4
	}
	
	return;
}


//(1) if the arrayList is NULL, return and do nothing
	//(2) free the backing array associated with the arrayList in memory
		//(2*) Need to do this first in order to avoid losing the pointer to the backingArray from the arrayList
	
/**
 * Destroys the arraylist by freeing the backing array.
 * @param arraylist the arraylist to be destroyed
 */
void destroy(arraylist_t *arraylist){

	free(arraylist -> backing_array);	//2
	
	
}

