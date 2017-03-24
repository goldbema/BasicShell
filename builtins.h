/*******************************************************************************
*      Filename: builtins.h
*        Author: Maxwell Goldberg
* Last Modified: 03.03.17
*   Description: The header file for builtins.c. See builtins.c for function
*                descriptions.
*******************************************************************************/

#ifndef BUILTINS_H
#define BUILTINS_H

#include <limits.h>

#include "input.h"
#include "signal_proc.h"

/* Initializer list of builtin function names */
#define BUILTINS_LIST_INIT {"cd", "exit", "status"}
/* The number of builtin functions */
#define NUM_BUILTINS       3

int isBuiltIn(char *);
void handleBuiltIn(struct CommandInfo *, struct ForegroundStatus *,
                   struct BackgroundProcesses *);
void executeCd(struct CommandInfo *);
void executeStatus(struct ForegroundStatus *);
void executeExit(struct BackgroundProcesses *);

#endif
