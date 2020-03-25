#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <limits.h>

void interruptHandler (int sig);
int internal (char *line[]);
int get_a_line(char *argu[]);
void fifocreate(char *line);
void fifopen(int fd,char buffer, int BUFFER_SIZE, char* fifopath);
void fifoclose(int fd ,char buffer, char* fifopath);
void my_system(char *line[], int ret);

char history[100] [50];
int historyrun=0;

void fifocreate(char *line){
    printf("\n MKFIFO runs \n");

    char buf [PATH_MAX];
    char *fifopath=realpath(line[2], buf); //setting real path of chdir.
    char pathname=line[1];

    if((mkfifo(fifopath,pathname))==-1){
        puts("Error");
    }
    int pid3=fork();
    int BUFFER_SIZE=256, fd;
    char buffer[BUFFER_SIZE];

    if(pid3==0){
        fd=open(fifopath,O_WRONLY);
        fgets(buffer, BUFFER_SIZE, stdin);
        write(fd,buffer, strlen(buffer));
        close(fd);
    }else{
        fd=open(fifopath,O_RDONLY);
        read(fd,buffer, sizeof(buffer));
        close(fd);
    }

}

 void interruptHandler (int sig){
  printf("Caught interrupt: %i\n",sig);
  if(sig==SIGINT){
    fflush(stdout);//flush all buffered memory in this case.
  }
}
void piping(char *line[]){
    printf("We are in PIPING");
    int fd[2];
    pid_t pid1=fork();
    if(pid1==0){
        if (execvp((const char *) line[0], (char *const *) &line[0])) {
            printf("\nError, cmd 1 not recognized\n");
        }
        close(1);
        dup(fd[1]);
        close(fd[1]);
    }else{
        wait(NULL);
        printf("\nCHILD 1 DONE\n");
        pid_t pid2=fork();
        if(pid2==0){
            close(0);
            dup(fd[0]);
            close(fd[0]);
            if (execvp((const char *) line[1], (char *const *) &line[1])) {
                printf("\nError, cmd 2 not recognized\n");
            }
        }else{
            wait(NULL);
            printf("\nCHILD 2 DONE\n");

        }

    }

}


int main() {
char *argu[100];

  while(1) {


    if(signal (SIGINT , interruptHandler)==SIG_ERR){        //Signal handling, always=Sig_Err since not set.
      exit(1);

    }else if (signal (SIGTSTP, SIG_IGN)==SIG_ERR) {
      printf("Signal CTRL+Z is ignored \n");      //Check this, maybe use interruptHandler for everything.
    }



   int num = get_a_line(argu);

   if (strlen(argu[0]) > 1){
      my_system(argu,num);
    }
  }
  return 0;
}
int get_a_line(char *argu[]) {
    int j = 0, bytes_read;
    int ret=0;
    char *line1 = NULL;
    int size = 256;
    size_t linesz = 0;
    char *token;
    line1 = (char *) malloc(size);
    // if (filename == '\0') { //if (filename==null){}
        bytes_read = getline(&line1, &linesz, stdin);
        if (bytes_read == -1) {
            exit(-1);
        }

        historyrun=historyrun%100;
        strcpy(history[(historyrun%100)], line1);
        historyrun++;

        while ((token = strsep(&line1, "  \t\n")) != NULL) {  //tokenize string, maybe use strsep
            for (int i = 0; i < strlen(token); i++) {
                if (token[i] <= 32) {
                    token[i] = '\0';
                }else if (token[i]==124){
                    ret=-1;
                }
            }
            if (strlen(token) > 0) {
                argu[j++] = token;
            }
        }
        return ret;
}

    void my_system(char *line[], int ret) {
        pid_t pid = fork();



        if (pid == 0) {

            sleep(1);
            if(internal(line)==0){
                exit(0);
            }
            if(ret == -1){ //Check for piping
                piping(line);
                exit(0);
            }

            if (execvp((const char *) line[0], (char *const *) &line[0])) {
                printf("\nError, cmd not recognized\n");
            }


        } else {
            wait(NULL);
            printf("\nCHILD DONE\n");
        }
    }

    int internal(char *line[]) {
        printf("\nThis is running\n");
        char *command = *line;

        if (strcmp(command, "chdir") == 0) {
            printf("This CHDIR works\n");
            char buf [PATH_MAX];
            char *directory=realpath(line[1], buf); //setting real path of chdir.
            if (directory) {
                printf("This source is at %s.\n", buf);
            } else {
                perror("realpath");
                exit(EXIT_FAILURE);
            }
            chdir(buf);
            return 0;
        } else if (strcmp(command, "history") == 0) {
            printf("This HISTORY works\n");
            for (int m = 0; m < historyrun; m++) {
                printf("%s\n",*history);
                m++;
            }
            return 0;
        } else if (strcmp(command, "mkfifo") == 0) {
            printf("This MKFIFO works\n");
            fifocreate(*line);
            return 0;
        } else if (strcmp(command, "limit") == 0) {
            struct rlimit new_lim, old_lim;
            getrlimit(RLIMIT_DATA, &old_lim);
            printf("This LIMIT works\n");
            printf("%s", line[1]);
            int i = atoi((const char *) line[1]);//convert char to int;
            i=i+0;
            new_lim.rlim_max = i;
            if (i<old_lim.rlim_cur){
            new_lim.rlim_cur=i;
            }
            if(setrlimit(RLIMIT_DATA, &new_lim) == -1) {
                printf("\nLimit error, please enter a (positive) integer\n");
            }
            return 0;
        }
        return -1;
    }
