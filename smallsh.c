//Garret Sweetwood
//Date: 08/04/17
//smallsh.c
//CS 344
// 


#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <math.h>



int numLen(unsigned x);
//void fgMode(int signo);

int main() {

	char* temp = NULL;
	char* tempNum = NULL;	
	char* iFile = NULL;
	char* oFile = NULL;
	char* line = NULL;
	char* args[513];
	int numArgs;
	char* token;
	int fd;
	int background;
	const char delim[3] = " \n";
	int status = 0;
	int  pid;
	int end = 0;
	int i;
	//int fg = 0;

											//signal handler to ignore SIGINT
	struct sigaction act;
	act.sa_handler = SIG_IGN; 				//set to ignore
	sigaction(SIGINT, &act, NULL);
	
	/*struct sigaction catchZ = {0};
	catchZ.sa_handler = fgMode;
	sigfillset(&catchZ.sa_mask);
	catchZ.sa_flags = SA_RESTHAND;*/
	
	while (!end) {
		background = 0;						//standard will be foreground

			   								//This is the standard prompt
		printf(": ");
		fflush(stdout);

											//Get the next line from stdin, store in line 
		ssize_t size = 0;
		if (!(getline(&line, &size, stdin))) {
			return 0; 					
		}

									
		numArgs = 0;						//this will keep track of number or arguments separated by a space
		token = strtok(line, delim); 		//get initial token, tokens are segments of the string, separated by a delimeter
											//in this case it is a space					
		while (token != NULL) {
			if (strcmp(token, "<") == 0) {	//compare each token to check for special characters <, >, &
				token = strtok(NULL, delim);
				iFile = strdup(token);			 
				token = strtok(NULL, delim);
			}
			else if (strcmp(token, ">") == 0) {
				token = strtok(NULL, delim);
				if (strstr(token, "$$") != NULL){
					char* temp;
					pid_t addPid = getpid();
					char* tempNum;
					tempNum = calloc(numLen(addPid), sizeof(char));
					sprintf(tempNum, "%d", addPid);
					temp = calloc((strlen(token) + numLen(addPid)) , sizeof(char));
					strcpy(temp, token);
					strcat(temp, tempNum);
					oFile = temp;
					token = strtok(NULL, delim);
				}
				else					
				oFile = strdup(token); 
				token = strtok(NULL, delim);
			}
			else if (strcmp(token, "&") == 0) {
				background = 1;				 //set background 
				break;
			}
			else {
											//when not a special character, store the token
				args[numArgs] = strdup(token);
				token = strtok(NULL, delim);
				numArgs++; 					//keep track of number of arguments
			}
		}

											//last element will be null
		args[numArgs] = NULL;

											//CHECK THE FIRST ARGUMENT FOR BUILT-INS OR COMMENT
											
		if (args[0] == NULL || !(strncmp(args[0], "#", 1))) {
											//if empty or #, do nothing
		}
		else if (strcmp(args[0], "exit") == 0) { 				
			exit(0);
			end = 1;
		}
		else if (strcmp(args[0], "status") == 0) {	//Print status
			if (WIFEXITED(status)) {				//print exit status
				printf("Exit status: %d\n", WEXITSTATUS(status));
				fflush(stdout);

			}
			else { 							//else print terminating signal
				printf("Terminating signal %d\n", status);
				fflush(stdout);

			}
		}
		else if (strcmp(args[0], "cd") == 0) { 	//cd - change directory command
											//If there is not arg for cd command
			if (args[1] == NULL) {
											//If no argument, go to HOME, environment variable
				chdir(getenv("HOME"));
			}
			else {								//Else cd to the directory passed
				chdir(args[1]);
			}
		}
		else { 									//NON-BUILT-INS
			   									//fork a child for non-buil-ins
			pid = fork();

			if (pid == 0) { 					//THIS IS THE CHILD 
				if (!background) { 				//If process is foreground
						   						//use the signal handler to no allow interuptions
					act.sa_handler = SIG_DFL; 			
					act.sa_flags = 0;
					sigaction(SIGINT, &act, NULL);
				}

				if (iFile != NULL) { 				//If a file is named
									 				//open that file as read only
					fd = open(iFile, O_RDONLY);

					if (fd == -1) {
													//File not found
						fprintf(stderr, "Invalid file: %s\n", iFile);
						fflush(stdout);
						exit(1);
					}
					else if (dup2(fd, 0) == -1) { 		//redirect input 
											
						fprintf(stderr, "dup2 error");
						exit(1);
					}

													//close the file stream
					close(fd);
				}
				else if (background) {
													//WHEN BACKGROUND IS SET

					fd = open("/dev/null", O_RDONLY);		

					if (fd == -1) {
													//if error
						fprintf(stderr, "Error, file open error");
						exit(1);
					}
					else if (dup2(fd, 0) == -1) { 	//redirect input (0 for stdin)
													//Error redirecting
						fprintf(stderr, "Error, cannot redirect");
						exit(1);
					}
												
					close(fd);
				}
				else  if (oFile != NULL) { 			//If file to output to given
										   	
					fd = open(oFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);	//create one if not

					if (fd == -1) {
						fprintf(stderr, "Error, invalid file: %s\n", oFile);
						fflush(stdout);
						exit(1);
					}

					if (dup2(fd, 1) == -1) { 		//Redirect output (1 for stdout)
													//Error redirecting
						fprintf(stderr, "Error, cannot redirect");
						exit(1);
					}

					close(fd);
				}

													//execute command stored in arg[0]
				if (execvp(args[0], args)) { 	
					fprintf(stderr, "Unknown command : %s\n", args[0]);  	//Exit if command not found
					fflush(stdout);
					exit(1);
				}
			}
			else if (pid < 0) { 	
				fprintf(stderr, "Error, fork error");
				status = 1;
				fflush(stdout);
				break;
			}
			else {									//THIS IS THE PARENT
				if (!background) { 					//if in foreground
					do {
						waitpid(pid, &status, 0);	//wait for child to terminate or end if in foreground
					} while (!WIFEXITED(status) && !WIFSIGNALED(status));
				}
				else {
												//print pid if process is in background
					printf("background pid: %d\n", pid);
				}
			}
		}

												//Start cleanup for the loop
		for (i = 0; i < numArgs; i++) {
			args[i] = NULL;
		}

		iFile = NULL;
		oFile = NULL;
		temp = NULL;
		tempNum = NULL;
		pid = waitpid(-1, &status, WNOHANG);	//now check for children terminated
		while (pid > 0) {							
			printf("background pid complete: %d\n", pid);

			if (WIFEXITED(status)) {			//If it exited, give the status 
				printf("Exit status: %d\n", WEXITSTATUS(status));
			}
			else { 
				printf("Terminating signal: %d\n", status);		//Otherwise it was terminated by a signal, give the status
			}

			pid = waitpid(-1, &status, WNOHANG);
		}
	}

	return 0;
}

int numLen(unsigned x) {
	if(x>=100000) return 6;
	if(x>=10000) return 5;
	if(x>=1000) return 4;
	if(x>=100) return 3;
	if(x>=10) return 2;
	return 1;
}

//void fgMode(int signo){
//	fg = 1;
//}
