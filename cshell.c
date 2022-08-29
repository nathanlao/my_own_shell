/*
 * cshell.c
 * 
 * Description: Command-Line interpreter called cshell supporing the environment variable and 
 *              history of executd commands, cshell will support both built-in commands (ls, pwd,
 *              whoami...) and non-built-in commands (exit, log, print, theme). Also providing two
 *              modes: interactive mode and script mode
 *              
 *                 
 * Created on: May 30th, 2022
 * Author: Nathan Lo
 */

#include <stdio.h>
#include <ctype.h> // isspace()
#include <string.h> // strlen()
#include <stdlib.h> // malloc()
#include <unistd.h> // fork()
#include <sys/wait.h> // wait()
#include <time.h>
#include "cshell.h"

#define COMMAND_LINE_LEN 256
#define VARIABLE_LENGTH 1024

int modeFlag = 0; // modeFlag == 0 (interactiveMode) modeFlag == 1 (scriptMode)

char names[100][100]; // string array for environment variables(variable names)
char values[100][100]; // string array for environment variables(values)
EnvVar environmentVariable[VARIABLE_DEPTH];

// Description: cshell execute non-built-in commands
// Postcondition: Use system call fork() to create a child process, child process use execvp()
//                to execute non-built-in commands, also use pipe() and dup2() to control 
//                output of commands
void execArgs(char** arguments) {
    int fds[2]; // read end: 0 and write end: 1
    pid_t pid;
    char buff[1024];
    int exit_status;

    // create pipe
    if (pipe(fds) == - 1) {
        perror("Pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        char error_message[53] = "Unable to fork()\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    memset(buff, 0, sizeof(buff));

    if(pid == 0) {

        dup2 (fds[1], STDOUT_FILENO);
        close(fds[0]); // child process close the read end 
        close(fds[1]); // child process close the write end after finish writing
        if (execvp(arguments[0], arguments) < 0) {

            char cmd_missing_message[53] = "Missing keyword or command, or permission problem.\n";
            write(STDERR_FILENO, cmd_missing_message, strlen(cmd_missing_message)); 
            
            // exit() for scriptMode will cause ininite loop, modeFlag hence needed here
            if (modeFlag == 0) {
                exit(EXIT_FAILURE);
            } 
        }
    } else {
        
        close(fds[1]); // parent close the write end, parent has to read
        int nbytes = read(fds[0], buff, sizeof(buff)); // read n bytes from read end

        checkThemeStatus();

        printf("%.*s", nbytes, buff);

        int status;
        waitpid(pid, &status, 0);
 
        if ( WIFEXITED(status) ) {
            // check child process exit status
            exit_status = WEXITSTATUS(status);       
        }

    }

    if (exit_status == 1) {
        invalidCMD = 1;
    } else if (exit_status == 0) {
        invalidCMD = 0;
    }

}


// Description: cshell execute non-built-in commands and  built-in commands (command handler)
// Postcondition: check for user input, "print" command print the message to console
//                "theme" command changes the theme color of cshell, "log" command 
//                 print the history of commands, if "$" is detected, store environment 
//                 variable to EnvironmentVariable[], duplicated variable's value will be
//                 updated. Execute non-built-in commands if none of the above are detected.
void executeCommand(char** parsedArgs, int numOfwords, Command* cmd) {

        // built in command: exit
        if (strcmp(parsedArgs[0], "exit") == 0) {
            exitCMD();
        // built in command: print
        } else if (strcmp(parsedArgs[0], "print") == 0) {
            invalidCMD = 0;
             for (int i = 1; i < numOfwords; i++) {
                if (strstr(parsedArgs[i], "$")) {
                    for (int j = 0; j < envVarNum; j++) {
                        if (strcmp(environmentVariable[j].name, parsedArgs[i]) == 0) {
                            printf("%s ", environmentVariable[j].value);
                        }
                    }          
                } else {
                    printf("%s ", parsedArgs[i]);
                }
            }
            printf("\n");
            addcmd(cmd);
            // built in command: theme
          }  else if (strcmp(parsedArgs[0], "theme") == 0 && numOfwords == 2) {
            setColor(parsedArgs);
            addcmd(cmd);

            //built in command: log
          } else if (strcmp(parsedArgs[0], "log") == 0) {
                invalidCMD = 0;
                printLog(cmd);
                addcmd(cmd);

            // Set environment Variable
         } else if (strstr(parsedArgs[0], "$")) {
             
                
             if (numOfwords == 1) {
                invalidCMD = 0;
                addcmd(cmd); // add to log

                // Check for "=" 
                char* ptr1 = strtok(parsedArgs[0], "=");
                char* ptr2 = strtok(NULL, "=");

                if (ptr1 != NULL && ptr2 != NULL) {
                    strcpy(names[envVarNum], ptr1);
                    strcpy(values[envVarNum], ptr2);
                } else {
                    char var_missing_message[53] = "Variable value expected\n";
                    write(STDERR_FILENO, var_missing_message, strlen(var_missing_message));
                }
                envVarNum++;
                
                //printf("%s and %s \n", ptr1, ptr2);

                for(int i = 0; i < envVarNum; i++) {
                    if (strcmp(environmentVariable[i].name, ptr1) != 0) {
                            environmentVariable[i].name = names[i];
                            environmentVariable[i].value = values[i];
                    } else {
                            strcpy(environmentVariable[i].name, "");
                            strcpy(environmentVariable[i].value, ptr2);
                    }
                }     
         
             }  else {
                invalidCMD = 1;
                addcmd(cmd); // add to log
                char var_missing_message[53] = "Variable value expected\n";
                write(STDERR_FILENO, var_missing_message, strlen(var_missing_message));
             }
        

            // execute non-built-in command
         } else {
            execArgs(parsedArgs);
            addcmd(cmd);
         }

}

// Description: Interactive mode is supported, print a prompt, take user input
//              and parse the command line, execute the commands
void interactiveMode() {
    char cmdLine[COMMAND_LINE_LEN];
    char* parsedArgument[20];
    Command* cmd = malloc(sizeof(Command));

    // 
    while (1) {
        // Print out a prompt
        printf("cshell$ ");

        // reset the input color
        reset();

        // read command line
        fgets(cmdLine, COMMAND_LINE_LEN, stdin);

       
        // The loop is excuted again if nothing is entered by user
        if((parsedArgument[0] = strtok(cmdLine,"\n")) == NULL) continue;

        if ((strlen(cmdLine) > 0) && (cmdLine[strlen (cmdLine) - 1] == '\n')) { // Remove "\n"
        	cmdLine[strlen (cmdLine) - 1] = '\0';
        }
    
        // parse command line
        int numOfwords = parseLine(cmdLine, parsedArgument);
        cmd -> commandName = parsedArgument[0];

        checkThemeStatus();
     
        executeCommand(parsedArgument, numOfwords, cmd);

     }
     free(cmd);
}


// Description: script mode is supported, readin a file, receive user input(each line of file)
//              and parse the command line, execute the commands
void scriptMode(char filename[256]) {
    FILE* fptr;

    char cmdLine[COMMAND_LINE_LEN];
    char* parsedArgument[20];
    Command* cmd = malloc(sizeof(Command));

    fptr = fopen(filename, "r");
    if (fptr == NULL) {
        printf("Unable to read script file: %s\n", filename);
        exit(1);
    } else {

        // reset the input color
        reset();

        while (fgets(cmdLine, COMMAND_LINE_LEN, fptr) != NULL) {

             // The loop is excuted again if nothing is entered by user
            if((parsedArgument[0] = strtok(cmdLine,"\n")) == NULL) continue;

            if ((strlen(cmdLine) > 0) && (cmdLine[strlen (cmdLine) - 1] == '\n')) { // Remove "\n"
                cmdLine[strlen (cmdLine) - 1] = '\0';
            }
            // parse command line
            int numOfwords = parseLine(cmdLine, parsedArgument);
            cmd -> commandName = parsedArgument[0];

            checkThemeStatus();
            
            executeCommand(parsedArgument, numOfwords, cmd);

        }
    }
    free(cmd);
    fclose(fptr);
    exitCMD();
}


int main(int argc, char *argv[]) {

    // Initialize the array of environmentVariable, avoid segment fault
    for (int i = 0;i < VARIABLE_DEPTH; i++) {
        environmentVariable[i].name = "";
        environmentVariable[i].value = "";
    }

    if (argc == 1) {
        modeFlag = 0;
        interactiveMode();
    } else if (argc == 2) {
        modeFlag = 1;
        scriptMode(argv[1]);
    }

    return 0;
}