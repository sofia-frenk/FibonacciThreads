#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <limits.h>
#include <sys/resource.h>
#include <sys/wait.h>

#define PIPE_NAME "FIBOPIPE"

//----Function to compute the Nth Fibonacci number recursively
long Fibonacci(long WhichNumber) 
{
//----If the 1st or second number, return the answer directly
    if (WhichNumber <= 1) {
        return(WhichNumber);
//----Otherwise return the sum of the previous two Fibonacci numbers
    } else {
        return(Fibonacci(WhichNumber - 2) + Fibonacci(WhichNumber - 1));
    }
}

void* FibThreadMethod(void* fibonacciVariableThread)
{
    extern int numberOfThreads;
    long fibonacciResult;
    //Calculates the Nth Fibonacci number using the recursive O(2n) algorithm \
send in a pointer to the deferenced fibonacciVariableThread, which is an int pointer \
(a copy of N from main)
    fibonacciResult = Fibonacci(*((int*)fibonacciVariableThread));
    //thread must print out the result before terminating
    printf("S: Fibonacci %d is %ld\n", *((int*)fibonacciVariableThread), fibonacciResult);
    //free malloced memory
    free(fibonacciVariableThread);
    //Decrements the number of running threads
    numberOfThreads --;
    return(NULL);
}

//function to set CPU time limit based on user entry
void SetCPUTimeLimit(int CPUTimeLimit)
{
    struct rlimit ResourceLimits;
    //Limit the CPU time. Need to get old one for hard limit field
    if(getrlimit(RLIMIT_CPU, &ResourceLimits) == -1)
    {
        perror("CPULimitedRun -- getting resource limit:");
        exit(EXIT_FAILURE);
    }
    //set new CPU limit in ms (sent in secs)
    ResourceLimits.rlim_cur = CPUTimeLimit;
    if(setrlimit(RLIMIT_CPU, &ResourceLimits) == -1)
    {
        perror("CPULimitedRun -- getting resource limit:");
        exit(EXIT_FAILURE);
    }
    printf("S: Setting CPU limit to %llus\n", ResourceLimits.rlim_cur);
}

//function to compute CPU time
void CPUUsage(void)
{
    struct rusage Usage;
    getrusage(RUSAGE_SELF, &Usage);
    printf("S: Server has used %lds %dus\n", Usage.ru_utime.tv_sec+Usage.ru_stime.tv_sec, \
Usage.ru_utime.tv_usec+Usage.ru_stime.tv_usec);
}

//Handler in case parent get a SIGXCPU signal
void SIGXCPUHandler(int theSignal)
{
    extern int ChildPID;
    printf("S: Received a SIGXCPU, ignoring any more\n");
    //handler ignores any further SIGXCPU signals
    signal(SIGXCPU, SIG_IGN); 
    printf("S: Received a SIGXCPU, sending SIGUSR1 to interface\n");
    //handler sends SIGUSR1 to child process
    kill(ChildPID, SIGUSR1);
}


int main(int argc, char** argv)
{
    //create a pointer to int to send to pthread_create
    int* N;
    //create an int for reading values from child into N
    int NasInt;
    extern int ChildPID;
    //set variable for the number of threads created, will be used in future to wait for all to \
complete
    extern int numberOfThreads;
    numberOfThreads = 0;
    FILE* PIPE;
    int Status;
    char* arrayRelatedToChild[] = {"./ThreadFibInterface", PIPE_NAME, NULL};
    pthread_t NewThread;
    
    //unlink the pipe each time the child is run so the space is cleaned
    unlink(PIPE_NAME);
    
    //make the named pipe 
    mkfifo(PIPE_NAME, 0777);
    
    //creation of child process
    if((ChildPID = fork()) == -1) 
    {   
        perror("Could not fork");
        exit(EXIT_FAILURE);
    }   
    //in the child 
    if(ChildPID==0)
    {   
        execlp(arrayRelatedToChild[0], "ThreadFibInterface", PIPE_NAME, NULL);
        perror("Error in exec");
        exit(EXIT_FAILURE);
    }
    //in the parent 
    else
    {
        if(signal(SIGXCPU, SIGXCPUHandler) == SIG_ERR)
        {
            perror("Could not install SIGXCPUHandler");
            exit(EXIT_FAILURE);
        }
    
        //If SIGXCPU interrupted the system call to read off the pipe, that read shouldn't be \
restarted
        if(siginterrupt(SIGXCPU, 1) == -1)
        {
            perror("Abandoning interrupted system calls");
            exit(EXIT_FAILURE);
        }
        siginterrupt(SIGXCPU, 1);
     
        //Limits its CPU usage to the number of seconds given as the command line parameter.
        SetCPUTimeLimit(atoi(argv[1]));

        //Opens the named pipe for reading
        if((PIPE = fopen(PIPE_NAME, "r")) == NULL)
        {   
	    printf("Error!!!!!\n");
            perror("Opening PIPE to read");
            exit(EXIT_FAILURE);
        }
        //receive value from child to read into NasInt
        fscanf(PIPE, "%d", &NasInt);
        while( NasInt>0)
        {
            //Reports the CPU time it has used
            CPUUsage();

            printf("S: Received %d from interface\n", NasInt); 
            //Store NasInt into malloced memory
            N = (int*)malloc(sizeof(int));
            *N = NasInt;
            //Starts and detaches a new thread calculating the Nth Fibonacci number using \
recursive fxn (send int N since it is a pointer to int)
            if(pthread_create(&NewThread, NULL, FibThreadMethod, N) != 0)
            {
                perror("Creating thread");
                exit(EXIT_FAILURE);
            } 
            printf("1:Number of threads: %d\n", numberOfThreads);
            if(pthread_detach(NewThread) != 0)
            {
                perror("Detaching thread");                    
                exit(EXIT_FAILURE);
            }
            printf("S: Created and detached the thread for %d\n", *N);
            printf("2:Number of threads: %d\n", ++numberOfThreads);
        
            fscanf(PIPE, "%d", &NasInt);       
        }

        //Closes the pipe 
        fclose(PIPE);

        //Waits for all threads to complete
        while(numberOfThreads > 0)
        {
            sleep(1);
            printf("S: Waiting for %d thread(s)\n", numberOfThreads);
        }

        //Cleans up the user interface program zombie 
        waitpid(ChildPID, &Status, WNOHANG);
        printf("S: Child %d completed with status %d\n", ChildPID, Status);
 
        //deletes the named pipe
        unlink(PIPE_NAME);
   }
    //exits
    return(EXIT_SUCCESS);
}
int ChildPID;
int numberOfThreads;
