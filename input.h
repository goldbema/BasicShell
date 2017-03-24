/*******************************************************************************
*      Filename: input.h
*        Author: Maxwell Goldberg
* Last Modified: 03.03.17
*   Description: The header file for input.c. Please see input.c for function 
*                definitions.
*******************************************************************************/

#ifndef INPUT_H
#define INPUT_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* Length of the input buffer in bytes, excluding the null terminator. */
#define INPUT_BUFFER_LEN 2048 
/* Maximum number of arguments in a single line of input. */
#define MAX_ARGS         512
/* Maximum length of a PID string. */
#define PID_LEN          5

/* A struct to hold an argument. The isActive attribute is used to filter out
 * arguments that will not be passed to exec (such as & and input and output
 * redirection filenames.
 */
struct Argument {
    char *value;
    int isActive;
};

/* A struct to hold all information in a line of user input. Once processInput()
 * has completed, this struct holds the arguments to be passed to exec() (or to
 * a builtin), the number of arguments that meet these criteria, the foreground
 * status of the command, and the filenames of input and output redirection 
 * files.
 */
struct CommandInfo {
    struct Argument *args[MAX_ARGS];
    int   numArgs;
    int   isForeground;
    char inRedirFile[INPUT_BUFFER_LEN+1];
    char outRedirFile[INPUT_BUFFER_LEN+1];
};

void processInput(char *, struct CommandInfo *);
void freeCommandInfoArgs(struct CommandInfo *);

#endif
