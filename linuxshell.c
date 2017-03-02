/*
 * Author(s): Kaylee Yuhas
 * Date: 9 Dec 2016
 * CIS 340 Systems Programming
 * Description: This project creates a shell environment that recognizing the internal
 * commands: cd, path, and quit. External commands are searching for in the directories
 * specified by the user. It also allows for input/output redirection and pipelining.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "command.h"
#include "paths.h"
#include <signal.h>	

#define MAX_SIZE	1024

/*
 * This method is used for debugging.
 */
void printCommandArray(command **ca, int nums) {
	int j;
	for(j = 0; j < nums; j++) {
		int i;
		for(i = 0; i < ca[j]->argc; i++) {
			printf("ca[%d]->argv[%d]: %s\n", j, i, ca[j]->argv[i]);
		}
	}
}

/*
 * This method creates an array of commands that can hold up to 512 commands. If "<" is
 * typed, the next word is the input file and it does not count as an argument for that 
 * command (i.e. the input file name is not added to the argv array. A similar thing is 
 * done if ">" is typed, but for the output file instead.
 */
command **parseCommand(char *userInput, int *numCommands) {
	command **commandArray;
	commandArray = (command **)malloc(sizeof(command *) * MAX_SIZE/2);
	int cNum = 0;
	
	//initialize the first value in the array
	commandArray[cNum] = (command *)malloc(sizeof(command));
	commandArray[cNum]->argv = (char **)malloc(sizeof(char *) * MAX_SIZE/2);
	commandArray[cNum]->name = NULL;
	commandArray[cNum]->input = NULL;
	commandArray[cNum]->output = NULL;
	
	int output = 0; int input = 0;
	char *outputFile = NULL; char *inputFile = NULL;
	int i = 0;
	//char *ptr = strtok(userInput, " \t\n");
	char *ptr = strtok(userInput, " ");
	while(ptr != NULL && i < MAX_SIZE/2) {
		if(strcmp(ptr, ">") == 0) {
			//they want to do output redirection
			output = 1;
		}
		else if(strcmp(ptr, "<") == 0) {
			//they want to do input redirection
			input = 1;
		}
		else if(strcmp(ptr, "|") == 0) {
			//they want to do a pipeline
			
			//finish up the previous command
			commandArray[cNum]->name = (char *)malloc(sizeof(char) * strlen(commandArray[cNum]->argv[0]));
			strcpy(commandArray[cNum]->name, commandArray[cNum]->argv[0]);
			commandArray[cNum]->argv[i] = NULL;
			commandArray[cNum]->argc = i;
			
			cNum++;
			
			//create a new command
			commandArray[cNum] = (command *)malloc(sizeof(command));
			commandArray[cNum]->argv = (char **)malloc(sizeof(char *) * MAX_SIZE/2);
			commandArray[cNum]->name = NULL;
			commandArray[cNum]->input = NULL;
			commandArray[cNum]->output = NULL;
			
			i = 0; //the argument numbers for the next command's argv should start at 0
		}
		else {
			//it is not an output or input redirection SYMBOL or pipeline
			if(output) {
				//this word is the name of the output file
				outputFile = (char *)malloc(sizeof(char) * strlen(ptr));
				strcpy(outputFile, ptr);
				output = 0;
			}
			else if(input) {
				//this word is the name of the input file
				inputFile = (char *)malloc(sizeof(char) * strlen(ptr));
				strcpy(inputFile, ptr);
				input = 0;
			}
			else {
				//not input/output redirection -- new argument/word instead
				commandArray[cNum]->argv[i] = (char *)malloc(sizeof(char) * strlen(ptr));
				strcpy(commandArray[cNum]->argv[i], ptr);
				i++;
			}
		}
		//go to the next word in the file
		//ptr = strtok(NULL, " \t\n");
		ptr = strtok(NULL, " ");
	}
	
	if(ptr == NULL && i == 0) {
		//the user did not type anything
		printf("Nothing typed. Try again.\n");
		commandArray[cNum]->name = "null";
		commandArray[cNum]->argv[i] = "null";
		commandArray[cNum]->argc = 1;
		*numCommands = cNum;
		return commandArray;
	}
	
	if(outputFile != NULL) {
		commandArray[cNum]->output = (char *)malloc(sizeof(char) * strlen(outputFile));
		strcpy(commandArray[cNum]->output, outputFile);
	}
	if(inputFile != NULL) {
		commandArray[0]->input = (char *)malloc(sizeof(char) * strlen(inputFile));
		strcpy(commandArray[0]->input, inputFile);
	}
	
	commandArray[cNum]->name = (char *)malloc(sizeof(char) * strlen(commandArray[cNum]->argv[0]));
	strcpy(commandArray[cNum]->name, commandArray[cNum]->argv[0]);
	commandArray[cNum]->argv[i] = (char *)malloc(sizeof(char));
	commandArray[cNum]->argv[i] = NULL;
	commandArray[cNum]->argc = i;
	
	cNum++;
	*numCommands = cNum;
	
	return commandArray;
}

