///
///  proj2.c
/// Řešení IOS-projekt2, 28.4.2019
/// Autor: Boris Burkalo (xburka00), VUT FIT
/// přeloženo: gcc 7.3.0
///

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>


/// CATERGORIES ///
#define HACK 0
#define SERF 1
//////////////////

/// RETURNING FROM INIT FUNCTION ///
#define ERROR 1
#define SUCCESS 0
///////////////////////////////////

/// FUNCTION DECLARATIONS ///
void fork_now(void);      /// FORKS TWO HELPING PROCESSES INTO PROCESSES THAT WE WILL BE USING FOR RIVER CROSSING
void fork_and_sem(void);  /// FORKED PROCESSES ARE PROCESSED
int init(void);           /// INITIALIZING SEMAPHORES AND MAPPING SHARED MEMORY (ALSO CHECKS FOR CORRECT MAPPING)
int free_all(void);       /// UNMAPS MAPPED MEMORY, CLOSES AND UNLINKES OPENED SEMAPHORES
void farewell_two(char *name, int count, int other);     /// REALIZES VOYAGE FOR 2 HACKS AND 2 SERFS
void farewell_four(char *name, int count, int category); /// REALIZES VOYAGE FOR 4 HACKS OR 4 SERFS
int args(int argcount, char *argvar[]); /// PROCESSES NEEDED ARGUMENTS
////////////////////////////

/// DEFINING SEMAPHORES   ///
sem_t *sem_test = NULL;   ///
sem_t *sem_print = NULL;  ///
sem_t *sem_exit = NULL;   ///
sem_t *sem_cpt = NULL;    ///
sem_t *sem_queue = NULL;  ///
sem_t *sem_serf = NULL;   ///
sem_t *sem_hack = NULL;   ///
sem_t *sem_count = NULL;  ///
/////////////////////////////

/// DEFINING SHARED MEMORY AND INITIALIZING IT TO NULL ///
int *sh_mem = NULL;
int *sh_action_count = NULL;
int *cat_count = NULL;
int *process_count = NULL;
int *nh_ns = NULL;
int *queue = NULL;
int *cpt_flag = NULL;
int *processes_on_board = NULL;
/////////////////////////////////////////////////////////

/// DEFINING AND INITIALIZING GLOBAL VARIABLES ///
char name[5];
int flag_cat = 0;
int count = 0;
int flag_printed = 0;
int w_min = 20;
FILE *file;
//////////////////////////////////////////////////

/// PARAMETERS  ///
int P = 0;      /// P NUMBER OF PROCESSES FROM EACH CATEGORY
int H = 0;      /// H MAX TIME AFTER WHICH NEW HACKER PROCESS IS CREATED
int S = 0;      /// S MAX TIME AFTER WHICH NEW SERF PROCESS IS CREATED
int R = 0;      /// R MAX DURATION OF VOYAGE
int W = 0;      /// W MAX DURATION OF SLEEP WHEN PROCESS CANNOT ENTER QUEUE
int C = 0;      /// C QUEUE CAPACITY
///////////////////

int main(int argc, char *argv[]) {
  int error_check = 0;
  int arg_check = 0;

  error_check = init();
  if (error_check == ERROR) {
    fprintf(stderr, "ERROR: memory mapping or opening semaphores failed!\n");
    free_all();
    return ERROR;
  }

  arg_check = args(argc, argv);
  if (arg_check == ERROR) {
    fprintf(stderr, "ERROR: incorrect argument format!\n");
    free_all();
    return ERROR;
  }

  pid_t hack_pid;   /// PID FOR MAIN HACKS PROCESS
  pid_t serf_pid;   /// PID FOR MAIN SERFS PROCESS
  queue[0] = 0;
  cat_count[0] = 0;
  cat_count[1] = 0;
  cpt_flag[0] = 1;
  sh_action_count[0] = 1;

  file = fopen("proj2.out", "w");
  if (file == NULL) {
    fprintf(stderr, "ERROR: could not open file!\n");
    free_all();
    return ERROR;
  }
  setbuf(file, NULL); /// SETBUF SO PRINTING IS CHRONOLOGICAL


  /// HELPING (MAIN) HACKER PROCESS ///
  hack_pid = fork();

  if (hack_pid == 0) {
    strcpy(name, "HACK");
    fork_now();
  }
  else if (hack_pid == -1) {
    fprintf(stderr, "ERROR: failed to fork parent!\n");
    free_all();
    fclose(file);
    return ERROR;
  }
  /// HELPING (MAIN) SERF PROCESS, IF FORK FAILS PROGRAM UNMAPS MEMORY, CLOSES SEMAPHORES AND RETURNS FAIL ///
  serf_pid = fork();

  if (serf_pid == 0) {
    strcpy(name, "SERF");
    fork_now();
  }
  else if (serf_pid == -1) {
    fprintf(stderr, "ERROR: failed to fork parent!\n");
    free_all();
    fclose(file);
    return ERROR;
  }

  /// WAITING FOR ALL PROCESSES TO END, ONLY AFTER THAT PARENT PROCESS CAN END THE PROGRAM ///
  for (int i = 0; i <= (P * 2 + 2); i++){
    wait(NULL);
  }
  fclose(file);
  free_all();
  return SUCCESS;
}

