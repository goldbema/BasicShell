/******************************************************************************
*      Filename: input.c
*        Author: Maxwell Goldberg
* Last Modified: 03.03.17
*   Description: Contains methods for processing a user command line string
*                and cleaning up memory allocated to the CommandLine struct.
*******************************************************************************/

#include "input.h"

/*******************************************************************************
*    Function: _expandVars()
*  Parameters: char *inBuffer - The user input string.
*             char *outBuffer - The input string with expansions performed.
* Description: Performs variable expansion on '$$' instances within the user
*              input string.
*     Returns: None.
*******************************************************************************/

void _expandVars(char *inBuffer, char *outBuffer) {
    int i, j;
    int current = 0;
    int dollarFlag = 0;
    pid_t pid = getpid(); /* Determine the process ID. */
    /* Write the process ID to string */
    char pidBuffer[PID_LEN+1];
    memset(pidBuffer, '\0', sizeof(pidBuffer));
    sprintf(pidBuffer, "%i", pid);   

    /* Iterate through the input String. */
    for (i = 0; i <= strlen(inBuffer); i++) {
        /* If the dollar flag isn't set and we encounter a dollar sign,
         * write the dollar sign to the outbuffer and increment the
         * outbuffer index. We don't want to overwrite single dollar sign
         * characters, so we need to remain agnostic about the presence
         * of a second dollar sign until the next loop iteration. We
         * set the dollar flag so that we know that a dollar sign was
         * previously encountered.
         */
        if (!dollarFlag && inBuffer[i] == '$') {
            outBuffer[current] = inBuffer[i];
            current++;
            dollarFlag = 1;
        /* If variable expansion is to occur, we set the outbuffer index
         * back one character in order to overwrite the previously written
         * dollar sign, and output the processID.
         */
        } else if (dollarFlag && inBuffer[i] == '$') {
            current--;
            for (j = 0; j < strlen(pidBuffer); j++) {
                outBuffer[current] = pidBuffer[j];
                current++;
            }
            dollarFlag = 0; /* Reset the dollar flag */
        /* Copy the inbuffer character to the outbuffer. */
        } else {
            outBuffer[current] = inBuffer[i];
            current++;
        }
    } 
}

/*******************************************************************************
*    Function: _addArgument()
*  Parameters: char *argBuffer - The string to be copied to the argument value.
*              int arPos - The position of the arg in the CommandInfo args arr.
*              int argLen - The length of the string (w/o null terminator).
*              struct CommandInfo *ci - A pointer to the CommandInfo struct.
* Description: Allocates memory for an individual Argument struct.
*     Returns: None.
*******************************************************************************/

void _addArgument(char *argBuffer, int argPos, int argLen, 
                  struct CommandInfo *ci) {
    /* Allocate the Argument struct. */
    ci->args[argPos] = malloc(sizeof(struct Argument));
    /* Allocate and fill the Argument value. */
    ci->args[argPos]->value = malloc((sizeof(char) * argLen) + 1);
    memset(ci->args[argPos]->value, '\0', (sizeof(char) * argLen) + 1);
    strcpy(ci->args[argPos]->value, argBuffer);
    /* Set each Argument active by default */
    ci->args[argPos]->isActive = 1;
}

/*******************************************************************************
*    Function: void _freeArgument()
*  Parameters: struct Argument * arg - A pointer to the argument to be 
*                                      deallocated.
* Description: Frees memory allocated for an Argument.
*     Returns: None.
*******************************************************************************/

void _freeArgument(struct Argument *arg) {
    if(arg) {
        /* Free the value. */
        if (arg->value) {
            free(arg->value);
            arg->value = NULL;
        }
        /* Free the argument itself */
        free(arg);
    }
}

/*******************************************************************************
*    Function: void freeCommandInfoArgs()
*  Parameters: struct CommandInfo *ci - A pointer the CommandInfo struct.
* Description: Deallocates all Arguments in the CommandInfo struct.
*     Returns: None.
*******************************************************************************/

void freeCommandInfoArgs(struct CommandInfo *ci) {
    int i;
    if (ci) {
        /* Iterate through all arguments, deallocating them. */
        for (i = 0; i < ci->numArgs; i++) {
            if (ci->args[i]) {
                _freeArgument(ci->args[i]);
                ci->args[i] = NULL;
            }
        }
        /* Reset the number of arguments. */
        ci->numArgs = 0;
    }
}

/*******************************************************************************
*    Function: _processBuffer()
*  Parameters: char *inputBuffer - The user input line with var expansions.
*              struct CommandInfo *ci - The CommandInfo struct.
* Description: Processes the expanded user input into CommandInfo Arguments.
*     Returns: None.
*******************************************************************************/

