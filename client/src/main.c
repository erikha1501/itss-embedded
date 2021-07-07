#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>

#define SERV_PORT 3000 /*port*/

int sockfd;
struct sockaddr_in servaddr;
int flag = 1;

//data send to commoditySale
int terminateFlag = 0;
int choice;

int main(int argc, char **argv) 
{
    if (argc !=2) {
        perror("Usage: TCPClient <IP address of the server"); 
        exit(1);
    }

    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
        perror("Problem in creating the socket");
        exit(2);
    }
        
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr= inet_addr(argv[1]);
    servaddr.sin_port =  htons(SERV_PORT); 
        
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) >= 0) {
        printf("Connected\n");
        createCommoditySale();
    } else {
        perror("Problem in connecting to the server");
        exit(3);
    }
    exit(0);
}

void printMenu()
{
    while (flag)
    {
        flag = 0;
        printf("====MENU====\n");
        if (scanf("%d", &choice) != NULL)
        {
            printf("User's choice: %d \n",choice);
            if (choice == 0)
            {
                printf("quit\n");
                terminateFlag = 1;
                exit(4);
            }
            flag = 1;
        }
    }
}

void createCommoditySale()
{
    pid_t pid;
    time_t t;
    int status;

    if ((pid = fork()) < 0)
        perror("fork() error");
    else if (pid == 0) {
        time(&t);
        printf("commoditySale (pid %d) started at %s", (int) getpid(), ctime(&t));
        sleep(5);
        time(&t);
        printf("commoditySale exiting at %s", ctime(&t));
        
        exit(42);
    }
    else {
        printf("parent has forked child with pid of %d\n", (int) pid);
        time(&t);
        printf("parent is starting wait at %s", ctime(&t));
        printMenu();
        if (WIFEXITED(status))
            printf("child exited with status of %d\n", WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("child was terminated by signal %d\n",
                WTERMSIG(status));
        else if (WIFSTOPPED(status))
            printf("child was stopped by signal %d\n", WSTOPSIG(status));
        else puts("reason unknown for child termination");
    }

}