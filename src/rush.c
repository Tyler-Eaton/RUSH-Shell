#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

// print error message to user
void printError()
{
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
	fflush(stdout);
}

// return the index of redirection symbol in arg list
int findRedirectionIndex(char **args, int argCount)
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

int main(int argc, char **argv)
{
	// buffer for cmd input
	char *cmd = NULL;
	char *arg = NULL;
	size_t cmdSize = 0;
	size_t argCount = 0;
	size_t pathCount = 1;
	char ***arguments = NULL;
	int* argCounts = NULL;
	// allocate space for path and set initial path to /bin
	char **path = (char **)malloc(sizeof(char *));
	path[0] = "/bin";

	// calling rush with arguments causes an error
	if (argc > 1)
	{
		printError();
		exit(1);
	}

	while (1)
	{
		// prompt user for input and get that input and store in buffer
		printf("rush> ");
		if (getline(&cmd, &cmdSize, stdin) == -1)
		{
			printError();
			exit(1);
		}

		// parse input command and store into array of strings
		char *temp = cmd;
		argCount = 0;
		int argIdx = 0;
		while ((arg = strsep(&temp, " \n\t")) != NULL)
		{
			if (strlen(arg) > 0)
			{
				if(arg[0] == '&') {
					argCounts = (int*)realloc(argCounts, (argIdx + 1) * sizeof(int));
					argCounts[argIdx] = argCount;
					argIdx++;
					argCount = 0;
					continue;
				}
				argCount++;
				arguments = (char ***)realloc(arguments, (argIdx + 1) * sizeof(char **));
				arguments[argIdx] = (char**)realloc(arguments[argIdx], argCount * sizeof(char*));
				arguments[argIdx][argCount-1] = (char*)realloc(arguments[argIdx][argCount-1], 50*sizeof(char));
				arguments[argIdx][argCount-1] = strdup(arg);
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
				path = (char **)realloc(path, (argCounts[0] - 1) * sizeof(char *));
				for (int i = 1; i < argCounts[0]; i++)
				{
					path[i - 1] = arguments[0][i];
				}
				pathCount = argCounts[0] - 1;
			}
			else
			{
				int numCmds = argIdx + 1;
				pid_t pids[numCmds];

				for (int i = 0; i < numCmds; i++)
				{
					if ((pids[i] = fork()) < 0)
					{
						printError();
						exit(1);
					}
					else if (pids[i] == 0)
					{
						int flag = 0;
						for (int j = 0; j < pathCount; j++)
						{
							char *res = strdup(path[j]);
							strcat(res, "/");
							strcat(res, arguments[i][0]);
							int canAccess = access(res, X_OK);
							if (canAccess == 0)
							{
								flag = 1;
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
											int fd = open(arguments[i][redirIndex + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644); // Open or create the output file
											dup2(fd, STDOUT_FILENO);													  // Redirect stdout to the file
											close(fd);																	  // Close the file descriptor
											arguments[i][redirIndex] = NULL;
											arguments[i][redirIndex + 1] = NULL;
										}
									}
									else
									{
										for (int h = 0; h < argCounts[i]; h++)
										{
											if (arguments[i][j][0] == '>')
											{
												printError();
												exit(1);
											}
										}
									}
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
						if (flag == 0)
						{
							printError();
						}
						exit(0);
					}
				}

				/* Wait for children to exit. */
				int status;
				pid_t p;
				while (numCmds > 0)
				{
					p = wait(&status);
					--numCmds;
				}
			}
		}

		// clear arguments after process has run
		for(int i = 0; i < argIdx + 1; i++) {
			for (int j = 0; j < argCounts[i]; j++)
				{
					arguments[i][j] = NULL;
				}
		}
		arguments = NULL;
		argCounts = NULL;

	}

	return 0;
}
