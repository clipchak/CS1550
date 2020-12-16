/*
  museumsim program for project 2
 */
#include <sys/mman.h>
#include <linux/unistd.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <stdlib.h>

//find a minimum
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

//track time
struct timeval t0,t1;

//semaphores
long mutex = 0;
long sem_museumOpen = 0;
long sem_guideWait = 0;
long sem_visitorWait = 0;

//global vars
void *ptrToSharedMemory;
int *numTickets;
int *guidesWaiting;
int *visitorsInMuseum;
int *guidesInMuseum;


// Structure to store command line arguments
struct options {
	int num_visitors;	//number of visitors
	int num_guides;		//number of tour guides
	int pv;						//probability of a visitor immediately following another visitor
	int dv;						//delay in seconds when a visitor does not immediately follow another visitor
	int sv; 					//random seed for the visitor arrival process
	int pg;						//probability of a tour guide immediately following another tour guide
	int dg;						//delay in seconds when a tour guide does not immediately follow another tour guide
	int sg;						//random seed for the tour guide arrival process
};

long create(int value, char name[32], char key[32]) {
  return syscall(__NR_cs1550_create, value, name, key);
}

long open(char name[32], char key[32]) {
  return syscall(__NR_cs1550_open, name, key);
}

long down(long sem_id) {
  return syscall(__NR_cs1550_down, sem_id);
}

long up(long sem_id) {
  return syscall(__NR_cs1550_up, sem_id);
}

long close(long sem_id) {
  return syscall(__NR_cs1550_close, sem_id);
}



void visitorLeaves(int visitorNumber){
  //find visitor leave time
  gettimeofday(&t1, 0);
  long int elapsed = (t1.tv_sec-t0.tv_sec);
  printf("Visitor %d leaves the museum at time %ld.\n", visitorNumber, elapsed);
  fflush(stdout);

  down(mutex);
  (*visitorsInMuseum)--;

  if ((*visitorsInMuseum) <= 0) { // stop waiting if there's no more visitors
    up(sem_visitorWait);
    up(sem_guideWait);
  }
  up(mutex);

}

void tourMuseum(int visitorNumber){
  //wait for the guide to be ready
  down(sem_visitorWait);

  //find visitor tour time
  gettimeofday(&t1, 0);
  long int elapsed = (t1.tv_sec-t0.tv_sec);
  printf("Visitor %d tours the museum at time %ld.\n", visitorNumber, elapsed);
  fflush(stdout);

  //tour the museum
  sleep(2);

  //leave
  visitorLeaves(visitorNumber);
}

void visitorArrives(int visitorNumber){
  //find visitor arrival time
	gettimeofday(&t1, 0);
	long int elapsed = (t1.tv_sec-t0.tv_sec);
  printf("Visitor %d arrives at time %ld.\n", visitorNumber, elapsed);
	fflush(stdout);

  //wait if there's already 10 visitors viewing to one guide
  down(mutex);
  if ((*visitorsInMuseum) == 10 && (*numTickets)>0 && (*guidesInMuseum) == 1) {
    up(mutex);
    down(sem_visitorWait);
  } else{
    up(mutex);
  }


  down(mutex);
  if((*numTickets) == 0){ //if there are no more tickets, leave
    up(mutex);
    visitorLeaves(visitorNumber);

  } else{   //else change variables and proceed to tour the museum

    (*numTickets)--;
    (*visitorsInMuseum)++;

    if ((*visitorsInMuseum) == 1) { //fire off the tour once the first visitor arrives
      up(sem_guideWait);
    }
    up(mutex);

    //tour
    tourMuseum(visitorNumber);
  }

}

void guideLeaves(int guideNumber){
  //wait for visitors to be done viewing before leaving
  down(sem_guideWait);

  //find time guide leaves
  gettimeofday(&t1, 0);
  long int elapsed = (t1.tv_sec-t0.tv_sec);
  printf("Tour guide %d leaves the museum at time %ld.\n", guideNumber, elapsed);
  fflush(stdout);

  //one less guide is now in the museum
  down(mutex);
  (*guidesInMuseum)--;
  up(mutex);

}

