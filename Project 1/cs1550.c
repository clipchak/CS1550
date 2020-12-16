#include <linux/cs1550.h>


/* global sem identifier which gets incremented
with the creation of each semaphore */
long semIdentifier = -1;

struct sem_list *semList = NULL;

/* This syscall creates a new semaphore and stores the provided key to protect access to the semaphore.
The integer value is used to initialize the semaphore's value.
The function returns the identifier of the created semaphore, which can be used to down and up the semaphore. */
asmlinkage long sys_cs1550_create(int value, char name[32], char key[32]){
  //defensive programming for sem creation
  if(value < 0){
    printk(KERN_WARNING "WARNING: semaphore value should not be initialized as less than 0, returning from function...\n");
    return -1;
  }
  if (name[0] == '\0' || key[0] == '\0'){
    printk(KERN_WARNING "WARNING: name or key should not be set as empty, returning from function...\n");
    return -1;
  }

  //create space for a new semaphore
  struct cs1550_sem * newSem  = (struct cs1550_sem *)kmalloc(sizeof(struct cs1550_sem), GFP_ATOMIC);
  if(!newSem){
    /* handle error ... */
    printk(KERN_WARNING "WARNING: COULD NOT ALLOCATE MEMORY, returning from function...\n");
    return -1;
  }

  //init values of the semaphore
  newSem->value = value;
  strcpy(newSem->name, name);
  strcpy(newSem->key, key);
  newSem->head = NULL;
  newSem->tail = NULL;

  //initialize the semaphores lock
  spin_lock_init(&newSem->lock);

  spin_lock(&newSem->lock);
  //increase the global identifier value (critical section)
  semIdentifier = semIdentifier + 1;
  spin_unlock(&newSem->lock);

  newSem->sem_id = semIdentifier;

  //adding the semaphore to the semaphore list
  if (semList == NULL){
    //if list is empty, add the first entry
    semList = (struct sem_list *)kmalloc(sizeof(struct sem_list), GFP_ATOMIC);
    if(!semList){
      /* handle error ... */
      printk(KERN_WARNING "WARNING: COULD NOT ALLOCATE MEMORY, returning from function...\n");
      return -1;
    }
    semList->sem = newSem;
    semList->next = NULL;

  } else{

    //if list is not empty, our new entry to be added at the end of the sem list
    struct sem_list *newEntry = (struct sem_list *)kmalloc(sizeof(struct sem_list), GFP_ATOMIC);
    if(!newEntry){
      /* handle error ... */
      printk(KERN_WARNING "WARNING: COULD NOT ALLOCATE MEMORY, returning from function...\n");
      return -1;
    }
    newEntry->sem = newSem;
    newEntry->next = NULL;

    //loop through, set the semaphore in the next open spot
    struct sem_list *tempList = semList;

    while(true){
      if(tempList->next == NULL){
        tempList->next = newEntry;
        break;
      }
      tempList = tempList->next;
    }

  }

  return semIdentifier;
}

/* This syscall opens an already created semaphore by providing the semaphore name and the correct key.
The function returns the identifier of the opened semaphore if the key matches the stored key or -1 otherwise. */
asmlinkage long sys_cs1550_open(char name[32], char key[32]){
  int returnValue = -1;

  //traverse semaphore list and find the correct semaphore
  struct sem_list *tempList = semList;
  while(tempList){
    //holder for the current Semaphore in the list
    struct cs1550_sem *currentSem = tempList->sem;

    //if the name and the key match, set return val to the identifier
    if(strcmp(currentSem->name, name) == 0 && strcmp(currentSem->key, key) == 0){
      returnValue = currentSem->sem_id;
    }

    tempList = tempList->next;
  }

  return returnValue;
}

/* This syscall implements the down operation on an already opened semaphore
 using the semaphore identifier obtained from a previous call to sys_cs1550_create or sys_cs1550_open.
 The function returns 0 when successful or -1 otherwise (e.g., if the semaphore id is invalid or if the queue is full).
Please check the lecture slides for the pseudo-code of the down operation. */

