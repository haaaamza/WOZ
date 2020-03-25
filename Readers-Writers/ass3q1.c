#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
/*xxxxxxxxxxxxx
 * Mian Hamza
 * 260655263
 *
 * Purpose: Create Readers-Writers program,
 * Reader (500 Threads) can access data concurrently
 * Writer (10 Threads) each writer only has exclusive access to data
 */

//<Variable DEC>
static int read_count,glob = 0;
static sem_t rw_mutex, mutex;
static useconds_t tme ;
struct timespec start_w, stop_w, start_r, stop_r;
double time_readnew,time_readmax,time_readmin, time_writenew, time_writemax, time_writemin, avg_read, avg_write;
//<Variable DEC>

//<Function DEC>
double min(double timenew, double timemin);
double compare(double timenew, double timemax);
double underflow(double ns);
//<Function DEC>

void *reader(void *arg){
    for (int m=0; m<60; m++){ //repeat 60 times

        clock_gettime(CLOCK_REALTIME, &start_r);
        if(sem_wait(&mutex)==-1){//start timer here
            exit(2);
        }
        read_count+=1;
        if (read_count==1){
            if (sem_wait(&rw_mutex)==-1){
                exit(2);
            }
        }
        if (sem_post(&mutex)==-1){
            exit(2);
        }//sempost after this we end timer

        clock_gettime(CLOCK_REALTIME,  &stop_r);

        //<TIME>
        long timer=stop_r.tv_sec-start_r.tv_sec;
        long ns=stop_r.tv_nsec-start_r.tv_nsec;
        if (start_r.tv_nsec > stop_r.tv_nsec) { // clock underflow
            --timer;
            ns += 1000000000;
        }
        time_readnew=(double)ns;
        time_readmax=compare(time_readnew,time_readmax);
        time_readmin=min(time_readnew,time_readmin);
        avg_read+=time_readnew;
        //<TIME>


        //<UNDERFLOW>
        avg_read=underflow(avg_read);
        time_readmax=underflow(time_readmax);
        time_readmin=underflow(time_readmin);
        //<UNDERFLOW>

        int reading= glob; //reading value from shared memory.
        int slp=(rand()%101)*1000;
        tme=(useconds_t)(slp);//sleeping
        usleep(tme);

        if (sem_wait(&mutex)==-1){
            exit(2);
        }
        read_count-=1;
        if (read_count==0){
            if(sem_post(&rw_mutex)==-1){
                exit(2);
            }
        }
        if (sem_post(&mutex)==-1){
            exit(2);
        }



    }


}

void *writer(void *arg){
    read_count=0;

    for (int m=0; m<30; m++){ //repeat 30 times

        clock_gettime(CLOCK_REALTIME,&start_w);
        if (sem_wait(&rw_mutex)==-1){//start timer before this
            exit(2);
        }//end timer here

        //***** Critical Section
        glob+=10;//incrementing glob by 10.
        clock_gettime(CLOCK_REALTIME, &stop_w);

        int slp=(rand()%101)*1000;
        tme=(useconds_t)(slp);//sleeping in loop
        usleep(tme);
        //*******End of Critical Section
        //<TIMER>
        long timer=stop_w.tv_sec-start_w.tv_sec;
        long ns=stop_w.tv_nsec-start_w.tv_nsec;
        if (start_w.tv_nsec > stop_w.tv_nsec) { // clock underflow
            --timer;
            ns += 1000000000;
        }
        time_writenew=(double)ns;
        time_writemax=compare(time_writenew,time_writemax);
        time_writemin=min(time_writenew,time_writemin);
        avg_write+=time_writenew;
        //<TIMER>

        //<UNDERFLOW>
        avg_write=underflow(avg_write);
        time_writemax=underflow(time_writemax);
        time_writemin=underflow(time_writemin);
        //<UNDERFLOW>



        if (sem_post(&rw_mutex)==-1){
            exit(2);
        }

    }

}

void main(int argc, char *argv[]) {
    int wrloop=10;//*argv[1]; //10
    int rdloop=500;//*argv[2];//500

    pthread_t red[rdloop], writ[wrloop]; //creating data type for reader and writer
    int s;
    int arg=1;


    if (sem_init(&rw_mutex, 1, 1) == -1) {
        printf("Error, init semaphore\n");
        exit(1);
    }
    if (sem_init(&mutex, 1, 1) == -1) {
        printf("Error, init semaphore\n");
        exit(1);
    }


    for (int y=0; y<rdloop; y++) {
        s = pthread_create(&red[y], NULL, reader, &arg);
        if (s != 0) {
            printf("Error, creating threads\n");
            exit(1);
        }
    }
    for (int w9=0; w9<rdloop; w9++){
        pthread_join(red[w9], NULL);
    }

    for (int z=0; z<wrloop; z++) {
        s = pthread_create(&writ[z], NULL, writer, &arg);
        if (s != 0) {
            printf("Error, creating threads\n");
            exit(1);
        }
    }
    for (int w8=0; w8<wrloop; w8++) {
        pthread_join(writ[w8], NULL); //waiting for pthread to join
    }
    avg_read=avg_read/(500*60);//computing average
    avg_write=avg_write/(10*30);

    printf("glob value %d \n\n", glob);

    //<Print READ>
    printf("Avg. Read time %lf \n", avg_read);
    printf("Max Read time %lf \n", time_readmax);
    printf("Min Read time %lf \n\n", time_readmin);

//<Print READ>

    printf("------------------------\n");
//<Print WRITE>
    printf("Avg. Write time %lf \n", avg_write);
    printf("Max Write time %lf \n", time_writemax);
    printf("Min Write time %lf \n", time_writemin);
//<Print WRITE>


    exit(0);
}
//<MAX>
double compare(double timenew, double timemax){
    if(timenew>timemax){
        timemax=timenew;
    }
    return timemax;
}
//<MAX>
//
//<MIN>
double min(double timenew, double timemin){
    if(timenew<timemin){
        timemin=timenew;
    }
    return timemin;
}
//<MIN>

double underflow(double ns){
    if (ns<0){
        ns += 1000000000;
    }
    return ns;
}