/* 
 * This method frees all of the dynamically-allocated memory associated with 
 * a command data type. It goes through each command in the command array.
*/
void freeCommand(command **cArray, int numCommands) {
	int j;
	for(j = 0; j < numCommands; j++) {
		int i;
		for(i = 0; i < cArray[j]->argc; i++) {
			cArray[j]->argv[i] = NULL;
			free(cArray[j]->argv[i]);
		}
		free(cArray[j]->argv);
		free(cArray[j]->name);
		if(cArray[j]->input != NULL) {
			free(cArray[j]->input);
		}
		if(cArray[j]->output != NULL) {
			free(cArray[j]->output);
		}
	}
}

/* 
 * This method frees all of the dynamically-allocated memory associated with 
 * a paths data type. First, the dirs[i] values are all freed. 
 * Then, the rest of the dynamically allocated data members are freed.
 * Note: numDirs does not need to be freed because we did not use malloc to create it.
*/
void freePaths(paths *path) {
	int i;
	for(i = 0; i < path->numDirs; i++) {
		path->dirs[i] = NULL;
		free(path->dirs[i]);
	}	
	free(path->dirs);
	free(path->name);
}

/*
 * This method starts off by freeing the previous path name to "reset" its value.
 * Then, it finds the length of the new path name by looking through the dirs array.
 * Once found, a new path name is created and set to its new value.
 */
void getPathName(paths *p) {
	p->name = NULL;
	free(p->name); //free what was there before

	int i; 
	int length = 0;
	for(i = 0; i < p->numDirs; i++) {
		if(i != 0) {
			length++; // for the colon you need to add!
		}
		length = length + strlen(p->dirs[i]);
	}
	p->name = (char *)malloc(sizeof(char)*(length + 1));
	strcpy(p->name, "");
	for(i = 0; i < p->numDirs; i++) {
		if(i > 0) {
			strcat(p->name, ":");
		}
		strcat(p->name, p->dirs[i]);
	}
}

/* 
 * This method tests to see if the string the user wants to append will fit. 
 * The path array "dirs" will only fit 1024 strings.
 * If it can fit, it is added to the array and the path name is updated accordingly.
*/
void addPathName(paths *p, char *add) {
	if(p->numDirs == MAX_SIZE) {
		printf("Can't add that path because the path name is full.\n");
		return;
	}
	p->dirs[p->numDirs] = (char *)malloc(sizeof(char) * strlen(add + 1));
	strcpy(p->dirs[p->numDirs], add);
	p->numDirs = p->numDirs + 1;
	getPathName(p);
	printf("%s successfully added to the path name!\n", add);
}

