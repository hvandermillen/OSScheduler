// must compile with: gcc  -std=c99 -Wall -o oslab oslab.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* WE will use the three-state model */
#define READY 0
#define RUNNING 1
#define BLOCKED 2
#define MAXNUM 9999 // pid must be less than this number

/* string of process states */
char states[3][10] = {"ready", "running", "blocked"};

/* Information about each process */
/* Important: Feel free to add any new fields you want. But do not remove any of
 * the cited one. */
typedef struct process_info {
  int pid;             // process ID, cannot be negative
  int state;           // state of the process: ready, running, blocked
  int arrival_time;    // time the process arrives
  int complete_time;   // time the process finishes
  int turnaround_time; // complete_time - arrival_time
  int cpu1;            // first computing interval
  int io;              // io interval
  int cpu2;            // second computing interval
  int interval;        // the current interval (cpu1, io, cpu2)
  int arrived;         //initialized to 0, set to 1 at the arrival time
  int done;            // This flag is initialized to 0 and 
                      //is set to 1 when the process is done
  int remaining_cpu1;
  int remaining_cpu2;
  int remaining_io;
} process_info;

/**** Global Variables ***/

process_info *process_list;         // array containing all processes info
int num_processes = 0; // total number of processes

/* You will implement the following functions */
/* Feel free to add more functions if you need. */

void process_file(FILE *fp); // Reads the input file and fill the process_list
int scheduler(int, int); // Returns the next process to be scheduled for execution
                    // (i.e. its state becomes RUNNING)
int update_running_process(int, int);
void update_blocked_processes(
    int); // decrement the io field in all blocked processes.
void print_processes(FILE *, int,
                     int); // print blocked/ready processes into the output file

/**********************************************************************/

int main(int argc, char *argv[]) {

  FILE *fp;                // for creating the output file
  FILE *output;            // output file
  char filename[100] = ""; // the output file name
  int cpu_active = 0;      // Incremented each cycle the CPU is busy
  int time;                // The clock
  int num_completed_processes = 0;
  int running_process_index = -1; // index of the process currently on the cpu
  int i;

  // Check that the command line is correct
  if (argc != 2) {

    printf("usage:  ./oslab filename\n");
    printf("filename: the processese information file\n");
    exit(1);
    //argv[1] = "test_input_2.txt";
  }

  // Process the input command line.

  // Check that the file specified by the user exists and open it
  if (!(fp = fopen(argv[1], "r"))) {
    printf("Cannot open file %s\n", argv[1]);
    exit(1);
  }

  // process input file
  process_file(fp);

  // form the output file name: original name + number of processes + .out
  strcat(filename, argv[1]);
  sprintf(filename, "%s%d", filename, num_processes);
  if (!(output = fopen(filename, "w"))) {
    printf("Cannot open file %s\n", filename);
    exit(1);
  }

  // The main loop
  time = 0;
  while (num_completed_processes < num_processes) {
    
      //try to schedule a process
    running_process_index = scheduler(running_process_index, time);
     print_processes(output, time, 0);
    
    //update running process
    switch (update_running_process(running_process_index, time)){
      case 0: //process keeps running
        cpu_active++;
        break;
      case 1: //blocks for IO, try to schedule a new process
        cpu_active++;
        running_process_index = -1;
        break;
      case 2: //process completed, try to schedule a new process
        cpu_active++;
        num_completed_processes++;
        running_process_index = -1;
        break;
      case 3: //no process ran, try to schedule a new process
        break;
    }

    
    update_blocked_processes(time);

    //icnrement time
    time++;
  }

  printf("num processes = %d\n", num_processes);
  printf("CPU utilization = %f\n", (float)cpu_active / (float)time);
  printf("total time = %d\n", time);
  for (i = 0; i < num_processes; i++)
    printf("process %d: turnaround time = %d\n", process_list[i].pid,
           process_list[i].turnaround_time);

  // close the processes file
  fclose(fp);
  fclose(output);
  free(process_list);

  return 0;
}

