#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>

// handler for SIGUSR1 signal
void SIGUSR1Handler(int theSignal)
{
    extern int stopTheLoopFlag;
    printf("I: Received a SIGUSR1, stopping loop\n");
    //handler causes the loop below to end
    stopTheLoopFlag=1; //if flag = 1 (TRUE), the loop ends
}

int main(int argc, char** argv)
{
    int N;
    //global variable that the SIGUSR1Handler uses to end the loop
    extern int stopTheLoopFlag;
    stopTheLoopFlag = 0;
    FILE* PIPE;
    //call to signal
    if(signal(SIGUSR1, SIGUSR1Handler) == SIG_ERR)
    {
        perror("Could not set signal handler.\n");
        return(EXIT_FAILURE);
    }
    //If SIGUSR1 interrupted the sys call to read off the pipe, that read shouldn't be restarted
    if(siginterrupt(SIGUSR1, 1) == -1)
    {
        perror("Abandoning interrupted system calls");
        exit(EXIT_FAILURE);
    }    

    //opening the pipe for writing
    if((PIPE = fopen(argv[1], "w")) == NULL)
    {
        perror("Opening FIBOPIPE to write");
        exit(EXIT_FAILURE);
    }
    //while N is not 0
    do
    {
       //Prompts the user and read an integer from the terminal 
        printf("I: Which Fibonacci number do you want : ");
        if(scanf("%d", &N) != 1)
        {
            printf("I: Reading from user abandoned\n");
        }
        if(stopTheLoopFlag)
        {
            N = 0;
        }
        //send N to server
        fprintf(PIPE, "%d\n",N);
 
        //flush the pipe to make sure the integer gets to the Fibonacci server
        fflush(PIPE);
    }while(N!=0);

    //close pipe
    fclose(PIPE);

    //exit
    printf("I: Interface is exiting\n");

    return(EXIT_SUCCESS);
}

int stopTheLoopFlag;