/* 
 * This method searches through the path's dirs array to look for the directory specified 
 * by the user. If found, the index of the value is returned. If not, -1 is returned.
*/
int findLocation(paths *p, char *string) {
	int i; int location = -1;
	for(i = 0; i < p->numDirs; i++) {
		if(strcmp(p->dirs[i], string) == 0) {
			//we found the entry!
			location = i;
			break;
		}
	}
	return location;
}

/* 
 * This method frees the element at index "location" in the dirs array. 
 * It then shifts everything else in the array to the left one.
*/
void removeElement(paths *p, int location) {
	if(location != p->numDirs - 1) {
		//it is NOT the last element in the dirs array
		int i;
		for(i = location; i < p->numDirs - 1; i++) {
			free(p->dirs[i]);
			unsigned long nextEntryLength = strlen(p->dirs[i+1]);
			p->dirs[i] = (char *)malloc(sizeof(char) * nextEntryLength);
			strcpy(p->dirs[i], p->dirs[i+1]);
		}
		free(p->dirs[i]); // free the last element of the dirs array
	}
	else {
		//it IS the last element
		free(p->dirs[location]);
	}
}

/* 
 * This method checks to see if there is anything to remove from the path array.
 * If it is possible to remove something, it checks to see if that thing is present in 
 * the array. If it is, it will be removed and the pathname will be updated. If it is not
 * in the array, an error message will be returned and the paths data will be unchanged.
*/
void removePathName(paths *p, char *remove) {
	if(p->numDirs == 0) {
		printf("Can't remove that path because the path name is empty.\n");
		return;
	}
	int location = findLocation(p, remove);
	if(location == -1) {
		printf("That directory is not included in the path name. Try again.\n");
		return;
	}
	removeElement(p, location);
	p->numDirs = p->numDirs - 1;
	getPathName(p);
	printf("%s successfully removed from the path name!\n", remove);	
}

/*
 * This method prints out a "$ " to the screen, similar to the bash shell.
 */
void printOutput() {
	char buffer[2] = "$ ";
	write(1, buffer, 2);
}

/*
 * This method takes in a command and paths data type and finds the length of the new
 * string that will hold the file' name. It will append a slash and the command name 
 * (e.g. "/ls" to the end of each entry in the dirs array. If it is found, it will 
 * execute that file. If not, it will print an error message.
 */
void executeCommand(command *c, paths *p) {
	unsigned long commandLength = strlen(c->name);
	unsigned long pathLength, i;
	char *fileName;
	for(i = 0; i < p->numDirs; i++) {
		pathLength = commandLength + strlen(p->dirs[i]) + 10; // 1 is for slash and '\0'
		fileName = (char *)malloc(sizeof(char) * pathLength);
		strcpy(fileName, p->dirs[i]);
		strcat(fileName, "/");
		strcat(fileName, c->name);
		execv(fileName, c->argv);
	}
}

/*
 * This method redirects the input or output for a command. If isOutput == 0, it is input redirection.
 * If isOutput == 1, it is output redirection.
 */