/**********************************************************************/
/* The following function does the following:
- Reads the number of process from the input file and save it in the global
variable: num_processes
- Allocates space for the processes list: process_list
- Read the file and fill up the info in the process_info array
- Keeps reading the file and fill up the information in the process_list
*/
void process_file(FILE *fp) {

  int i = 0;
  fscanf(fp, "%d", &num_processes);

  if ((process_list = malloc(num_processes * sizeof(process_info))) == NULL) {
    printf("Failure to allocate process list of %d processes\n", num_processes);
    exit(1);
  }

  while (fscanf(fp, "%d %d %d %d %d", &process_list[i].pid,
                &process_list[i].cpu1, &process_list[i].io,
                &process_list[i].cpu2, &process_list[i].arrival_time) == 5) {

    process_list[i].done = 0;
    process_list[i].remaining_cpu1 = process_list[i].cpu1;
    process_list[i].remaining_cpu2 = process_list[i].cpu2;
    process_list[i].remaining_io = process_list[i].io;
    process_list[i].state = READY;
    i++;
  }
} // END OF process_file

//returns the index of the next process to run
int scheduler(int running_process_index, int time) {
  int lowest_pid = MAXNUM;
  int nextProcess = -1; // the index of the next process to be scheduled
  for (int i = 0; i < num_processes; i++) {
    if (process_list[i].arrived == 0 && process_list[i].arrival_time <= time) {
      process_list[i].arrived = 1;
    }
    // search through all ready processes to find the one with the lowest PID
    if ((process_list[i].state <= RUNNING) &&
        (process_list[i].pid < lowest_pid) &&
        process_list[i].arrived == 1 && process_list[i].done == 0) {
      lowest_pid = process_list[i].pid;
      nextProcess = i;
    }
  }

  if (nextProcess != -1) {
    //update the state of the process
    process_list[nextProcess].state = RUNNING;
    if(nextProcess != running_process_index) {
      process_list[running_process_index].state = READY;
    }
  } else { 
    //if no other process is ready, keep running the current one
    return running_process_index;
  }

  
  
  return nextProcess;
}

// return 0 if the process keeps running, 1 if it blocks for IO, 2 if the
// process completes, 3 if no process runs
int update_running_process(int pid, int time) {
  if (pid == -1) { // no process is running
    return 3;
  }
  //make a pointer to the current running process
  process_info* running_process = &process_list[pid];

  switch (running_process->interval) {
  case 0: // process is in the CPU1 interval
    if (running_process->remaining_cpu1 > 1) { // process has remaining time in cpu1 interval
      running_process->remaining_cpu1 -= 1; // decrease remaining cpu1 time
    } else {
      // block for io
      running_process->interval++; // set interval to blocked for io
      running_process->state = BLOCKED;
      return 1;
    }
    break;
  case 2:
    if (running_process->remaining_cpu2 > 1) { // process has remaining time in cpu1 interval
      running_process->remaining_cpu2--; // decrease remaining cpu1 time
    } else {
      // process completes
      running_process->done = 1; // mark process as completed
      running_process->complete_time = time;
      running_process->turnaround_time =
          running_process->complete_time - running_process->arrival_time;
      return 2;
    }
    break;
  }

  return 0; // the process keeps running
}

// decrement the io field in all blocked processes
void update_blocked_processes(
    int time) { 
  process_info* current_process;

  // iterate through process_list to update all blocked processes
  for (int i = 0; i < num_processes; i++) {
    current_process = &process_list[i];
    
    if (current_process->state == BLOCKED &&
        current_process->arrived == 1 && current_process->done == 0) {
      if (current_process->remaining_io > 0) {
        // if it is still waiting for IO, reduce the time left waiting
        current_process->remaining_io--;
      } else {
        // the process is done blocking for IO, it can be added to the ready
        // queue
        current_process->interval++;
        current_process->state = READY;
      }
    }
  }
}

// print blocked/ready processes into the output file
void print_processes(FILE* fp, int time, int running_process_index) { 

  //start each section by printing the time
  fprintf(fp, "time: %d\n", time);

  for (int i = 0; i < num_processes; i++) {
    //for every process (that has arrived and not completed), print its status
    if ((process_list[i].arrival_time <= time) && (process_list[i].done == 0) && (process_list[i].state == RUNNING)) {
      fprintf(fp, "process: %d: %s\n",process_list[i].pid, states[process_list[i].state]);
    }
  }
  
  for (int i = 0; i < num_processes; i++) {
    //for every process (that has arrived and not completed), print its status
    if ((process_list[i].arrival_time <= time) && (process_list[i].done == 0) && (process_list[i].state != RUNNING)) {
      fprintf(fp, "process: %d: %s\n",process_list[i].pid, states[process_list[i].state]);
    }
  }

  fprintf(fp, "\n");
  
} 

/**********************************************************************/