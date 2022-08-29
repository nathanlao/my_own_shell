/*
 * cshell.h
 * 
 * Description: Contain all the helper functions, global variables and struct type
 *              
 *              
 *                 
 * Created on: May 30th, 2022
 * Author: Nathan Lo
 */

#include <stdio.h>


#define COMMAND_LENGTH 1024
#define HISTORY_DEPTH 1000
#define VARIABLE_DEPTH 1024
#define TIME_LENGTH 1024
#define SIGNAL_LENGTH 1024
char logHistory[HISTORY_DEPTH][COMMAND_LENGTH]; // string array for log 
char storeTime[TIME_LENGTH][COMMAND_LENGTH]; // string array for time
char storesignal[SIGNAL_LENGTH][COMMAND_LENGTH]; // string array for return value

static int CMDnum = 0; // number of commands
static int envVarNum = 0; // number of environment variables

int themeFlag = 0; // themeFlag initialize to false(turn off)
int invalidCMD = 0; // flag to check for invalid command

typedef struct {
    char *commandName;
    struct tm* time;
    int returnValue;
} Command;


typedef struct {
	char* name;
	char* value;
} EnvVar;


// Description: Helper funtions parse command line
// Postcondition: check for empty space and recognize the form <command><arg0><arg1>...
//                return the number of total words in the command line
int parseLine(char* line, char** argv) {
    char* ptr; // temp pointer

    // Keep track and count the number of parsed words
    int numOfwords = 0;

    // Slpit the command line with delimiter " ".
	ptr = strtok(line, " ");
	while(ptr != NULL){
		argv[numOfwords] = ptr;
		numOfwords++;
		ptr = strtok(NULL, " ");
	}

    argv[numOfwords] = NULL;
    return numOfwords;
}

// Description: Helper funtions to change theme color by using ANSI color codes
// Postcondition: red() change theme to red, green() change theme to green
//                blue() change theme to blue, reeset() change theme back to original
void red () {
  printf("\033[0;31m");
  themeFlag = 1;
}

void green () {
  printf("\033[0;32m");
  themeFlag = 2;
}

void blue () {
  printf("\033[0;34m");
  themeFlag = 3;
}

void reset () {
  printf("\033[0m");
}

// Description: Helper funtions to check color status
void checkThemeStatus() {
    if (themeFlag == 1) {
        red();
    } else if (themeFlag == 2) {
        green();
    } else if (themeFlag == 3) {
        blue();
    }
}

// Description: Helper funtions to set theme color 
// Postcondition: red, green, blue are supported, otherwise, print a error message and set 
//                it to an invalid command
void setColor(char** args){
    if ((strcmp(args[1], "red") == 0)) {
        invalidCMD = 0;
        red();
    } else if (strcmp(args[1], "green") == 0) {
        invalidCMD = 0;
        green();
    } else if (strcmp(args[1], "blue") == 0) {
        invalidCMD = 0;
        blue();
    } else {
        invalidCMD = 1;
        printf("unsupported theme\n");
    }
}

// Description: Helper functions to add a command to log
// Postcondition: strcpy() command name to logHistory[][], strcpy() real time to storeTime[][]
//                and strcpy() return value to storesignal[][]
void addcmd(Command* cmd) { 
	time_t timer;
	char time_buffer[26];
    char return_value_buffer[256];

    // store log command 
	if (strcpy(logHistory[CMDnum],cmd -> commandName)) {
        
        // store time
		timer = time(NULL);
		cmd->time = localtime(&timer);
		strftime(time_buffer, 26, "%c", cmd->time);
		strcpy(storeTime[CMDnum], time_buffer);

        // store return value
        if (invalidCMD == 1) {
            cmd->returnValue = -1;
        } else {
            cmd ->returnValue = 0;
        }

		sprintf(return_value_buffer, "%d", cmd -> returnValue);
        strcpy(storesignal[CMDnum], return_value_buffer);
		CMDnum++;
	} 
}


// Description: Helper functions to print log
void printLog(Command* cmd){ 

	for (int i = 0; i < CMDnum; i++) {
		if (logHistory[i] != NULL) {

            checkThemeStatus(); 
			printf("%s\n", storeTime[i]);

            checkThemeStatus();
			printf(" %s %s\n",logHistory[i], storesignal[i]);
		}
	}
}

// Description: Helper function to terminates the shell
// Postcondition: print a message and exit cleanly
void exitCMD() {
    checkThemeStatus();
    printf("Bye!\n");
    exit(0);
}