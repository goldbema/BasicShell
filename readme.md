# Basic UNIX Shell

This shell supports:
* ``cd``, ``status``, and ``exit`` as built-in commands.
* Non-built-in commands, through the use of ``fork()`` and ``execvp()``.
* Input and output redirection.
* Execution of commands as background processes.

## Compilation and Execution

To compile the shell, type ``make``. To begin executing the shell, type ``main``. Upon successful execution, the shell command line character ``:`` will appear on a new line.

## Command Line Syntax

The general syntax for a shell command is:

`command [argument_1 argument_2 ...] [< in_file] [> out_file] [&]`

* ``in_file`` is the name of the file to which standard input will be redirected.
* ``out_file`` is the name of the file to which standard output will be redirected.
* ``&`` is used to set the command as a background process.

## Built-In Usage

* ``cd`` takes zero or one other argument. If no argument is specified, the working directory is changed to the user home directory. Otherwise, the shell attempts to change the working directory to the absolute or relative path specified by the user.
* ``exit`` takes no other arguments.  Its execution will cause the shell to kill all other shell processes followed by the termination of the shell, itself.
* ``status`` takes no other arguments. It outputs the exit status/terminating signal of the last process that was executed in the foreground. If no foreground process has been executed, an error message is output.

## Cleaning Up

To remove the executable and object files from the working directory, type ``make clean``.

© Maxwell Goldberg 2017
