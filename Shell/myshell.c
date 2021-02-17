#include <stdio.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <unistd.h> 
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>

void sigchld_handle(int sigchld_num) {
    int saved_errno = errno;
    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
    errno = saved_errno;
}

int signal_handle() {
    struct sigaction my_sa;
    memset(&my_sa, 0, sizeof(my_sa));
    my_sa.sa_handler = &sigchld_handle;
    my_sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    // change default SIGINT
    return sigaction(SIGCHLD, &my_sa, 0);
}

char** is_pipe_process(int count, char** arglist){
	
	for(char** p_arg = arglist; p_arg < &arglist[count]; p_arg++){
		char* pipe_char = "|";
		if(strcmp(*p_arg,pipe_char) == 0){
			*p_arg = NULL;
			return ++p_arg;//returns pointer to the second command, we know ++p_arg is not NULL
		}
	}
	return NULL;
}
int regular_process_arglist(char** arglist){
	pid_t pid = fork();

	if(pid > 0){
		int status;
		waitpid(pid, &status, 0);
	}
	else if(pid == 0){
		signal(SIGINT, SIG_DFL); 
		execvp(*arglist, arglist);
		perror(*arglist);
		exit(1);
	}
	else{
		fprintf(stderr, "regular_process_arglist: fail fork\n");
		exit(1);
	}
	return 1;
}

int pipe_process_arglists(char** first_arglist, char** second_arglist){
	pid_t pid1,pid2;
	int fd[2];

	if (pipe(fd) == -1) {
		fprintf(stderr,"pipe failed\n");
		exit(1);
	}
	pid1 = fork(); //first fork

	if(pid1 == 0){//first command
		signal(SIGINT ,SIG_DFL);
		while ((dup2(fd[1], STDOUT_FILENO) == -1) && (errno == EINTR)) {}
		close(fd[1]);
		close(fd[0]);
		execvp(*first_arglist, first_arglist);
		perror(*first_arglist);
		exit(1);
	}
	else if(pid1 > 0){//father
		close(fd[1]);

		pid2 = fork();//second fork

		if(pid2 == 0){//second command
			signal(SIGINT, SIG_DFL);
			while ((dup2(fd[0], STDIN_FILENO) == -1) && (errno == EINTR)) {}
			close(fd[0]);
			execvp(*second_arglist, second_arglist);
			perror(*second_arglist);
			exit(1);
		}
		else if(pid2 >0 ){//father
			int status;
			close(fd[0]);
			
			//waiting for all children
			waitpid(pid1, &status, 0);
            waitpid(pid2, &status, 0);
		}
		else{
			fprintf(stderr,"pipe_process_arglist: fail second fork\n");
			exit(1);
		}
	}
	else{
		fprintf(stderr,"pipe_process_arglist: fail first fork\n");
		exit(1);
	}
	return 1;
}

int backround_process_arglist(char** arglist){
	pid_t pid = fork();
	if(pid == 0){
		execvp(*arglist, arglist);
		perror(*arglist);
		exit(1);
	}
	else if(pid < 0){
		fprintf(stderr,"backround_process_arglist: fail fork\n");
		exit(1);	
	}
	return 1;	
}
// arglist - a list of char* arguments (words) provided by the user
// it contains count+1 items, where the last item (arglist[count]) and *only* the last is NULL
// RETURNS - 1 if should continue, 0 otherwise
int process_arglist(int count, char** arglist){
	char* br_char = "&";
	if( strcmp(arglist[count-1], br_char) == 0 ){//backround process
		arglist[count-1] = NULL;
		backround_process_arglist(arglist);
	}
	else{
		char** second_command = is_pipe_process(count,arglist);
		if(second_command != NULL){//is a pipe command
			pipe_process_arglists(arglist,second_command);//the "|" turned to NULL in the is_pipe_process function
		}
		else{//regular command
			regular_process_arglist(arglist);
		}
	}
	return 1;	
}

// prepare and finalize calls for initialization and destruction of anything required
int prepare(void){
	

	if (signal_handle() == -1){
        fprintf( stderr, "signal_handle failed\n");
        return 1;
    }
    signal(SIGINT, SIG_IGN);
    return 0;
}
int finalize(void){
	return 0;
}

