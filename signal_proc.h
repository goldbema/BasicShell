/*******************************************************************************
*      Filename: signal_proc.h
*        Author: Maxwell Goldberg
* Last Modified: 03.03.17
*   Description: The header file for signal_proc.c. See signal_proc.c for 
*                function descriptions.
*******************************************************************************/

#ifndef SIGNAL_PROC_H
#define SIGNAL_PROC_H

#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "input.h"

/* Maximum number of background processes */
#define NUM_BACKGROUND_PIDS 64
/* Global foreground-only flag declaration. Necessary for SIGTSTP signal handler
 */
extern int FOREGROUND_FLAG;

/* A struct to contain the status of the foreground process. Note that this struct
 * can be used to capture the status of any process, so its name is a candidate
 * for refactoring.
 */
struct ForegroundStatus {
    int statusNum;
    int isSignal;
};

/* A struct to hold background process IDs. When using it to add and wait for
 * processes, the entire array of processes must be iterated through because
 * there isn't a guarantee that a given process will terminate before another.
 * This struct is a candidate for refactoring because a linked list might
 * better represent the concept of the PID list. That is, additions can be
 * made in constant time, individual deletions occur in constant time, and
 * we only need to check the list nodes rather than all elements of an array.
 */
struct BackgroundProcesses {
    pid_t array[NUM_BACKGROUND_PIDS];
    int size;
};

void initBackgroundProcesses(struct BackgroundProcesses *);
void initForegroundStatus(struct ForegroundStatus *);
void handleNonBuiltIn(struct CommandInfo *, struct ForegroundStatus *,
                      struct BackgroundProcesses *);
void backgroundCleanup(struct BackgroundProcesses *);
void informStatus(pid_t, int, struct ForegroundStatus *);

void catchSIGINT(int);
void catchSIGTSTP(int);
void registerParentHandlers();
void registerForegroundChildHandlers();
void registerBackgroundChildHandlers();

/* Forward declaration of the executeStatus() builtin function. */
void executeStatus(struct ForegroundStatus *);

#endif