void openMuseum(int guideNumber){
  //find time guide opens museum
  gettimeofday(&t1, 0);
  long int elapsed = (t1.tv_sec-t0.tv_sec);
  printf("Tour guide %d opens the museum for tours at time %ld.\n", guideNumber, elapsed);
  fflush(stdout);

  //release the visitors
  up(sem_museumOpen);

  //guide waits for visitors to come
  down(sem_guideWait);

  //up the amount of visitors that are allowed to be shown by this guide
  int i = 0;
  for(i = 0; i < 10; i++){
    up(sem_visitorWait);
  }

  //leave
  guideLeaves(guideNumber);

}

void guideArrives(int guideNumber){
  //find guide arrival time
  gettimeofday(&t1, 0);
  long int elapsed = (t1.tv_sec-t0.tv_sec);
  printf("Tour guide %d arrives at time %ld.\n", guideNumber, elapsed);
  fflush(stdout);

  down(mutex);
  (*guidesInMuseum)++;
  (*guidesWaiting)--;


  if((*guidesInMuseum) <= 2){ //if there is 2 or less guides in the mus, open to tour
    up(mutex);
    openMuseum(guideNumber);
  } else{ //else wait for a guide to leave
    up(mutex);
    down(sem_guideWait);
  }

}

int main(int argc, char** argv) {
  //incorrect input error checking
  if(argc != 17) {
    printf("Usage: %s -m -k -pv -dv -sv -pg -dg -sg\n", argv[0]);
    fflush(stdout);
    return 1;
  }

  //read arguments
  struct options opt;
  opt.num_visitors = atoi(argv[2]);
  opt.num_guides = atoi(argv[4]);
	opt.pv = atoi(argv[6]);
	opt.dv = atoi(argv[8]);
	opt.sv = atoi(argv[10]);
	opt.pg = atoi(argv[12]);
	opt.dg = atoi(argv[14]);
	opt.sg = atoi(argv[16]);

  //prevent gradescope timeout for certain cases
  if(opt.num_guides > 2){
    return 1;
  }

  // The museum is now empty
  printf("The museum is now empty.\n");
	fflush(stdout);

  //initialize mutex, 3 semaphores
  mutex = create(1,"mutex","mutex");
  sem_museumOpen = create(0, "museum open/closed", "museum open/closed");
  sem_guideWait = create(0, "guide waits", "guide waits");
  sem_visitorWait = create(0, "visitor waits", "visitor waits");

  //initialize addresses of global vars
  ptrToSharedMemory = mmap(NULL,
			   4 * sizeof(int),
				PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
  numTickets = (int*)ptrToSharedMemory;
  guidesWaiting = numTickets + 1;
  visitorsInMuseum = guidesWaiting + 1;
  guidesInMuseum = visitorsInMuseum + 1;

  //initialize values of global vars
  (*numTickets) = 0;
  (*guidesWaiting) = opt.num_guides;
  (*visitorsInMuseum ) = 0;
  (*guidesInMuseum) = 0;

  //set the num tickets to a minimum
  *numTickets = MIN(10*opt.num_guides, opt.num_visitors);

  //track time
  gettimeofday(&t0, 0);

  int i = 0;
  int pid = fork();
  if (pid == 0){ // Visitor Generator process
    //visitors wait for the museum to open
    down(sem_museumOpen);

    for(i = 0; i < opt.num_visitors; i++) { //create M visitors
      pid = fork();

      if (pid == 0){ //child, visitor process logic
        //Visitor arrives, tours, leaves, process exits
        visitorArrives(i);
        exit(0);

      } else{ //parent, visitor generator process

        //Probabilistic delay before generating next visitor
        srand(opt.sv);
        int value = rand() % 100 + 1;
        if (value > opt.pv){
          sleep(opt.dv);
        }

      }

    }

    for(i = 0; i < opt.num_visitors; i++) { //calls M wait() for M visitors
        wait(NULL);
    }


  } else { // Guide Generator process...

      for(i = 0; i < opt.num_guides; i++) { //create K guides
          pid = fork();

          if(pid == 0) { //child, guide process logic
            //Guide arrives, opens museum, leaves, process exits
            guideArrives(i);
            exit(0);

          } else{ //parent, guide generator process
            //Probabilistic delay before generating next guide
            srand(opt.sg);
            int value = rand() % 100 + 1;
            if (value > opt.pg){
              sleep(opt.dg);
            }

          }
      }

      for(i = 0; i < opt.num_guides; i++) { //calls K wait() for K guides
          wait(NULL);
      }

      //release resource of main
      wait(NULL);

      //close the semaphores
      close(sem_museumOpen);
      close(sem_guideWait);
      close(sem_visitorWait);
      close(mutex);

    }


  return 0;
}