int args(int argcount, char *argvar[]){
  char *ptr;

  if (argcount == 7) {
    P = strtol(argvar[1], &ptr, 10);
    if (P < 2 || (P % 2) != 0 || strcmp(ptr, "\0")) {
      return ERROR;
    }
    H = strtol(argvar[2], &ptr, 10);
    if (H < 0 || H > 2000 || strcmp(ptr, "\0")) {
      return ERROR;
    }
    S = strtol(argvar[3], &ptr, 10);
    if (S < 0 || S > 2000 || strcmp(ptr, "\0")) {
      return ERROR;
    }
    R = strtol(argvar[4], &ptr, 10);
    if (R < 0 || R > 2000 || strcmp(ptr, "\0")) {
      return ERROR;
    }
    W = strtol(argvar[5], &ptr, 10);
    if (W < 20 || W > 2000 || strcmp(ptr, "\0")) {
      return ERROR;
    }
    C = strtol(argvar[6], &ptr, 10);
    if (C < 5 || strcmp(ptr, "\0")) {
      return ERROR;
    }
    return SUCCESS;
  }
  else {
    return ERROR;
  }
}

int init(void){
  sh_mem = mmap(NULL, sizeof(sh_mem) * 2, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  cat_count = mmap(NULL, sizeof(cat_count) * 2, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  sh_action_count = mmap(NULL, sizeof(sh_action_count), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  process_count = mmap(NULL, sizeof(process_count), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  nh_ns = mmap(NULL, sizeof(nh_ns) * 2, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  queue = mmap(NULL, sizeof(queue), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  cpt_flag = mmap(NULL, sizeof(cpt_flag), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  processes_on_board = mmap(NULL, sizeof(processes_on_board), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  sem_test = sem_open("/xburka00.proj2.sem", O_CREAT | O_EXCL, 0666, 1);
  sem_print = sem_open("/xburka00.proj2.sem1", O_CREAT | O_EXCL, 0666, 1);
  sem_exit = sem_open("/xburka00.proj2.sem2", O_CREAT | O_EXCL, 0666, 0);
  sem_cpt = sem_open("/xburka00.proj2.sem3", O_CREAT | O_EXCL, 0666, 0);
  sem_queue = sem_open("/xburka00.proj2.sem4", O_CREAT | O_EXCL, 0666, 1);
  sem_hack = sem_open("/xburka00.proj2.sem5", O_CREAT | O_EXCL, 0666, 0);
  sem_serf = sem_open("/xburka00.proj2.sem6", O_CREAT | O_EXCL, 0666, 0);
  sem_count = sem_open("/xburka00.proj2.sem7", O_CREAT | O_EXCL, 0666, 1);

  if (sh_mem == MAP_FAILED || cat_count == MAP_FAILED || sh_action_count == MAP_FAILED || process_count == MAP_FAILED || nh_ns == MAP_FAILED || queue == MAP_FAILED ||
      cpt_flag == MAP_FAILED || processes_on_board == MAP_FAILED) {
    return ERROR;
  }
  if (sem_test == SEM_FAILED || sem_print == SEM_FAILED || sem_exit == SEM_FAILED || sem_cpt == SEM_FAILED || sem_queue == SEM_FAILED || sem_hack == SEM_FAILED ||
      sem_serf == SEM_FAILED || sem_count == SEM_FAILED) {
    return ERROR;
  }
  return SUCCESS;
}

int free_all(void){
  sem_close(sem_test);
  sem_close(sem_print);
  sem_close(sem_exit);
  sem_close(sem_cpt);
  sem_close(sem_queue);
  sem_close(sem_hack);
  sem_close(sem_serf);
  sem_close(sem_count);
  sem_unlink("xburka00.proj2.sem");
  sem_unlink("xburka00.proj2.sem1");
  sem_unlink("xburka00.proj2.sem2");
  sem_unlink("xburka00.proj2.sem3");
  sem_unlink("xburka00.proj2.sem4");
  sem_unlink("xburka00.proj2.sem5");
  sem_unlink("xburka00.proj2.sem6");
  sem_unlink("xburka00.proj2.sem7");
  munmap(sh_mem, sizeof(sh_mem));
  munmap(sh_mem, sizeof(cat_count));
  munmap(sh_mem, sizeof(sh_action_count));
  munmap(sh_mem, sizeof(process_count));
  munmap(sh_mem, sizeof(nh_ns));
  munmap(sh_mem, sizeof(queue));
  munmap(sh_mem, sizeof(cpt_flag));
  munmap(sh_mem, sizeof(processes_on_board));
  return 0;
}

/// FUNCTION CREATS P NUMBER OF PROCESSES FROM EACH CATEGORY (2 * P)
void fork_now(){
  pid_t fork_id;
  int test_param = 0;
  int time_proc = 0;
  while (test_param < P) {
    flag_cat = 0;
    if (!(strcmp(name, "HACK"))) {
      time_proc = H;
    }
    else {
      time_proc = S;
    }
    if (time_proc != 0) {
      usleep((rand() % (time_proc + 1)) * 1000);
    }

    fork_id = fork();
    if (fork_id == 0) {
      if ((strcmp(name, "HACK"))) {
        flag_cat = 1;
      }
      process_count[0]++;
      sem_wait(sem_count);
      cat_count[flag_cat]++;
      count = cat_count[flag_cat];
      sem_post(sem_count);
      fork_and_sem();
    }
    else if (fork_id == -1) {
      fprintf(stderr, "ERROR: failed to fork parent!\n");
      free_all();
      exit(1);
    }
    test_param++;
  }
  for (int i = 0; i <= (P * 2); i++){
    wait(NULL);
  }
  exit(0);
}

/// FUNCTION PROCESSES THE WHOLE EXPERIENCE OF EACH PROCESS
void fork_and_sem(){
  /// PROCESS REPORTS THAT IT WAS STARTED AND CONTINUES. ///
  sem_wait(sem_print);
  fprintf(file, "%d\t: %s %d\t: starts\n", sh_action_count[0]++, name, count);
  sem_post(sem_print);

  /// CHECKS IF QUEUE (WITH THE SIZE OF C) IS FULL, IF NOT IT REPORTS "WAITS" AND STAYS IN QUEUE. IN CASE OF QUEUE BEING FULL, PROCESS REPORTS
  /// "LEAVES QUEUE" AND PROCEEDS TO SLEEP FOR TIME IN INTERVAL 0 TO W. WHEN THE PROCESS WAKES UP, IT REPORTS "IS BACK" AND ONCE AGAIN CHECKS
  /// IF QUEUE IS FULL.
  while (1) {
    sem_wait(sem_print);
    if (queue[0] < C) {
      nh_ns[flag_cat]++;
      fprintf(file, "%d\t: %s %d\t: waits\t\t: %d\t: %d\n", sh_action_count[0]++, name, count, nh_ns[HACK], nh_ns[SERF]);
      queue[0]++;
      sem_post(sem_print);
      break;
    }
    fprintf(file, "%d\t: %s %d\t: leaves queue\t: %d\t: %d\n", sh_action_count[0]++, name, count, nh_ns[HACK], nh_ns[SERF]);
    sem_post(sem_print);
    usleep(((rand() % (W - w_min + 1)) + w_min) * 1000);
    sem_wait(sem_print);
    fprintf(file, "%d\t: %s %d\t: is back\n", sh_action_count[0]++, name, count);
    sem_post(sem_print);
  }

  /// ONCE THERE ARE ENOUGH PROCESSES TO FORM A GROUP OF 4 (THAT CONSISTS OF EITHER 4 HACKS/SERFS OR TWO OF EACH), THE GROUP BOARDS
  /// THE LAST PROCESS TO BOARD THE SHIP GETS TO BE CAPTAIN AND THE SHIP AND SETS ON A SAIL. THE VOYAGE IS REPRESENTED BY PUTTING
  /// CAPTAIN TO SLEEP FOR TIME IN INTERVAL FROM 0 TO R.
  sem_wait(sem_test);
  if (flag_cat == HACK) {
    processes_on_board[flag_cat]++;
    if (processes_on_board[flag_cat] == 4) {
      for (int i = 0; i < 4; i++) {
        sem_post(sem_hack);
      }
      farewell_four(name, count, HACK);
    }
    else if (processes_on_board[HACK] == 2 && processes_on_board[SERF] >= 2) {
      for (int i = 0; i < 2; i++) {
        sem_post(sem_hack);
      }
      for (int i = 0; i < 2; i++) {
        sem_post(sem_serf);
      }

      farewell_two(name, count, SERF);
    }
    else {
      sem_post(sem_test);
    }
    sem_wait(sem_hack);
  }
  else if (flag_cat == SERF) {
    processes_on_board[flag_cat]++;
    if (processes_on_board[flag_cat] == 4) {
      for (int i = 0; i < 4; i++) {
        sem_post(sem_serf);
      }
      farewell_four(name, count, SERF);
    }
    else if (processes_on_board[SERF] == 2 && processes_on_board[HACK] >= 2) {
      for (int i = 0; i < 2; i++) {
        sem_post(sem_serf);
      }
      for (int i = 0; i < 2; i++) {
        sem_post(sem_hack);
      }
      farewell_two(name, count, HACK);
    }
    else {
      sem_post(sem_test);
    }
    sem_wait(sem_serf);
  }

  /// AFTER THE VOYAGE THE "cpt_flag" REPRESENTS NUMBER OF PROCESSES THAT LEFT THE SHIP, WHEN ALL THREE PASSANGERS LEAVE THE SHIP
  /// THE CAPTAIN GETS TO LEAVE AS WELL AND LETS ANOTHER GROUP ON BOARD BY UNLOCKING THE SEMAPHORE.
  sem_wait(sem_exit);
  if (cpt_flag[0] < 4) {
    sem_wait(sem_print);
    fprintf(file, "%d\t: %s %d\t: member exits\t: %d\t: %d\n", sh_action_count[0]++, name, count, nh_ns[HACK], nh_ns[SERF]);
    cpt_flag[0]++;
    sem_post(sem_print);
    if (cpt_flag[0] == 4) {
      sem_post(sem_cpt);
    }
    sem_post(sem_exit);
  }
  else if (cpt_flag[0] == 4) {
    sem_wait(sem_print);
    fprintf(file, "%d\t: %s %d\t: captain exits\t: %d\t: %d\n", sh_action_count[0]++, name, count, nh_ns[HACK], nh_ns[SERF]);
    cpt_flag[0] = 1;
    sem_post(sem_print);
    sem_post(sem_test);
  }
  /// WHILE EXITING THE BOAT, ALL CAPTAIN UNLOCKs SEMAPHORE, WHICH LETS PROCESS FROM QUEUE ON BOARD ///
  exit(0);
}

/// FUNCTION TAKES CARE OF CRUISE WHEN THERE ARE 2 PASSANGERS OF EACH CATEGORY ON BOARD
void farewell_two(char *name, int count, int other) {
  sem_wait(sem_print);
  nh_ns[HACK] -= 2;
  nh_ns[SERF] -= 2;
  processes_on_board[flag_cat] = 0;
  processes_on_board[other] -= 2;
  fprintf(file, "%d\t: %s %d\t: boards\t: %d\t: %d\n", sh_action_count[0]++, name, count, nh_ns[HACK], nh_ns[SERF]);
  queue[0] -= 4;
  sem_post(sem_print);
  usleep((rand() % R) * 1000);
  sem_post(sem_exit);
  sem_wait(sem_cpt);
}

/// FUNCTION TAKES CARE OF CRUISE WHEN TERE ARE 4 PASSANGERS OF SAME CATEGORY
void farewell_four(char *name, int count, int category) {
  sem_wait(sem_print);
  nh_ns[category] -= 4;
  processes_on_board[flag_cat] = 0;
  fprintf(file, "%d\t: %s %d\t: boards\t: %d\t: %d\n", sh_action_count[0]++, name, count, nh_ns[HACK], nh_ns[SERF]);
  queue[0] -= 4;
  sem_post(sem_print);
  usleep((rand() % R) * 1000);
  sem_post(sem_exit);
  sem_wait(sem_cpt);
}
