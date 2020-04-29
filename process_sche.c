#define _GNU_SOURCE  
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <syslog.h>


#include "process_sche.h"


#ifndef SCHED_IDLE
#define SCHED_IDLE 5
#endif
#define KERN_INFO "<6>"

int core_assign(int pid,int core){

	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(core,&set);

	sched_setaffinity(pid,sizeof(set),&set);

	return 0;

}


int process_create(int exe_time){
    
    pid_t processId;
    
    processId = fork();
    
    if (processId < 0) {
        
        perror("fork error");
        
    } else if ( processId == 0) { /* Child Process */
	unsigned long begin_s, begin_ns, end_s, end_ns;
	char kernel_info[60];
	struct timeval tv;
	//syscall(78, &tv, NULL); 
	gettimeofday(&tv,NULL);
	begin_s = (unsigned long)tv.tv_sec, begin_ns = (unsigned long)tv.tv_usec * 100; //開始計時

	//debug////////
	//printf("start:%ld\n",begin_s);
	/////////
	
	/*
        for (int i=0; i<exe_time; i++) {
            UNIT_TIME;
	
            
            // if((i%9)==0)
            //     printf("Child[%d] already run %d unit time\n",getpid(),i);
        }//在跑完exetime之前跑了幾個unittime
        */
	
	//syscall(78, &tv, NULL);
	gettimeofday(&tv , NULL);
	end_s =(unsigned long) tv.tv_sec, end_ns =(unsigned long) tv.tv_usec * 		100;//結束計時
	//printf("end:%ld\n",end_s);
	/*
	sprintf(kernel_info, "[project1] %d %lu.%09lu %lu.%09lu\n", getpid(), 		begin_s, begin_ns, end_s, end_ns);
	printf("%s\n", kernel_info);
	openlog("slog",LOG_PID|LOG_CONS,LOG_USER);
	syslog(LOG_INFO,"%s\n",kernel_info); //sys_syslog
	closelog();
	*/

        exit(0);
        
    } else { /* Parent Process */
        
	
        
		core_assign(processId,CHILD_CPU);//把子行程綁在另一顆cpu上

        return processId; //將子程序的pid回傳
    }
    
    return EXIT_FAILURE;

}


int process_stop(int pid){

	struct sched_param sp;
	sp.sched_priority = 0;

	int t = sched_setscheduler(pid,SCHED_IDLE,&sp);


	return 0;
}

int process_wake(int pid){

	struct sched_param sp;
	sp.sched_priority = 0;

	int t = sched_setscheduler(pid,SCHED_OTHER,&sp);


	return 0;
}



