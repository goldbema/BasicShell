/*******************************************************************************
*      Filename: signal_proc.c
*        Author: Maxwell Goldberg
* Last Modified: 03.03.17
*   Description: Contains functions related to the processing of non-builtin 
*                functions, the initialization of the BackgroundProcesses and
*                ForegroundStatus structs, and the registration of signal 
*                handlers.
*******************************************************************************/

#include "signal_proc.h"

/*******************************************************************************
*    Function: initBackgroundProcesses()
*  Parameters: struct BackgroundProcesses *bp - A pointer to the background PID
*                                               array.
* Description: Initializes a BackgroundProcesses struct.
*     Returns: None.
*******************************************************************************/

void initBackgroundProcesses(struct BackgroundProcesses *bp) {
    int i;
    /* Initialize the array size. */
    bp->size = 0;
   
    /* Initialize all PIDS to -1. We will test against this value to determine
     * if a background process ID is in the array. 
     */ 
    for (i = 0; i < NUM_BACKGROUND_PIDS; i++) {
        bp->array[i] = -1;
    }
}

/*******************************************************************************
*    Function: initForegroundStatus()
*  Parameters: struct ForegroundStatus *fs - A pointer to the foreground status.
* Description: Initializes a ForegroundStatus struct.
*     Returns: None.
*******************************************************************************/

void initForegroundStatus(struct ForegroundStatus *fs) {
    fs->isSignal = 0;
    /* We will test against a statusNum of -1 in executeStatus() to determine
     * if any foreground non-builtin has been executed or not.
     */
    fs->statusNum = -1;
}

/*******************************************************************************
*    Function: _addBackgroundProcess()
*  Parameters: struct BackgroundProcesses *bp - A pointer to the background arr.
*              pid_t childPid - The PID to be added to the array.
* Description: Adds a background process ID to the array.
*     Returns: None.
*******************************************************************************/

void _addBackgroundProcess(struct BackgroundProcesses *bp, pid_t childPid) {
    int i;

    /* If the array is full, exit with an error. */
    if (bp->size >= NUM_BACKGROUND_PIDS) {
        fprintf(stderr, "Warning: %d background processes already running\n",
                NUM_BACKGROUND_PIDS);
        fflush(stderr);
        exit(1);
    }
    /* Otherwise, iterate through the array until we find an empty element. */
    for (i = 0; i < NUM_BACKGROUND_PIDS; i++) {
        /* Add the PID and increase the array size */
        if (bp->array[i] == -1) {
            bp->array[i] = childPid;
            bp->size++;
            return;
        }
    }
    /* If we don't find an empty element at this point, display an error and exit.
     */
    fprintf(stderr, "Warning: unable to place background process in array.\n");
    fflush(stderr);
    exit(1);
}

/*******************************************************************************
*    Function: handleNonBuiltIn()
*  Parameters: struct CommandInfo *ci - A pointer to the CommandInfo struct.
*              struct ForegroundStatus *fs - A pointer to the foreground status.
*              struct BackgroundProcesses *bp - A pointer to the background PID
*                                               array.
* Description: Handles redirection of input and output as well as fork() and 
*              exec() operations for non-builtin commands.
*     Returns: None.
*******************************************************************************/

