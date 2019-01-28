#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <sys/wait.h>
#include "ListChar.h"
#include "MessageInfo.h"

struct ListChar directories;
int *fdIn, *fdOut, *pids, w;
void sighandlerCHLD(int sig);

int canWrite;

int main(int argc, char** argv)
{
	char* docfile = NULL;
	w = 2;

	if (argc != 5 && argc != 3)
	{
		printf("Wrong syntax. Correct arguments are:\n");
		printf("%s -d docfile [-w numOfWorkers]\n", *argv);
		return 1;
	}

	//Read arguments
	int i;
	for (i = 1; i < argc; )
	{
		if (strcmp(argv[i], "-w") == 0)
		{
			w = atoi(argv[++i]);
			++i;
		}
		else if (strcmp(argv[i], "-d") == 0)
		{
			docfile = argv[++i];
			++i;
		}
		else
		{
			printf("Wrong syntax. Correct arguments are:\n");
			printf("%s -d docfile [-w numOfWorkers]\n", *argv);
			return 1;
		}
	}

	//Check input
	if (w < 1 || !docfile)
	{
		printf("w should be > 0 and docfile should not be empty.\n");
		return 1;
	}

	char line[MSGSIZE], reply[MSGSIZE]; //in and out message buffers

	struct sigaction act;
	act.sa_handler = sighandlerCHLD;
	act.sa_flags = 0;
	sigfillset(&act.sa_mask);
	sigaction(SIGCHLD, &act, NULL);

	mkdir("fifo", 0755);

	//Read directories
	constructLC(&directories);
	FILE *dirs;
	if ((dirs = fopen(docfile, "rt")) == NULL)
	{
		perror("fopen");
		exit(1);
	}
	do
	{
		bzero(line, MSGSIZE);
		fgets(line, MSGSIZE-1, dirs);
		if (strlen(line))
			line[strlen(line)-1] = '\0';
		else
			continue;
		insertLC(&directories, line);
	}while(!feof(dirs));
	fclose(dirs);

	pids = malloc(sizeof(int) *  w);
	fdIn = malloc(sizeof(int) *  w);
	fdOut = malloc(sizeof(int) * w);

	//Create, open named pipes and create w workers
	for (i = 0; i < w; i++)
	{
		char npIn[15], npOut[15];
		composePipeNames(i, npIn, npOut);
		unlink(npIn);  mkfifo(npIn,  0644);
		unlink(npOut); mkfifo(npOut, 0644);
		fdIn[i]  = open(npIn,  O_RDWR | O_NONBLOCK);
		fdOut[i] = open(npOut, O_RDWR | O_NONBLOCK);

		if ((pids[i] = fork()) < 0) { perror("fork"); exit(1); }
		else if (!pids[i])
		{
			//Worker process
			execlp("./Worker", "Worker", npOut, npIn, NULL);
			perror("exec"); exit(1);
		}
	}

	//Send directories to workers
	struct LCNode *cur;
	for (i = 0, cur = directories.start; cur; i++, cur = cur->next)
	{
		bzero(line, MSGSIZE);
		strcpy(line, cur->word);
		writeLine(line, fdOut[i % w]);
	}
	bzero(line, MSGSIZE);
	strcpy(line, "end");
	for (i = 0; i < w; i++)
		writeLine(line, fdOut[i]);

	int loop = 1;
	do
	{
		printf("JobExecutor> "); fflush(stdout);
		readLine(line, 0);
		if (strlen(line) == 0)
			continue;
		char* firstWord = strtok(line, " ");

		if (strcmp(firstWord, "/exit") == 0)
		{
			act.sa_handler = SIG_IGN; //ignore SIGCHLD
			act.sa_flags = 0;
			sigfillset(&act.sa_mask);
			sigaction(SIGCHLD, &act, NULL);

			loop = 0;
			int total = 0;
			line[strlen(firstWord)] = ' ';
			for (i = 0; i < w; i++)
			{
				bzero(reply, MSGSIZE);
				writeLine(line, fdOut[i]);
				readLine(reply, fdIn[i]);
				printf("Reply: %d\n", atoi(reply)); fflush(stdout);
				total += atoi(reply);
			}
			bzero(reply, MSGSIZE);
			printf("All workers found %d strings and exited.\n", total);
		}
		else if (strcmp(firstWord, "/wc") == 0)
		{
			int characters = 0, words = 0, lines = 0;
			line[strlen(firstWord)] = ' ';
			int Tcharacters = 0, Twords = 0, Tlines = 0;
			for (i = 0; i < w; i++)
			{
				bzero(reply, MSGSIZE);
				writeLine(line, fdOut[i]);
				readLine(reply, fdIn[i]);
				sscanf(reply, "%d %d %d", &characters, &words, &lines);
				Tlines += lines;
				Twords += words;
				Tcharacters += characters;
			}
			printf("Bytes: %d, Words: %d, Lines: %d\n", Tcharacters, Twords, Tlines);
		}
		else if (strcmp(firstWord, "/search") == 0)
		{
			line[strlen(firstWord)] = ' ';
			for (i = 0; i < w; i++) //send search command
				writeLine(line, fdOut[i]);

			int normal=0, forced=0;
			for (i = 0; i < w; i++)
			{
				while(1)
				{
					bzero(reply, MSGSIZE);
					readLine(reply, fdIn[i]);
					if (strcmp(reply, "end") == 0)
					{
						normal++;
						break;
					}
					else if (strcmp(reply, "endD") == 0)
					{
						forced++;
						break;
					}
					else
						printf("%s\n", reply);
				}
			}
			printf("%d out of %d workers replied on time\n", normal, normal+forced);
		}
		else if (strcmp(firstWord, "/mincount") == 0)
		{
			line[strlen(firstWord)] = ' ';
			int minimum = INT_MAX, current;
			char minFilename[MSGSIZE], curFilename[MSGSIZE];
			for (i = 0; i < w; i++)
			{
				bzero(reply, MSGSIZE);
				writeLine(line, fdOut[i]);
				readLine(reply, fdIn[i]);
				sscanf(reply, "%d %s", &current, curFilename);
				if (current == 0)
					continue;
				if (current < minimum || (current == minimum && strcmp(curFilename, minFilename) < 0))
				{
					strcpy(minFilename, curFilename);
					minimum = current;
				}
			}
			if (minimum != INT_MAX)
				printf("%d %s\n", minimum, minFilename);
			else
				printf("Word not found\n");
		}
		else if (strcmp(firstWord, "/maxcount") == 0)
		{
			line[strlen(firstWord)] = ' ';
			int maximum = -1, current;
			char maxFilename[MSGSIZE], curFilename[MSGSIZE];
			for (i = 0; i < w; i++)
			{
				bzero(reply, MSGSIZE);
				writeLine(line, fdOut[i]);
				readLine(reply, fdIn[i]);
				sscanf(reply, "%d %s", &current, curFilename);
				if (current == 0)
					continue;
				if (current > maximum || (current == maximum && strcmp(curFilename, maxFilename) < 0))
				{
					strcpy(maxFilename, curFilename);
					maximum = current;
				}
			}
			if (maximum != -1)
				printf("%d %s\n", maximum, maxFilename);
			else
				printf("Word not found\n");
		}
		else
			printf("Wrong command.\n");

	}while(loop == 1);


	//Wait workers to exit and destroy named pipes
	for (i = 0; i < w; i++)
	{
		waitpid(pids[i], NULL, 0);
		char npIn[15], npOut[15];
		composePipeNames(i, npIn, npOut);
		close(fdIn[i]); close(fdOut[i]);
		unlink(npIn);  	unlink(npOut);
		rmdir("fifo");
	}

	//Free memory
	free(pids);
	free(fdIn);
	free(fdOut);
	destructLC(&directories);
	return 0;
}

void sighandlerCHLD(int sig)
{
	int exited = wait(NULL);
	int i;
	char line[MSGSIZE];
	for (i = 0; i < w; i++)
	{
		if (exited == pids[i])
		{
			while(read(fdIn[i], line, MSGSIZE) > 0);
			while(read(fdOut[i], line, MSGSIZE) > 0);

			if ((pids[i] = fork()) < 0) { perror("fork"); exit(1); } //kane neo paidi sti thesi i
			else if (!pids[i])
			{
				//Create Worker process
				char npIn[15], npOut[15];
				composePipeNames(i, npIn, npOut);
				execlp("./Worker", "Worker", npOut, npIn, NULL);
				perror("exec"); exit(1);
			}

			//Send directories
			int j;
			struct LCNode *cur;
			for (j = 0, cur = directories.start; cur; j++, cur = cur->next)
			{
				if (j % w == i)
				{
					bzero(line, MSGSIZE);
					strcpy(line, cur->word);
					writeLine(line, fdOut[i]);
				}
			}
			bzero(line, MSGSIZE);
			strcpy(line, "end");
			writeLine(line, fdOut[i]);
			printf("Worker with pid %d was replaced with worker with pid %d.\n", exited, pids[i]);
			break;
		}
	}
}
