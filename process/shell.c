#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <glob.h>

#define BUFFER_MAX_LENGTH 255
static char* cwd;
static char buf[BUFFER_MAX_LENGTH];
static char *commandArgv[255];
static int commandArgc = 0;
glob_t globbuf;
static int j = 0;
void signal_handler_child() {
	pid_t cpid;
	int terminationStatus;
	while(1){
	cpid= waitpid(-1, &terminationStatus,WNOHANG);
	if(cpid==-1){
		if(errno == EINTR){
			continue;
		}
		break;
	}else if(cpid == 0){
		break;
	}
	}
}

void signal_handler(){
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
	signal(SIGCHLD, &signal_handler_child);
}
void childSignalHandler(){
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
}
void printPrompt(){
	printf("[3150 Shell: %s]$ ", getcwd(cwd, PATH_MAX+1));
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
void tokenCommand()
{
	destroyCommand();
	char* bufPointer;
	char* temp;
	int count = 0;
	int i;

	bufPointer = strtok(buf, " ");
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
void changeDirectory() {
	if (chdir(commandArgv[1]) == -1) {
		printf("%s: cannot change directory\n",commandArgv[1]);
	}
}
int checkBuiltInCommands() {
	if (strcmp("exit", commandArgv[0]) == 0) {
		if (commandArgc == 1) {
			exit(EXIT_SUCCESS);
		} else {
			printf("exit: wrong number of arguments\n");
			return 1;
		}
	}
	if (strcmp("cd", commandArgv[0]) == 0) {
		if (commandArgc == 2) {
			changeDirectory();
			return 1;
		} else {
			printf("cd: wrong number of arguments\n");
			return 1;
		}
	}
	return 0;
}
void processCreation() {
	pid_t child_pid, wpid;
	int status;
	int k;

	if (!(child_pid = fork())) {
		childSignalHandler();
		setenv("PATH", "/bin:/usr/bin:.", 1);

		if (j == 0) {
			execvp(*commandArgv, commandArgv);
		} else {
			for(k=0;k<globbuf.gl_offs;k++){
			globbuf.gl_pathv[k]=commandArgv[k];}
			execvp(globbuf.gl_pathv[0], globbuf.gl_pathv);
		}

		if(errno == ENOENT) {
			printf("%s:No command found\n",commandArgv[0]);
			} else {
				printf("%s: unknown error\n",commandArgv[0]);
			}
		exit(EXIT_SUCCESS);
	}else {
		do {
		wpid = waitpid(child_pid, &status, WUNTRACED | WCONTINUED);
		if (wpid == -1) {
				perror("waitpid");
				exit(EXIT_FAILURE);
			}
		} while(!WIFEXITED(status) && !WIFSIGNALED(status) && !WIFSTOPPED(status) );
	}
}

void commandInterpreter() {
        if (checkBuiltInCommands() == 0) {
		processCreation();
                }
}

int main(int argc, char **argv) {
	cwd = (char*) calloc(PATH_MAX+1, sizeof(char));
	signal_handler();
	printPrompt();
	while(1){
		if (fgets(buf, 255, stdin) != NULL) {
			if (strcmp(buf, "\n") != 0) {
				buf[strlen(buf)-1] = '\0';
      				tokenCommand();
				commandInterpreter();
				destroyCommand();
				printPrompt();
			} else {
				printPrompt();
			}
		} else {
				printf("\n");
				return 0;
		}
		}
	return 0;
}