void handleNonBuiltIn(struct CommandInfo *ci, struct ForegroundStatus *fs,
                      struct BackgroundProcesses *bp) {
    pid_t spawnPid = -5;
    int childExitMethod = -5;
    int i, sourceFD, targetFD, status;
    char *argList[ci->numArgs + 1];
    sigset_t mask;

    /* Signals issued while a parent is waiting can affect the execution of
     * waitpid() and set the ForegroundStatus struct into an undefined 
     * state. We need to shield the parent from the signals below while
     * it is waiting for a foreground process to complete. We will use mask
     * to shield against these signals.
     */
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTSTP);

    /* Set active arguments into the correct state for an execvp() call. */
    for (i = 0; i < ci->numArgs; i++) {
        argList[i] = ci->args[i]->value;
    } 
    /* Append a NULL argument to the list. */
    argList[ci->numArgs] = NULL;

    /* Fork off a child process */
    spawnPid = fork();
   
    /* If an error occurred, display it and exit. */ 
    if (spawnPid == -1) {
        perror("fork");
        exit(1);
    /* If the PID is 0, we are in the child process. */
    } else if (spawnPid == 0) {
        /* Register child signal handlers depending on whether or not the command
         * has been issued in the foreground.
         */
        if (ci->isForeground) {
            registerForegroundChildHandlers();
        } else {
            registerBackgroundChildHandlers();
        }

        /* Determine input redirection. If the command is in the background,
         * and there is no explicitly assigned file, set the input 
         * redirection file to /dev/null.
         */
        if (strlen(ci->inRedirFile) == 0 && !ci->isForeground) {
            strcpy(ci->inRedirFile, "/dev/null");
        }
        /* Perform a similar operation for output redirection. */
        if (strlen(ci->outRedirFile) == 0 && !ci->isForeground) {
            strcpy(ci->outRedirFile, "/dev/null");
        }

        /* Use dup2() to set the input redirection. */
        if (strlen(ci->inRedirFile) != 0) {
            /* If the redirect file can't be opened, exit with an error. */
            if ((sourceFD = open(ci->inRedirFile, O_RDONLY)) == -1) {
                fprintf(stderr, "cannot open %s for input\n", ci->inRedirFile);
                fflush(stderr);
                exit(1);
            }
            if (dup2(sourceFD, 0) == -1) {
                perror("dup2");
                exit(1);
            }
            close(sourceFD);
        }
        /* Use dup2() to set the output redirection. */
        if (strlen(ci->outRedirFile) != 0) {
            /* If the redirect file can't be opened, exit with an error. */
            if ((targetFD = open(ci->outRedirFile, O_WRONLY | O_CREAT | O_TRUNC,
                                 0777)) == -1) {
                fprintf(stderr, "cannot open %s for output\n", ci->outRedirFile);
                fflush(stderr);
                exit(1);
            }
            if (dup2(targetFD, 1) == -1) {
                perror("dup2");
                exit(1);
            }
            close(targetFD);
        }      

        /* Attempt to execvp() on the argument list. If it fails,  exit with 
         * an error.
         */
        if (execvp(argList[0], argList) < 0) {
            perror(argList[0]);
            exit(1);
        }
    }
 
    /* If the command is issued for a foreground process, wait for the 
     * foreground process to terminate. Block out signals that might interfere 
     * with this wait.
     */
    if (ci->isForeground) {
        sigprocmask(SIG_BLOCK, &mask, NULL);
        status = waitpid(spawnPid, &childExitMethod, WSTOPPED);
        sigprocmask(SIG_UNBLOCK, &mask, NULL);

        /* If waitpid() didn't issue an error, inform the ForegroundStatus
         * struct.
         */
        if (status != -1) { 
            informStatus(spawnPid, childExitMethod, fs);
            /* If the child was terminated by signal, display the signal no.*/
            if (WIFSIGNALED(childExitMethod)) {
                executeStatus(fs);
            }
        /* Otherwise, display the error. */
        } else {
            perror("waitpid");
        }
    /* If the command is issued for a background process, print the PID and 
     * add it to the BackgroundProcesses array.
     */
    } else {
        fprintf(stdout, "background pid id %d\n", spawnPid);
        fflush(stdout);
        _addBackgroundProcess(bp, spawnPid);
    }
}

/*******************************************************************************
*    Function: backgroundCleanup()
*  Parameters: struct BackgroundProcesses *bp - A pointer to the background PID
*                                               array.
* Description: Attempts to clean up background processes with nonblocking
*              waitpid().
*     Returns: None.
*******************************************************************************/

void backgroundCleanup(struct BackgroundProcesses * bp) {
    int i, status;
    /* Use a ForegroundProcess struct to get the exit status of background
     * processes that we can clean up.
     */
    struct ForegroundStatus processStat;
    initForegroundStatus(&processStat);

    /* Iterate through the array... */
    for (i = 0; i < NUM_BACKGROUND_PIDS; i++) {
        /* If we find a PID, call nonblocking waitpid(). */
        if (bp->array[i] != -1) {
            /* If it returns with a positive status, we have 
             * terminated a process. In this case, print its exit value.
             */
            if (waitpid(bp->array[i], &status, WNOHANG) > 0) {
                fprintf(stdout, "background pid %d is done: ", bp->array[i]);
                fflush(stdout);
                informStatus(bp->array[i], status, &processStat);
                executeStatus(&processStat);
                bp->array[i] = -1;
            }
        }
    }
}

