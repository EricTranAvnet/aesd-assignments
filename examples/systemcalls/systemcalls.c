#include "systemcalls.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
 	int ret = 0;
	// If there is no command, return false
	if (cmd == NULL)
	{
		return false;
	}

	ret = system(cmd);
	// If child process could not be created, return false
	if (ret == -1)
	{
		return false;
	}
	// if process has been created, exited but status of execution command is not good, false
	else if (WIFEXITED(ret) && WEXITSTATUS(ret) != 0)
	{
		return false;
	}

	// then we should be good to return true
    	return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/

	pid_t child_pid;
	int ret ;
	int status;
	int status_child_pid;

	child_pid = fork();
// If child coudn't be created
	if (child_pid == -1)
	{
		return false;
	}
// if we are in the child process
	if (child_pid == 0)
	{
		if (command[0][0] != '/' )
		{
			exit(1);
		}

		ret = execv(command[0],command);

		if (ret == -1)
		{
			exit(1);
		}
	}

	status_child_pid = wait(&status);

// Waiting failed
	if (status_child_pid == -1 )
	{
		return false;
	}

// Waiting ok but executatiion failed
	if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
	{
		return false;
	}

// Child terminated abnormally
	if (!WIFEXITED(status))
	{
		return false;
	}

    va_end(args);

// We should be good at that point
    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/


        int status;
        int status_child_pid;

	int kidpid;
	int fd = open("redirected.txt", O_WRONLY | O_TRUNC| O_CREAT, 0644);

	if (fd < 0)
	{
		return false;
	}

	switch (kidpid = fork())
	{
  		case -1 :
			return false;

	// child
		case 0:
    			if (dup2(fd, 1) < 0)
			{
    				close(fd);
				exit(1);
			}
			if (command[0][0] != '/')
			{
				exit(1);
			}
    			execv(command[0], command);
  			exit(1);

		default:

        		status_child_pid = waitpid(kidpid,&status,0);
			// Waiting failed
			        if (status_child_pid == -1 )
			        {
			                return false;
			        }

			// Waiting ok but executatiion failed
			        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
			        {
			                return false;
			        }

			// Check if child terminated abnormally
				if (!WIFEXITED(status))
				{
					return false;
				}
	}
    va_end(args);

    return true;
}
