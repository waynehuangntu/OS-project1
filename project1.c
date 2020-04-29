#define _GNU_SOURCE
#define _USE_GNU

#include "process_sche.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>


const int MaxProcessNum = 1E3;
const int time_quantum = 5E2; //RR會用到
const long long n = 1E9;

int NumP_InList = 0;  //在Waiting list中的ProcessNum
int FinshNum = 0;  //執行完的ProcessNum
#define FIFO 0
#define RR 1
#define SJF 2
#define PSJF 3

typedef struct process
{
    char name[33];
    int ready_time, exec_time, ID;
    int have_exectime;
    pid_t pid;
	unsigned long begin_s,begin_ns,end_s,end_ns;
	
} Process;

void gettime_start(Process *P)
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	P->begin_s = (unsigned long)tv.tv_sec;
	P->begin_ns = (unsigned long)tv.tv_usec*100;
}

void gettime_end(Process *P)
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	P->end_s = (unsigned long)tv.tv_sec;
	P->end_ns = (unsigned long)tv.tv_usec*100;
}

int fifo(struct process *p,int countoftime, int nproc){
    int next = -1;
    for(int i = 0; i < nproc;i++){
        if(p[i].pid == -1 || p[i].pid == -2)
            continue;
        if(next == -1 || p[i].ready_time < p[next].ready_time)
            next = i; //較早到的先做
    }
    return next;
}

int sjf(struct process *p,int countoftime, int nproc){
    int next = -1;
    for(int i = 0; i < nproc;i++){
        if(p[i].pid == -1 || p[i].pid == -2)
            continue;
        if(next == -1 || p[i].exec_time - p[i].have_exectime < p[next].exec_time - p[next].have_exectime)
            next = i;
        //同時包含了non-preemptive and preemptive sjf
        //non-preemptive:have_exectime = 0 所以exec_time比較小的先跑
        //preemptive:remaing haven't exec_time 小的先跑
        
    }
    return next;
}

int rr(struct process *p,int countoftime, int nproc){
    int next = -1;
    for(int i = 0; i < nproc;i++){
        if(p[i].pid == -1 || p[i].pid == -2)
            continue;
        if(next == -1)
            next = i;
    }
    return next;
}

int rr_context_switch(struct process *p, int countoftime, int nproc, int running){
    int next = running;
    for(int i = 0; i < nproc;i++){
        next = (next+1) % nproc;
        if (p[next].pid != -1 && p[next].pid != -2) break;
    }
    return next; //switch 下一個process
}

int do_policy(Process P[], int policy, int CountofTime, int ProcessNum){

    if(policy == 0){
        return fifo(P, CountofTime, ProcessNum);
    }
    else if(policy == 1){
        return rr(P, CountofTime, ProcessNum);
    }
    else if(policy == 2){
        return sjf(P, CountofTime, ProcessNum);
    }
    else if(policy == 3){
        return sjf(P, CountofTime, ProcessNum);
    }
    return 0;
}

int main()
{
    core_assign(getpid(), SCHED_CPU); //first set the affinity, then the rest child processes created via fork() will inherit it
    Process P[MaxProcessNum];
    char policy_name[10];
    scanf("%s", policy_name);
    int ProcessNum;
    scanf("%d", &ProcessNum);
    for(int i = 0; i < ProcessNum; i++)
    {
        scanf("%s %d %d", P[i].name, &P[i].ready_time, &P[i].exec_time);
        P[i].pid= -2;
        P[i].have_exectime= 0;   
    }

    int policy = 0;
    char list[4][5] = {"FIFO", "RR", "SJF", "PSJF"};
    for(int i = 0; i < 4; i++){
         if(strcmp(policy_name, list[i]) == 0) {
            policy = i;
            break;
        }
    }
    
    const int priorityH = sched_get_priority_max(SCHED_RR);
    const int priorityL = sched_get_priority_min(SCHED_RR);
    struct sched_param param;
    param.sched_priority = 60;
    //確保目前在CPU core(0)上優先執行此程式
    pid_t pidP = getpid();
    /*if(sched_setscheduler(pidP, SCHED_FIFO, &param) != 0) {
        perror("sched_setscheduler error");
        exit(EXIT_FAILURE);  
    }*/
    
    int CountofTime = 0, CurrentProcessNum = 0; //時間軸, 目前fork出來的ProcessNum
    int running= -1;
    int this_round_exectime= 0;

    while(FinshNum != ProcessNum) //迴圈結束後跑完全部的process
    {
        //printf("running: %d\n", running);
        for (int i = 0; i < ProcessNum; ++i)
        {
            if(P[i].ready_time == CountofTime){
                P[i].pid = process_create(P[i].exec_time); //the ith process fork a new process and execute
                gettime_start(&P[i]);
                process_stop(P[i].pid);
            }
        }

        if(running == -1){ // 沒人READY好
            this_round_exectime= 0;
            int next_process = do_policy(P, policy, CountofTime, ProcessNum);
            //printf("next_process: %d\n", next_process);
            //fflush(stdout);
            if(next_process == -1){
                running= -1;
            }
            else{
                process_wake(P[next_process].pid); //把process的priority從IDLE調整成OTHER
                running= next_process;
            }   
        }

        else if(P[running].have_exectime == P[running].exec_time){ //running跑完
            waitpid(P[running].pid, NULL, 0); //使用waitpid可以等待指定行程
            //printf("time: %d\n", CountofTime);
            gettime_end(&P[running]);
            printf("%s %d\n",P[running].name,P[running].pid);
            char kernel_info[60];
            sprintf(kernel_info, "[project1] %d %lu.%09lu %lu.%09lu\n", 		 	P[running].pid,P[running].begin_s, P[running].begin_ns, 				P[running].end_s,P[running].end_ns);
			printf("%s\n", kernel_info);
            fflush(stdout);
            this_round_exectime= 0;//reset this round exectime
            P[running].pid= -1; //之後不會再pick到
            FinshNum++;
            if(FinshNum == ProcessNum)break;
            int next_process = do_policy(P, policy, CountofTime, ProcessNum);//find next process to execute
            //printf("next_process: %d\n", next_process);
            //fflush(stdout);
            if(next_process == -1){
                running= -1;
            }
            else{
                process_wake(P[next_process].pid);
                running= next_process;
            }             
			//printf("next process %d",next_process);
        }

        else if(policy == RR){
            if(this_round_exectime == 500)//timeslice has meeted
            {
                int next_process= rr_context_switch(P, CountofTime, ProcessNum, running);//switch process
                if(running != next_process)
                {
                    process_stop(P[running].pid);
                    process_wake(P[next_process].pid);
                    running= next_process;
                }
                this_round_exectime= 0;
            }
        }
        else if(policy == PSJF){
            int next_process= sjf(P, CountofTime, ProcessNum);
            if(running != next_process)//有可能重新shcedule還是同一個process
            {
                process_stop(P[running].pid);
                process_wake(P[next_process].pid);
                running = next_process;
            }
        }

        volatile unsigned long i;
        for(i=0;i<1000000UL;i++);

        P[running].have_exectime++;
        this_round_exectime++;
        CountofTime++;//timeline ++
        //printf("running %d",running);
        //printf("%d ", CountofTime);
        //fflush(stdout);
    }

}
