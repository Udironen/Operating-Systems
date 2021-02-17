This code implements a simple shell program

The shell will support the following operations, which are described in more detail in file shell.pdf:

1. Executing commands. The user enters a command, i.e., a program and its arguments, such as
sleep 10. The shell executes the command and waits until it completes before accepting another
command.

2. Executing commands in the background. The user enters a command followed by &, for
example: sleep 10 &. The shell executes the command but does not wait for its completion
before accepting another command.

3. Single piping. The user enters two commands separated by a pipe symbol (|), for example:
cat foo.txt | grep bar. The shell executes both commands concurrently, piping the standard
output of the first command to the standard input of the second command. The shell waits until
both commands complete before accepting another command.

full details in shell.pdf
