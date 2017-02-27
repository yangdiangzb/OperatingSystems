#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/times.h>
#include <signal.h>
#include <glob.h>

#define BUFFER_MAX_LENGTH 255
static char buf[BUFFER_MAX_LENGTH];
static char *command[10];
static char *duration[10];
static int taskNumber = 0;
static char *commandArgv[255];
static int commandArgc = 0;
FILE *fp;
pid_t pid;
pid_t pids[10][2];
glob_t globbuf;
static int j = 0;
void fifoHandler(int signal){
	kill(pid,SIGTERM);
}
void paraHandler(int signal){
	pid_t myPid = getpid();
	int i;
	for(i = 0;i<taskNumber;i++){
		if(myPid==pids[i][0]){
			kill(pids[i][1],SIGTERM);
			break;
		}
	}
}
void destroyCommand(){
         while (commandArgc != 0) {
                commandArgv[commandArgc] = NULL;
                commandArgc--;
        }
	j = 0;
	globbuf.gl_offs = 0;
	globfree(&globbuf);
}
void tokenCommand(char *command)
{
        destroyCommand();
        char* bufPointer;
	char* temp;
	int count = 0;
	int i ;
        bufPointer = strtok(command, " ");
        while (bufPointer !=NULL) {
                commandArgv[commandArgc] = bufPointer;
                bufPointer = strtok(NULL, " ");
		temp = commandArgv[commandArgc];
                for(i = 0;i<strlen(temp);i++) {
                        if(temp[i]=='*'){
                                count++;
                        }
                }

                if (count==0) {
                        globbuf.gl_offs++;
                } else {
                        if(j==0) {
                                glob(commandArgv[commandArgc], GLOB_DOOFFS | GLOB_NOCHECK ,NULL,&globbuf);
			}else {
                                glob(commandArgv[commandArgc], GLOB_DOOFFS | GLOB_NOCHECK | GLOB_APPEND, NULL,&globbuf);
                        }

                        j++;
                }
                count = 0;
		commandArgc++;
	}
}
void fifoScheduler(){
	signal(SIGALRM,fifoHandler);
	struct tms cpuTime;
	double ticks_per_sec = (double)sysconf(_SC_CLK_TCK);
	int i;
	for(i=0;i<taskNumber;i++){
		tokenCommand(command[i]);
		clock_t cstartTime,cendTime;
		char *dura = (char*)  calloc(255,sizeof(char));
		int dur;
		strcpy(dura, duration[i]);
		dur = atoi(dura);
		cstartTime = times(&cpuTime);
		int k;
//		printf("%d mom\n",getpid());
		if(!(pid = fork())){
			setenv("PATH", "/bin:/usr/bin:.", 1);
			pid = getpid();
			if (j == 0) {
                        execvp(*commandArgv, commandArgv);
                } else {
                        for(k=0;k<globbuf.gl_offs;k++){
                        globbuf.gl_pathv[k]=commandArgv[k];}
                        execvp(globbuf.gl_pathv[0], globbuf.gl_pathv);
                }
			}
		else{
			alarm(dur);
			wait(NULL);

			cendTime = times(&cpuTime);
			printf("<<Process %d>>\n",pid);
			printf("Time Elapsed:\t%.4f\n",(cendTime-cstartTime)/ticks_per_sec);
	        	printf("User time:\t%.4f\n",cpuTime.tms_cutime/ticks_per_sec);
        		printf("System time:\t%.4f\n",cpuTime.tms_cstime/ticks_per_sec);
		}
	}
}
void paraScheduler(){
	struct tms cpuTime;
	double ticks_per_sec = (double)sysconf(_SC_CLK_TCK);
        int m;
        for(m=0;m<taskNumber;m++){
//		printf("%d big mom\n",getpid());
		if(!(pids[m][0] = fork())){
			signal(SIGALRM,paraHandler);
                        pids[m][0] = getpid();
			setenv("PATH", "/bin:/usr/bin:.", 1);

			tokenCommand(command[m]);
          	        clock_t cstartTime,cendTime;
                	char *dura = (char*)  calloc(255,sizeof(char));
                	int dur;
                	strcpy(dura, duration[m]);
                	dur = atoi(dura);
                	cstartTime = times(&cpuTime);
			int k;
//			printf("%d secondmom\n",getpid());
			if(!(pids[m][1] = fork())){
				pids[m][1] = getpid();
				if (j == 0) {
                        		execvp(*commandArgv, commandArgv);
                		} else {
                        		for(k=0;k<globbuf.gl_offs;k++){
                        		globbuf.gl_pathv[k]=commandArgv[k];}
                        		execvp(globbuf.gl_pathv[0], globbuf.gl_pathv);
                		}

                        }else {
                                alarm(dur);
				wait(NULL);
			
				cendTime = times(&cpuTime);
	                	printf("<<Process %d>>\n",pids[m][1]);
        	        	printf("Time Elapsed:\t%.4f\n",(cendTime-cstartTime)/ticks_per_sec);
                		printf("User time:\t%.4f\n",cpuTime.tms_cutime/ticks_per_sec);
                		printf("System time:\t%.4f\n",cpuTime.tms_cstime/ticks_per_sec);
                		exit(0);
			}
		}

	}
			for (m = 0;m<taskNumber;m++){
				waitpid(pids[m][0],NULL,0);
			} 
}
int main(int argc,char *argv[]) {
	if(argc==3){
		char *mode=argv[1];
		char *file=argv[2];		
		fp = fopen(file,"r");
		
//		clock_t startTime,endTime;
//        	pid_t ppid = getpid();
//	        struct tms cpuTime;
//        	double ticks_per_sec = (double)sysconf(_SC_CLK_TCK);
//	        startTime = times(&cpuTime);

		if(fp == 0){
			printf("Could not open the file!\n");
		}else{
			while(fgets(buf,255,fp) != NULL){
				if(strcmp(buf,"\n")!=0){
					char *bufPointer;
					bufPointer = strtok(buf,"\t");
					command[taskNumber] = (char*)  calloc(255,sizeof(char));
					strcpy(command[taskNumber],bufPointer);
					bufPointer = strtok(NULL,"\t");
					duration[taskNumber] = (char*) calloc(255,sizeof(char));
					strcpy(duration[taskNumber],bufPointer);
					taskNumber++;
				}
			}
			fclose(fp);
		}
		if(strcmp(mode,"FIFO")==0){
			fifoScheduler();
/*			endTime = times(&cpuTime);
	        	printf("<<Process %d>>\n",ppid);
        		printf("Time Elapsed:\t%.4f\n",(endTime - startTime)/ticks_per_sec);
        		printf("User time:\t%.4f\n",cpuTime.tms_utime/ticks_per_sec);
        		printf("System time:\t%.4f\n",cpuTime.tms_stime/ticks_per_sec);
*/
		}else if(strcmp(mode,"PARA")==0){
			paraScheduler();
/*			endTime = times(&cpuTime);
		        printf("<<Process %d>>\n",ppid);
        		printf("Time Elapsed:\t%.4f\n",(endTime - startTime)/ticks_per_sec);
        		printf("User time:\t%.4f\n",cpuTime.tms_utime/ticks_per_sec);
        		printf("System time:\t%.4f\n",cpuTime.tms_stime/ticks_per_sec);
*/
		}else{
			printf("Make sure your first argument is either FIFO or PARA!");
			return 0;
		}		
	}else{
		printf("Make sure you have only TWO arguments!");
		return 0;
	}
	return 0;
}