int redirectIO(char *fileName, int isOutput) {
	int fd;
	if(isOutput == 0) {
		//input redirection
		fd = open(fileName, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
		if(fd == -1) {
			printf("Input file not found. Cannot execute command. Try again.\n");
			return 0;
		}
		close(0); //close standard input
		dup(fd);
	}
	if(isOutput == 1) {
		//output redirection
		fd = open(fileName, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
		close(1); // close standard output
		dup(fd);
	}
	return 1;
}

/*
 * This method executes an external command if there is only one command (no pipelining).
 */
int externalCommand(command ** cArray, paths *path, int nums) {
	pid_t pid;
	int status;
	pid = fork();
		
	if(pid == -1) {
		printf("An error occurred. Cannot run that command.\n");
		return 1;
	}
	else if(pid == 0) {
		//child process
		if(path->numDirs > 0) {
			//check to see if there was input or output redirection
			if(cArray[0]->input != NULL) {
				int re = redirectIO(cArray[0]->input, 0);
				if(re == 0) {
					//there was in error with input redirection -- exit process
					exit(0);
				}
			}
			if(cArray[0]->output != NULL) {
				redirectIO(cArray[0]->output, 1);
			}	
			executeCommand(cArray[0], path); 
			printf("Did not find the file.\n");
		}
		else {
			//there are not directories to search through	
			printf("Cannot find file. No directories to search.\n");
		}
		_exit(0);
	}
	else {	
		//good parent -- waits for child to finish executing
		wait(&status);
		return 1;
	}
}

void catchSignal(int signum) {
	printf("Cannot run this command. Try again.\n");
	_exit(0);
}

/*
 * This method creates a pseudo-parent that then creates the appropriate number of pipes and children.
 * The children will each run their own command. The pseudo-parent waits for all its children to return
 * before it exits.
 */
void pipelining(command **cArray, paths *path, int nums) {
	pid_t pid;
	int status;
	pid = fork();
	
	if(pid == -1) {
		printf("An error occurred. Try a different command.\n");
		return;
	}
	
	else if(pid == 0) {
		//this is the pseudo-parent for all of the child processes that involve pipes

		//this process catches these signals.
		signal(SIGINT, catchSignal);

		int numPipes = nums - 1;
		int filedes[numPipes*2]; //need a read/write end for each pipe
		
		//create all of the pipes
		int i;
		for(i = 0; i < numPipes; i++) {
			pipe(filedes + 2*i);
		}

		//now let's check to see if there will be an input error
		if(cArray[0]->input != NULL) {
			int re = redirectIO(cArray[0]->input, 0);
			if(re == 0) {
				//there was an input redirection error
				exit(0);
			}
		}
		if(cArray[numPipes]->output != NULL) {
			int re = redirectIO(cArray[numPipes]->output, 1);
		}
		
		//now, branch off all of the children -- the number of children is 
		//equal to the number of commands
		for(i = 0; i < nums; i++) {
			usleep(100000); //THIS IS MERELY TO MAKE THE PROCESS WAIT LONG ENOUGH TO CATCH ANY SIGNALS IF THEY ARE THROWN.
			if(fork() == 0) {
				//this is a child process
				
				/* FIRST CHILD */
				if(i == 0) {
					close(1); //close standard output
					dup(filedes[1]); 
				}
				
				/* LAST CHILD */
				else if(i == (nums - 1)) {
					int inputFiledes = 2*(i-1);
					close(0); //close standard input
					dup(filedes[inputFiledes]);
				}
				
				/* NOT FIRST OR LAST CHILD */
				else {
					//redirect input
					int inputFiledes = 2*(i-1);
					close(0);
					dup(filedes[inputFiledes]);
					
					//redirect output
					int outputFiledes = 2*i + 1;
					close(1);
					dup(filedes[outputFiledes]);
				}
				
				int j;
				for(j = 0; j < 2*numPipes; j++) {
					close(filedes[j]);
				}	
				executeCommand(cArray[i], path);
				//send an interrupt signal to the parent
				kill(getppid(), SIGINT);
				_exit(0);
			}
			
		}
		//end of for loop making children
		
		//parent should close all of the file descriptors pointing to the pipes and wait
		//for children to finish
		int j;
		for(j = 0; j < 2*numPipes; j++) {
			close(filedes[j]);
		}
		
		int children;
		for(children = 0; children < nums; children++) {
			wait(&status);
		}
		
		//close this pseudo-parent process
		_exit(0);
	}
	else {
		//this is the actual shell parent -- do not exit this one
		wait(&status);
		return;
	}
}


/*
 * This method takes in a command array, number of commands, and paths data type and determines
 * what command it should execute. If there is only one command, it checks if it is an internal command 
 * and if not, checks for input and output redirection. If there is more than one command, it 
 * creates the appropriate number of pipes and then does the pipelining, also checking for input or 
 * output redirection.
 */
int determineCommands(command **cArray, paths *path, int nums) {
	if(nums == 1) {
		//the user only typed in one command
		
		/*					quit					*/
		if(strcmp(cArray[0]->name, "quit") == 0 && cArray[0]->argc == 1) {
			int bool = 0;
			return bool;
		}
		
		/*					path					*/
		else if(strcmp(cArray[0]->name, "path") == 0) {
			if(cArray[0]->argc == 1) {
				//they just typed in path
				if(path->name != NULL) {
					getPathName(path);
					printf("The current pathname is: %s\n", path->name);
				}
				else {
					printf("The current pathname is empty.\n");
				}
			}
			else if(cArray[0]->argc == 3) {
				//they want to either add or remove a path name
				if(strcmp(cArray[0]->argv[1], "+") == 0) {
					addPathName(path, cArray[0]->argv[2]);
				}
				else if(strcmp(cArray[0]->argv[1], "-") == 0) {
					removePathName(path, cArray[0]->argv[2]);
				}
				else {
					//they entered something other than + or - as second input
					printf("Invalid format\n");
				}
			}
			else {
				//they entered an incorrect amount of commands
				printf("Incorrect format.\n");
			}
			return 1;
		}
		
		/*					cd					*/
		else if(strcmp(cArray[0]->name, "cd") == 0 && cArray[0]->argc == 2) {
			if(cArray[0]->argc == 2) {
				int dir = chdir(cArray[0]->argv[1]);
				if(dir == -1) {
					printf("Could not change directories.\n");
				}
				else if(dir == 0) {
					printf("Successfully changed to %s directory.\n", cArray[0]->argv[1]);
				}
			}
			else {
				//they entered an incorrect amount of commands
				printf("Incorrect format.\n");
			}
			return 1;
		}
		
		/*					not internal command					*/
		else {
			externalCommand(cArray, path, nums);
			return 1;
		}
	}
	
	else {
		//there is more than one command -- the user used a pipeline!
		pipelining(cArray, path, nums);
		return 1;
	}
}

/*
 * This is a method used for debugging.
 */
void printIO(command **ca, int nums) {
	int j;
	for(j = 0; j < nums; j++) {
		if(ca[j]->input != NULL)
			printf("ca[%d]->input: %s\n", j, ca[j]->input);
		if(ca[j]->output != NULL)
			printf("ca[%d]->output: %s\n", j, ca[j]->output);
	}	
}


/*
 * The main method of this program creates a paths data type first, which will hold
 * the pathname and an array of all the directories that the shell will search through
 * for any commands that are not internal commands. The structure can hold up to 512 
 * directories. 
 * Then, it creates an array of commands, which it uses to determine what the user wants to do.
 */
int main() {	
	paths *path;
	path = (paths *)malloc(sizeof(paths));
	path->name = NULL;
	path->numDirs = 0;
	path->dirs = (char **)malloc(sizeof(char *) * (MAX_SIZE/2));
	//can hold max_size/2 values

	command **commandArray = (command **)malloc(sizeof(command *) * MAX_SIZE/2);
	
	char *userInput; 
	int boolean = 1;
	while(boolean) {
		printOutput();
		
		//can hold up to 1024 characters
		userInput = (char *)malloc(sizeof(char) * MAX_SIZE); 
		ssize_t sizeofInput = read(0, (void *)userInput, MAX_SIZE);
		userInput[sizeofInput - 1] = '\0';
		
		int commandsNumber = 0;
		int *numCommands = &commandsNumber;
		
		commandArray = parseCommand(userInput, numCommands);
		
		if(*numCommands != 0) {
			//the user typed something in
			
			//printCommandArray(commandArray, *numCommands);
			//printIO(commandArray, *numCommands);
			
			boolean = determineCommands(commandArray, path, *numCommands);
			freeCommand(commandArray, *numCommands);
			free(commandArray);
		}
	
		else {
			//the user did not type anything in
			free(commandArray[0]);
			free(commandArray);
		}
		
		free(userInput);
	}
	freePaths(path);
	free(path);
	return 0;
}


