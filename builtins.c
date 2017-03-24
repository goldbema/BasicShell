/*******************************************************************************
*      Filename: builtins.c
*        Author: Maxwell Goldberg
* Last Modified: 03.03.17
*   Description: Contains a method for determining if a character string is the
*                name of a builtin command as well as methods for selecting a
*                builtin command and executing each builtin.
*******************************************************************************/

#include "builtins.h"

/*******************************************************************************
*    Function: isBuiltIn()
*  Parameters: char *arg - The string to be evaluated.
* Description: Determines whether or not a character string is the name of a 
*              builtin command.
*     Returns: 1 if the string is the name of a builtin command, 0 otherwise.
*******************************************************************************/

int isBuiltIn(char *arg) {
    /* Form an array of all valid names */
    char *builtins[NUM_BUILTINS] = BUILTINS_LIST_INIT;
    int i;

    /* Iterate through each element of the array. If the argument has the same
     * value, return 1.
     */
    for (i = 0; i < NUM_BUILTINS; i++) {
        if (strcmp(arg, builtins[i]) == 0) {
            return 1;
        }     
    }
    /* Otherwise, return 0 */
    return 0;
}

/*******************************************************************************
*    Function: handleBuiltIn()
*  Parameters: struct CommandInfo *ci - A pointer to the CommandInfo struct.
*              struct ForegroundStatus *fs - The status struct ptr to be used in 
*                                            executeStatus().
*              struct BackgroundProcess *bp - The array of background PIDs.
* Description: Selects a builtin function to execute based on the values of the
*              CommandInfo struct.
*     Returns: None.
*******************************************************************************/

void handleBuiltIn(struct CommandInfo *ci, struct ForegroundStatus *fs,
                   struct BackgroundProcesses *bp) {
    /* Find the name of the builtin command to be executed. Note that the
     * CommandInfo struct will contain a builtin command name as its first
     * argument by virtue of a call to isBuiltIn() within main() and prior
     * checks for no arguments and comments.
     */
    char *commandName = ci->args[0]->value;
   
    /* Find the argument name, and execute its corresponding function. */ 
    if (strcmp(commandName, "cd") == 0) {
        executeCd(ci);
    } else if (strcmp(commandName, "exit") == 0) {
        executeExit(bp);
    } else if (strcmp(commandName, "status") == 0) {
        executeStatus(fs);
    }
}

/*******************************************************************************
*    Function: executeCd()
*  Parameters: struct CommandInfo *ci - A pointer to the CommandInfo struct.
* Description: Executes the cd builtin command.
*     Returns: None.
*******************************************************************************/

void executeCd(struct CommandInfo *ci) {
    int status;
    char pathBuffer[PATH_MAX];
    /* Place the default directory to navigate to in the pathBuffer
     * if the CommandInfo has only one argument. Otherwise, leave
     * the buffer empty (the user has provided either an absolute
     * or relative path of their own).
     */
    memset(pathBuffer, '\0', sizeof(pathBuffer));
    if (ci->numArgs == 1) {
        strcpy(pathBuffer, getenv("HOME"));
        strcat(pathBuffer, "/");
    }

    /* Check for an erroneous number of arguments. */
    if (ci->numArgs > 2) {
        fprintf(stderr, "Warning: More than one arg passed to cd\n");
        fflush(stderr);
    /* If a second arg is provided, add it to the path */
    } else if (ci->numArgs == 2) {
        strncat(pathBuffer, ci->args[1]->value, 
                PATH_MAX - 1 - strlen(ci->args[1]->value));
        pathBuffer[PATH_MAX-1] ='\0';
    }
    /* Attempt to change directories. Display an error if it occurs. */
    if ((status = chdir(pathBuffer)) != 0) {
        perror("chdir");
    }
}

/*******************************************************************************
*    Function: executeStatus()
*  Parameters: struct ForegroundStatus *fs - A pointer to the last foreground
*              process status.
* Description: Executes the status builtin command.
*     Returns: None.
*******************************************************************************/

void executeStatus(struct ForegroundStatus *fs) {
    /* This function generates output based on the values in the ForegroundStatus
     * struct. This means that this struct must have correct values prior to
     * the calling of this function. */
    char outputBuffer[128];
    memset(outputBuffer, '\0', sizeof(outputBuffer));

    /* If the struct contains its initial status number, no non-builtins have
     * been executed, so display this to the user. */
    if (fs->statusNum == -1) {
        fprintf(stderr, 
           "status: No foreground process executed by shell instance\n");
        fflush(stderr);
    /* If the struct doesn't contain a signal, add the exit number to string */
    } else if (!fs->isSignal) {
        sprintf(outputBuffer, "exit value %d", fs->statusNum);
    /* Otherwise, add the terminating signal to the string. */
    } else if (fs->isSignal) {
        sprintf(outputBuffer, "terminated by signal %d", fs->statusNum);
    }
    /* Print the generated string. */
    fprintf(stdout, "%s\n", outputBuffer);
    fflush(stdout);
}

/*******************************************************************************
*    Function: executeExit()
*  Parameters: struct BackgroundProcess *bp - The background PIDs array.
* Description: Kills all background processes, and cleans them up. The caller
*              will handle setting the exit status for the program.
*     Returns: None.
*******************************************************************************/

void executeExit(struct BackgroundProcesses *bp) {
    int i;

    /* Iterate through the entire array. Given that there are no guarantees
     * that certain processes will finish before others, we must check
     * the entire array in order to find and send SIGTERM signals to all
     * extant background processes. 
     */
    for (i = 0; i < NUM_BACKGROUND_PIDS; i++) {
        if (bp->array[i] != -1 && kill(bp->array[i], SIGTERM) != 0) {
            perror("kill");
            /* Clean up terminated processes */
            wait(0);
        }
    }
}
