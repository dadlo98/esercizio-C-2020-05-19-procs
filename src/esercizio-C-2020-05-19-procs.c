#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <semaphore.h>

#define N 10

int * countdown;
int * shutdown;
int * process_counter;
sem_t * process_semaphore;

void child_function(int i){
    while(1){
      if (sem_wait(process_semaphore) == -1) {
        perror("sem_wait");
        exit(EXIT_FAILURE);
      }

      if( *countdown > 0 ){
        *countdown = (*countdown -1);
        process_counter[i]++;
      }

      if (sem_post(process_semaphore) == -1) {
        perror("sem_post");
        exit(EXIT_FAILURE);
      }

      if( *shutdown != 0){
        exit(0);
      }

	  }
    exit(0);
}

int main(int argc, char * argv[]) {
    int res;

    process_semaphore = mmap(NULL, // NULL: è il kernel a scegliere l'indirizzo
			sizeof(sem_t) + (N+2)*sizeof(int), // dimensione della memory map
			PROT_READ | PROT_WRITE, // memory map leggibile e scrivibile
			MAP_SHARED | MAP_ANONYMOUS, // memory map condivisibile con altri processi e senza file di appoggio
			-1,
			0); // offset nel file

    if(process_semaphore == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    countdown = (int *) (process_semaphore + 1);
    shutdown = countdown + 1;
    process_counter = countdown + 2;

    printf("initial value of countdown=%d, shutdown=%d\n", *countdown, *shutdown);
    res = sem_init(process_semaphore, 1, 1);
    if(res == -1){
        perror("sem_init");
        exit(EXIT_FAILURE);
    }

    pid_t pid;
    for(int i=0; i < N; i++){
    	pid = fork();
    	switch(pid){
            case 0:
                child_function(i);

                break;
            case -1:
                perror("fork()");
                exit(EXIT_FAILURE);
            default:
                ; 
        }
    }

    sleep(1);
    if (sem_wait(process_semaphore) == -1) {
        perror("sem_wait");
        exit(EXIT_FAILURE);
      }
    *countdown = 100;
    if (sem_post(process_semaphore) == -1) {
      perror("sem_post");
      exit(EXIT_FAILURE);
    }

    while(1){
      if(*countdown == 0){
          (*shutdown) = 1;
          break;
      }
    }

    for(int i=0; i < N; i++){
        if(wait(NULL) == -1){
            perror("wait");
            exit(EXIT_FAILURE);
        }
    }

    for(int i=0; i<N; i++) {
       printf("\nprocess_counter[%d] = %d\n", i, process_counter[i]);
    }

    return 0;
}