void _processBuffer(char *inputBuffer, struct CommandInfo *ci) {
    int numArgs = 0;
    int numChars = 0;
    int currArgLen = 0;
    char *c = inputBuffer;
    char argBuffer[INPUT_BUFFER_LEN+1];
    memset(argBuffer, '\0', sizeof(argBuffer));

    /* While we haven't processed the full string, the maximum number of
     * Arguments hasn't been exceeded, and we haven't hit a newline or 
     * null terminator...
     */
    while (numChars <= strlen(inputBuffer) && numArgs < MAX_ARGS &&
           *c != '\0' && *c != '\n') {

        /* Place nonspace chars into the argument string buffer. */          
        if (!isspace(*c)) {
            argBuffer[currArgLen++] = *c;
        /* If we hit a space char and the previous character was
         * nonspace, add an argument to the CommandInfo.
         */
        } else if (numChars > 0 && !isspace(*(c - 1))) {
            _addArgument(argBuffer, numArgs, currArgLen, ci);
            currArgLen = 0;
            numArgs++;
            memset(argBuffer, '\0', sizeof(argBuffer));
        }
        numChars++;
        c++;
    }

    /* Determine if there is a final argument. If so, add it to the
     * CommandInfo. 
     */
    if (numChars > 0 && !isspace(inputBuffer[numChars - 1])
        && numArgs < MAX_ARGS) {
        _addArgument(argBuffer, numArgs, currArgLen, ci);
        numArgs++;
    }
    /* Set the number of arguments. */
    ci->numArgs = numArgs;
}

/*******************************************************************************
*    Function: _determineForeground()
*  Parameters: struct CommandInfo *ci - A pointer to the CommandInfo struct.
* Description: Determines the foreground state of a CommandInfo struct.
*     Returns: None.
*******************************************************************************/

void _determineForeground(struct CommandInfo *ci) {
    int boolean;
    if (ci->numArgs > 0) {
        /* If the last argument is '&', set the Argument as inactive so that it
         * can be cleaned up.
         */
        boolean = (strcmp(ci->args[ci->numArgs-1]->value, "&") != 0);
        if (!boolean) {
            ci->args[ci->numArgs-1]->isActive = 0;
        }
        /* Set the foreground status appropriately */
        ci->isForeground = boolean;
    }
}

/*******************************************************************************
*    Function: _determineRedirects()
*  Parameters: struct CommandInfo *ci - A pointer to the CommandInfo struct.
* Description: Determines the input and output redirects of a CommandInfo struct
*     Returns: None.
*******************************************************************************/

void _determineRedirects(struct CommandInfo *ci) {
    int i;
    memset(ci->inRedirFile, '\0', sizeof(ci->inRedirFile));
    memset(ci->outRedirFile, '\0', sizeof(ci->outRedirFile));

    /* Iterate through the arguments array. */
    for (i = 0; i < ci->numArgs; i++) {
        /* If we encounter an input redirect, copy the next argument to the
         * input redirect buffer */
        if (strcmp(ci->args[i]->value, "<") == 0) {
            ci->args[i]->isActive = 0;
            if (i < (ci->numArgs - 1)) {
                strcpy(ci->inRedirFile, ci->args[i+1]->value);
                ci->args[i+1]->isActive = 0;
            /* If there isn't a subsequent argument, print an error. */
            } else {
                fprintf(stderr, "Warning: Input redir doesn't specify file\n");
                fflush(stderr);
            }
        }
        /* If we encounter an output redirect, copy the next argument to the
         * output redirect buffer */
        if (strcmp(ci->args[i]->value, ">") == 0) {
            ci->args[i]->isActive = 0;
            if (i < (ci->numArgs - 1)) {
                strcpy(ci->outRedirFile, ci->args[i+1]->value);
                ci->args[i+1]->isActive = 0;
            /* If there isn't a subsequent argument, print an error. */
            } else {
                fprintf(stderr, "Warning: Output redir doesn't specify file\n");
                fflush(stderr);
            }
        }
    } 
}

/*******************************************************************************
*    Function: _filterInactiveArgs()
*  Parameters: struct CommandInfo *ci - A pointer to the CommandInfo struct.
* Description: Filters out inactive arguments from the Arguments array.
*     Returns: None.
*******************************************************************************/

void _filterInactiveArgs(struct CommandInfo *ci) {
    int i;
    int newNumArgs = 0;
    struct Argument *newArgs[MAX_ARGS];

    /* Iterate through the Arguments array. */
    for (i = 0; i < ci->numArgs; i++) {
        /* If the argument is active, add it to the new arguments list */
        if (ci->args[i]->isActive) {
            newArgs[newNumArgs] = ci->args[i];
            newNumArgs++;
        }        
        /* Otherwise, free the argument. */
        else {
            _freeArgument(ci->args[i]);
            ci->args[i] = NULL;
        }
    } 
    /* Set the new number of arguments. */
    ci->numArgs = newNumArgs;

    /* Replace the arguments array with the new arguments array. */
    for (i = 0; i < ci->numArgs; i++) {
        ci->args[i] = newArgs[i];
    }
}

/*******************************************************************************
*    Function: processInput()
*  Parameters: char *inputBuffer - The user command line input.
*              struct CommandInfo *ci - A pointer to the CommandInfo struct.
* Description: Performs all input processing tasks necessary to convert a 
*              user input string into a CommandInfo struct.
*     Returns: None.
*******************************************************************************/


void processInput(char *inputBuffer, struct CommandInfo *ci) {
    char tempBuffer[(INPUT_BUFFER_LEN * 3)+1];

    /* Expand "$$" instances into process IDs. */
    _expandVars(inputBuffer, tempBuffer);
    /* Store arguments into the CommandInfo array. */
    _processBuffer(tempBuffer, ci);
    /* Determine the foreground status of the command. */
    _determineForeground(ci);
    /* Determine input and output redirects. */
    _determineRedirects(ci);
    /* Filter out inactive arguments. */
    _filterInactiveArgs(ci);
}
