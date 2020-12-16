#include <linux/smp_lock.h>

struct cs1550_sem
{
   int value;
   long sem_id;
   spinlock_t lock;
   char key[32];
   char name[32];
   //Some FIFO queue of your devising
   struct cs1550_queue *head;	//head of the queue
   struct cs1550_queue *tail;	//tail of the queue
};

//Add to this file any other struct definitions that you may need
struct cs1550_queue
{
   struct task_struct *currentProcess;	//process representation
   struct cs1550_queue *next;	//next spot in queue
};

struct sem_list
{
  struct cs1550_sem *sem;
  struct sem_list *next;
};
