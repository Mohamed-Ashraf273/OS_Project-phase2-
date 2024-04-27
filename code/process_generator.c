#include "headers.h"
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/shm.h>
void clearResources(int);
// make them global so than clearResources can use threm to kill scheduler and clock
int clk_id;
int scheduler_id;
int msg_id;
int shm_id;
struct Process
{
  int process_id;   // process id
  int arrive_time;  // process arival time
  int running_time; // needed time to run this process
  int priority;     // priority of the process
  int memsize;
  // int process_memory;
};

int main(int argc, char *argv[])
{
  /// initialization and signals
  signal(SIGINT, clearResources);
  int algrithm_number=0;
  int slice = 0;
  int number_of_process = 0;//ali must set it by the number of readed processes
  char algorithm[2], slic_num[10], msg[10], shm[10], num_processes[30]; // i use this when convert integer to string
  // TODO Initialization
  // 1. Read the input files.
  /**Read file
   *
   *
   * ALi makes this and store information od each process in struct and
   * then put it in queue or dara structure to send it to scdeduler
   * number_of_process = ?? number of readed processess do this
   */
  FILE* inputfile;
  char buffer[100];
  inputfile = fopen("processes.txt","r");
  if(inputfile == NULL){
    printf("Failed to open the input file.\n");
    exit(1);
  }
  // count number of lines
  while(fgets(buffer,sizeof(buffer),inputfile)!=NULL){
    number_of_process++;
  }
  // return to the second line
  fseek(inputfile,0,SEEK_SET);
  fgets(buffer,sizeof(buffer),inputfile);
  number_of_process--;
  // read file and store it in processes
  struct Process processes[number_of_process];
  for(int i=0;i<number_of_process;i++){
    fscanf(inputfile,"%d\t%d\t%d\t%d\t%d",&processes[i].process_id,&processes[i].arrive_time,&processes[i].running_time,&processes[i].priority,&processes[i].memsize);
   
  }
  fclose(inputfile);

  for(int i=0;i<number_of_process;i++){
    printf("%d\t%d\t%d\t%d\t%d\n",processes[i].process_id,processes[i].arrive_time,processes[i].running_time,processes[i].priority,processes[i].memsize);
  
  }
 
  ///////////////////////Read from user////////////////////////////////
  // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.  //done
  while (!(algrithm_number == 1 || algrithm_number == 2 || algrithm_number == 3))
  {
    printf("Please Enter which algorithm do you want   \n");
    printf("1 is for RR \n");
    printf("2 is for SRTN \n");
    printf("3 is for HPF \n");

    scanf("%d", &algrithm_number);
  }
  while (algrithm_number == 1&&slice<=0)
  {
    printf("RR needs a time slice it is numbers of seconds that RR runs a process please enter it \n");
    scanf("%d", &slice);
  }
  ////////////////////////////convert number to string ////////////////////////////////////
  sprintf(algorithm, "%d", algrithm_number); // to send this information to scheduker
  sprintf(slic_num, "%d", slice);
  sprintf(num_processes, "%d", number_of_process);

  // 3. Initiate and the scheduler and clock processes.
  // 3. Initiate clock process //done.
  ////////////////////////ipc resources////////////////////////////
  msg_id = msgget(IPC_PRIVATE, 0666 | IPC_CREAT); // This is the IPC communication between scheduler and process generator
  shm_id = shmget(IPC_PRIVATE, 8, 0666 | IPC_CREAT);
  if (msg_id == -1 || shm_id == -1)
  {
    printf("Cannot create message queue or shared memory\n");
    raise(SIGINT);
  }
  sprintf(msg, "%d", msg_id);
  sprintf(shm, "%d", shm_id);

  /////////////////////////////////////////////////////////fork///////
  clk_id = fork();
  if (clk_id == -1)
  {
    printf("Error when try to create clock \n");
    exit(-1);
  }
  if (clk_id == 0)
  {
    execl("clk", "clk", NULL);
  }
  else
  {
    // 3. Initiate SCheduler process //done.

    scheduler_id = fork(); // create scheduler process
    if (scheduler_id == -1)
    {
      printf("Error when try to create scheduler \n");
      exit(-1);
    }
    else if (scheduler_id == 0)
    {

      execl("scheduler", "scheduler", msg, shm, algorithm, slic_num, num_processes, NULL);
    }
    else
    {
      initClk();
      int prev = -1; // initial time is equal to 0
      int x;
      int i = 0;                            // for dummy simulation
      int send, size; // size is the size of message number_processess is the number of process in the file
      printf("current time is %d\n", getClk());
      // To get time use this
      while (1)
      {
        // 4. Use this function after creating the clock process to initialize clock//done

        x = getClk(); // if he read the same time
        if (x != prev)
        {
       

          while (i != number_of_process && processes[i].arrive_time == x+1) // if there is a process arrive in this time send it to scheduler
          {
            //printf("processes[%d]: %d\n",i,processes[i].memsize);
            size = sizeof(processes[i]);
            //printf("SendingTime: %d\n",getClk());
            send = msgsnd(msg_id, &(processes[i]), size, !IPC_NOWAIT);
            if (send == -1) // error when try to send so clear resources
            {
              printf("Error when try to send message to scheduler\n");
              kill(getpid(), SIGINT);
            }
            printf("process send\n");
            i++; // this must change by the number of processes arrived from the file
          }

          prev = x;
       
        }
      }
    }
  }
}
// clear resources
void clearResources(int signum) // 7. Clear clock resources //done
{
  printf("process generator\n");
  msgctl(msg_id, IPC_RMID, (struct msqid_ds *)0); // clear msg
  shmctl(shm_id, IPC_RMID, (struct shmid_ds *)0); // clear shared memory
  // send signal to all this is if I kill it from terminal do must kill all other processes
  // killpg(getpgrp(), SIGINT); // if he interrupt all processes he fork interrupt
  // TODO Clears all resources in case of interruption
  destroyClk(true); // clear clock beacuse this processes will run untill I send kill from terinmal or scheduler terminate
                    // kill(clk_id, SIGINT);//

  exit(0);
}
