/*******************************************************************************
*      Filename: main.c
*        Author: Maxwell Goldberg
* Last Modified: 03.03.17
*   Description: The main shell function.
*******************************************************************************/

#include "builtins.h"
#include "input.h"
#include "signal_proc.h"

/* Command line output string */
#define CL_PROMPT ":"

/* Global foreground-only mode flag switch. */
int FOREGROUND_FLAG = 0;

/*******************************************************************************
*    Function: main()
*  Parameters: Main function parameters.
* Description: Performs initialization actions and the main shell loop.
*     Returns: Exit status.
*******************************************************************************/

int main(int argc, char * argv[]) {
    int exitFlag = 0;
    char inputBuffer[INPUT_BUFFER_LEN+1];   
    struct CommandInfo command = {0};
    struct ForegroundStatus fs = {0};
    struct BackgroundProcesses bp = {0};

    /* Register signal handlers */
    registerParentHandlers();

    /* Initialize ForegroundStatus and Background Processes structs */
    initBackgroundProcesses(&bp);
    initForegroundStatus(&fs);

    while (!exitFlag) {
        /* Reap background processes immediately prior to user input. */
        backgroundCleanup(&bp);

        /* Display prompt and fflush */
        printf("%s ", CL_PROMPT);
        fflush(stdout);

        /* Take in user input */
        memset(inputBuffer, '\0', sizeof(inputBuffer));
        fgets(inputBuffer, INPUT_BUFFER_LEN+1, stdin);

        /* Process user input into command struct */
        processInput(inputBuffer, &command);
        /* If the foreground-only mode flag is set, override whatever
         * foreground status is set so that the command is in the
         * foreground.
         */
        if (FOREGROUND_FLAG) {
            command.isForeground = 1;
        }        


        /* Case: No arguments */
        if (command.numArgs <= 0) {
            /* Do nothing... */
        /* Case: Comment string */
        } else if (command.args[0]->value[0] == '#') {
            /* Do nothing... */
        /* Case: Builtin function call */
        } else if (isBuiltIn(command.args[0]->value)) {
            handleBuiltIn(&command, &fs, &bp);
            if (command.numArgs > 0 && strcmp(command.args[0]->value,
                                              "exit") == 0) {
                exitFlag = 1;
            }
        /* Case: Non-builtin function call */
        } else {
            handleNonBuiltIn(&command, &fs, &bp);
        }

        /* Free allocated memory for the next loop. */
        freeCommandInfoArgs(&command);
    }
    
    return 0;
}