/*******************************************************************************
*    Function: informStatus()
*  Parameters: pid_t pid - The process ID.
*              int result - The integer containing the wait() status.
*              struct ForegroundStatus *status - A pointer to the status.
* Description: Places values into the status struct that can be used for output
*              by execStatus().
*     Returns: None.
*******************************************************************************/

void informStatus(pid_t pid, int result, struct ForegroundStatus *status) {
    /* If the process terminated by exit, note this in the struct. */
    if (WIFEXITED(result) != 0) {
        status->isSignal = 0;
        status->statusNum = WEXITSTATUS(result);
    /* Otherwise, note its signal termination and signal number in the struct */
    } else if (WIFSIGNALED(result) != 0) {
        status->isSignal = 1;
        status->statusNum = WTERMSIG(result);
    }
}

/*******************************************************************************
*    Function: catchSIGINT()
*  Parameters: int signo - The signal number.
* Description: The signal handler function for the shell process. Its primary
*              purpose is to prevent the shell from terminating on SIGINT.
*     Returns: None.
*******************************************************************************/

void catchSIGINT(int signo) {
    /* Output a newline */
    puts("");
}

/*******************************************************************************
*    Function: catchSIGTSTP()
*  Parameters: int signo - The signal number.
* Description: The SIGTSTP handler function for the shell process. It flips
*              the global FOREGROUND_FLAG switch and displays a message on each
*              received signal.
*     Returns: None.
*******************************************************************************/

void catchSIGTSTP(int signo) {
    puts("");
    if (!FOREGROUND_FLAG) {
        puts("Entering foreground-only mode (& is now ignored)");
        FOREGROUND_FLAG = 1;
    } else {
        puts("Exiting foreground-only mode");
        FOREGROUND_FLAG = 0;
    } 
}

/*******************************************************************************
*    Function: registerParentHandlers()
*  Parameters: None.
* Description: Registers the SIGINT and SIGTSTP signal handlers for the shell
*              process.
*     Returns: None.
*******************************************************************************/

void registerParentHandlers() {
    struct sigaction SIGINT_action = {0}; 
    struct sigaction SIGTSTP_action = {0};

    /* Register the SIGINT handler */
    SIGINT_action.sa_handler = catchSIGINT;
    sigfillset(&(SIGINT_action.sa_mask));
    SIGINT_action.sa_flags = 0;

    sigaction(SIGINT, &SIGINT_action, NULL);

    /* Register the SIGTSTP handler */
    SIGTSTP_action.sa_handler = catchSIGTSTP;
    sigfillset(&(SIGTSTP_action.sa_mask));
    SIGTSTP_action.sa_flags = 0;

    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
}

/*******************************************************************************
*    Function: registerForegroundChildHandlers()
*  Parameters: None.
* Description: Registers the foreground child to ignore SIGTSTP and to perform
*              the default handling of SIGINT.
*     Returns: None.
*******************************************************************************/

void registerForegroundChildHandlers() {
    struct sigaction ignore_action = {0};
    struct sigaction default_action = {0};

    ignore_action.sa_handler = SIG_IGN;
    default_action.sa_handler = SIG_DFL;
   
    sigaction(SIGTSTP, &ignore_action, NULL);
    sigaction(SIGINT, &default_action, NULL);
}

/*******************************************************************************
*    Function: registerBackgroundChildHandlers()
*  Parameters: None.
* Description: Registers a background child to ingnore both SIGINT and SIGTSTP.
*     Returns: None.
*******************************************************************************/

void registerBackgroundChildHandlers() {
    struct sigaction ignore_action = {0};

    ignore_action.sa_handler = SIG_IGN;

    sigaction(SIGINT, &ignore_action, NULL);
    sigaction(SIGTSTP, &ignore_action, NULL);
}
