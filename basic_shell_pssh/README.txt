Quoc Thinh Vo

How To Compile & Run
====================
$ make
$ ./pssh


Description
===========
The progam was able to compile and run as a normal shell.
It was able to display the current working directory within the prompt. Run a single command with optional input and output redirection and
Command line arguments were supported.
Run multiple pipelined commands with optional input and output redirection. Naturally, command line arguments to programs were still be 
supported. Implemented the builtin command exit, which will terminate the shell.

Implemented the builtin command 'which'. This command accepts 1 parameter (a program name),
searches the system PATH for the program, and prints its full path to stdout if found (or simply
nothing if it is not found). If a fully qualified path or relative path is supplied to an executable
program, then that path should simply be printed to stdout. If the supplied program name is another
builtin command, your shell should indicate that in a message printed to stdout. 

The execute_tasks function ultilized the read - write structures of the pipe to make the multilple commands possible. And if
there was only one command, it would break out of the for loop and runt the Paser task command as a single command outside of the loop.
The program testing commands were used to test it and it was able to execute the task that were intended to do.