//initialize the lock for the sem list
DEFINE_SPINLOCK(sem_list_lock);
asmlinkage long sys_cs1550_down(long sem_id){
  //lock critical section
  spin_lock(&sem_list_lock);

  //traverse semaphore list and find the correct semaphore
  struct sem_list *tempList = semList;

  //the semaphore we'll be downing on
  struct cs1550_sem *sem = NULL;

  while(tempList){
    //holder for the current Semaphore in the list
    struct cs1550_sem *tempSem = tempList->sem;

    //if we find the ID set currentSem val to be the semaphore
    if(tempSem->sem_id == sem_id){
      sem = tempSem;
    }
    //iterate
    tempList = tempList->next;
  }

  //if the semaphore wasn't found, we return unsuccessful
  if (sem == NULL){
    return -1;
  }

  //decrement the semaphore value
  sem->value = sem->value - 1;

  //if the sem is negative, we add it to the queue and schedule it
  if(sem->value < 0){
    //create a new node
    struct cs1550_queue *node = (struct cs1550_queue*)kmalloc(sizeof(struct cs1550_queue),GFP_ATOMIC);
    if(!node){
      /* handle error ... */
      printk(KERN_WARNING "WARNING: COULD NOT ALLOCATE MEMORY, returning from function...\n");
      return -1;
    }
    //set the node's values
    node->currentProcess = current;
    node->next = NULL;

    //if the head of queue is null, this is the first entry
    if(sem->head == NULL){
      sem->head = node;
    }
    else{
      //else, add the node to the back of the queue
      sem->tail->next = node;
    }
    //set the end of the queue to this new node
    sem->tail = node;

    //set the task
    set_current_state(TASK_INTERRUPTIBLE);
    //release before scheduling
    spin_unlock(&sem_list_lock);
    //send to the scheduler
    schedule();

  }
  else{
    //else release the lock, the process doesnt need to be scheduled
    spin_unlock(&sem_list_lock);
  }

  return 0;
}

/* This syscall implements the up operation on an already opened semaphore
using the semaphore identifier obtained from a previous call to sys_cs1550_create or sys_cs1550_open.
The function returns 0 when successful or -1 otherwise (e.g., if the semaphore id is invalid).
Please check the lecture slides for pseudo-code of the up operation. */
asmlinkage long sys_cs1550_up(long sem_id){
  //lock critical section
  spin_lock(&sem_list_lock);

  //traverse semaphore list and find the correct semaphore
  struct sem_list *tempList = semList;

  //the semaphore we'll up on
  struct cs1550_sem *sem = NULL;

  while(tempList){
    //holder for the current Semaphore in the list
    struct cs1550_sem *tempSem = tempList->sem;

    //if we find the ID set currentSem val to be the semaphore
    if(tempSem->sem_id == sem_id){
      sem = tempSem;
    }

    tempList = tempList->next;
  }

  //if the semaphore wasn't found, we return unsuccessful
  if (sem == NULL){
    return -1;
  }

  //increment the semaphore's value
  sem->value = sem->value + 1;

  //if the process is ready to be woken up
  if (sem->value <= 0){
    //point to the head of the process queue
    struct cs1550_queue *queueHead = sem->head;
    if (queueHead != NULL){

      //if the head of the queue = tail of queue
      //set the head and the tail to NULL
      if(queueHead == sem->tail){
        sem->head = NULL;
        sem->tail = NULL;
      }else{
        //set the head pointer to the next process
        sem->head = queueHead->next;
      }
      //WAKE up the process
      wake_up_process(queueHead->currentProcess);

    }
    //free the memory
    kfree(queueHead);
  }

  //unlock, exit critical section
  spin_unlock(&sem_list_lock);

  return 0;
}

/* This syscall removes an already created semaphore from the system-wide semaphore list
 using the semaphore identifier obtained from a previous call to sys_cs1550_create or sys_cs1550_open.
 The function returns 0 when successful or -1 otherwise (e.g., if the semaphore id is invalid
  or the semaphore's process queue is not empty). */
asmlinkage long sys_cs1550_close(long sem_id){
  //if there is only one entry in the sem list
  if(semList->next == NULL){
    //free the memory of the space
    kfree(semList);
    //set this spot to NULL
    semList = NULL;
    //reset id counter
    semIdentifier = 0;
    //return successful
    return 0;
  }

  //traverse semaphore list and find the correct semaphore
  struct sem_list *tempList = semList;
  struct sem_list *tempListNext = semList->next;

  //the semaphore we'll remove
  struct cs1550_sem *sem = NULL;

  while(tempListNext){
    //holder for the current Semaphore in the list
    struct cs1550_sem *tempSem = tempListNext->sem;

    //if we find the ID set currentSem val to be the semaphore were going to remove
    //set the previous entry to the next of the entry were removing
    if(tempSem->sem_id == sem_id){
      sem = tempSem;
      tempList->next = tempListNext->next;
      break;
    }

    tempList = tempList->next;
    tempListNext = tempListNext->next;
  }

  //if the semaphore wasn't found we return
  if (sem == NULL){
    return -1;
  }

  //free the memory of the semaphore we just removed
  kfree(sem);

  return 0;
}
