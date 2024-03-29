#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CMDS 10
#define MAX_ARGS_PER_CMD 16

// return the index of redirection symbol in arg list
int findRedirectionIndex(char **args, const int argCount)
{
	for (int i = 0; i < argCount; i++)
	{
		if (strcmp(args[i], ">") == 0)
		{
			return i;
		}
	}
	return -1;
}

// print error message to user
void printError()
{
		char error_message[30] = "An error has occurred\n";
		write(STDERR_FILENO, error_message, strlen(error_message));
		fflush(stdout);
}

int main(int argc, char **argv)
{
		// global variables
	size_t cmdSize = 0;
	// keep track of number of arguments passed to each command
	int argCount = 0;
	// keep track of number of paths
	int pathCount = 1;
	// index of start each command seperated by &
	int argIdx = 0;
	char *cmd = NULL;
	char *arg = NULL;
	int argCounts[MAX_CMDS];
	char **path = (char **)malloc(sizeof(char *));
	path[0] = "/bin";

	// create 2d array of strings to store commands and args from user input
	char *(*arguments)[MAX_ARGS_PER_CMD] = malloc(sizeof(*arguments) * MAX_CMDS);

		// calling rush with arguments causes an error
	if (argc > 1)
	{
		printError();
		exit(1);
	}

	// main loop
	while (1)
	{
		// prompt user for input and get that input and store in buffer
		printf("rush> ");
		fflush(stdout);

		// get line of text and store in cmd
		if (getline(&cmd, &cmdSize, stdin) == -1)
		{
			printError();
			exit(1);
		}

		// parse the cmd into tokens and store each token in array
		char *temp = cmd;
		argCount = 0;
		argIdx = 0;
		while ((arg = strsep(&temp, " \n\t")) != NULL)
		{
			if (strlen(arg) > 0)
			{
				// parallel commands move next commands into new array based on argIdx
				if (arg[0] == '&')
				{
					argIdx++;
					argCount = 0;
					continue;
				}
				arguments[argIdx][argCount] = strdup(arg);
				argCount++;
				argCounts[argIdx] = argCount;
			}
		}

		// ensure that arguments were passed
		if (argCount > 0)
		{
			// check if command is built in first
			if (strcmp(arguments[0][0], "exit") == 0)
			{
				if (argCount > 1)
				{
					printError();
				}
				else
				{
					exit(0);
				}
			}
			else if (strcmp(arguments[0][0], "cd") == 0)
			{
				if (argCount != 2)
				{
					printError();
				}
				else
				{
					int dir = chdir(arguments[0][1]);
					if (dir == -1)
					{
						printError();
					}
				}
			}
			else if (strcmp(arguments[0][0], "path") == 0)
			{
				path = (char **)realloc(path, argCount * sizeof(char *));
				for (int i = 1; i < argCount; i++)
				{
					path[i - 1] = arguments[0][i];
				}
				pathCount = argCount - 1;
			}
			// attempt to execute all of the commands passed
			else
			{
				// get total number of commands from parsing input
				int numCmds = argIdx + 1;
				pid_t pids[numCmds];

				// iterate through all commands and fork processes
				for (int i = 0; i < numCmds; i++)
				{
					if ((pids[i] = fork()) < 0)
					{
						printError();
						exit(1);
					}
					else if (pids[i] == 0)
					{
						int canAccessFlag = 0;
						// iterate through all paths in path list to find exe
						for (int j = 0; j < pathCount; j++)
						{
							// get path and append /command to path
							char *res = strdup(path[j]);
							strcat(res, "/");
							strcat(res, arguments[i][0]);
							int canAccess = access(res, X_OK);
							// if exe is found, fork process and check if it has redirection
							if (canAccess == 0)
							{
								canAccessFlag = 1;
								pid_t pid = fork();
								// check if fork failed
								if (pid == -1)
								{
									printError();
									break;
								}
								// child process exec command
								else if (pid == 0)
								{
									// determine if there is redirection
									int redirIndex = findRedirectionIndex(arguments[i], argCounts[i]);
									// check that redirection symbol is inputted
									if (redirIndex > 0)
									{
										// ensure that only 1 argument after is inputted
										if (argCounts[i] - (redirIndex + 1) != 1)
										{
											printError();
											exit(1);
										}
										else
										{
											// open or create the output file and redirect stdout
											int fd = open(arguments[i][redirIndex + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
											dup2(fd, STDOUT_FILENO);
											close(fd);
											arguments[i][redirIndex] = NULL;
											arguments[i][redirIndex + 1] = NULL;
										}
									}
									else
									{
										// check that multiple redirection symbols were inputted and report as error
										for (int h = 0; h < argCounts[i]; h++)
										{
											if (arguments[i][h][0] == '>')
											{
												printError();
												exit(1);
											}
										}
									}
									// execute cmd with arguments list
									int error = execv(res, arguments[i]);
									if (error == -1)
									{
										printError();
										exit(1);
									}
								}
								// parent should wait for child to finish and break out of loop
								else
								{
									int status;
									waitpid(pid, &status, 0);
									break;
								}
							}
						}
						if (canAccessFlag == 0)
						{
							printError();
						}
						// important to call exit here so children do not loop again and fork grandchildren
						exit(0);
					}
				}

				// wait for all children to exit
				int status;
				pid_t p;
				while (numCmds > 0)
				{
					p = wait(&status);
					--numCmds;
				}
			}

			//set arguments to NULL to use again in the next iteration
			for (int i = 0; i < argIdx + 1; i++)
			{
				for (int j = 0; j < argCounts[i]; j++)
				{
					arguments[i][j] = NULL;
				}
				argCounts[i] = 0;
			}
		}
	}

	return 0;
}