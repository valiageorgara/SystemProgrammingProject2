#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <time.h>
#include "ListChar.h"
#include "ListPost.h"
#include "Trie.h"
#include "MessageInfo.h"

void readDirectoryFiles(const char*, struct Trie*);
void readTextFile(char*, struct Trie*);
void parseDirectories(struct ListChar, struct Trie*);
void addTimestamp(char*);

int canWrite; //metavliti gia to deadline tou search
int fdOut=1;

void sighandler(int sig)
{
	canWrite = 0;
	char reply[MSGSIZE];
	bzero(reply, MSGSIZE);
	sprintf(reply, "endD");
	writeLine(reply, fdOut);
}

int main(int argc, char **argv)
{
	struct ListChar directories; //list with directories to parse
	constructLC(&directories);
	char line[MSGSIZE], reply[MSGSIZE]; //in and out message buffers

	int fdIn=0;
	struct sigaction act;
	act.sa_handler = sighandler;
	act.sa_flags = 0;
	sigfillset(&act.sa_mask);
	sigaction(SIGALRM, &act, NULL);

	fdIn  = open(argv[1],  O_RDWR | O_NONBLOCK);
	fdOut = open(argv[2],  O_RDWR | O_NONBLOCK);

	//fprintf(stderr, "Created worker with PID %d\n", getpid());
	//Read directories
	do
	{
		bzero(line, MSGSIZE);
		readLine(line, fdIn);

		if (strcmp(line, "end") != 0)
			insertLC(&directories, line);
		else
			break;
	}while(1);

	int loop = 1, stringsFound = 0;
	struct Trie trie;
	constructTrie(&trie);
	parseDirectories(directories, &trie);
	destructLC(&directories);

	//Create log file
	char logFile[100];
	bzero(logFile, MSGSIZE);
	mkdir("log", 0755);
	sprintf(logFile, "log/Worker_%d", getpid());
	FILE* log = fopen(logFile, "wt");
	if (log == NULL) { perror("fopen"); exit(1); }
	fclose(log);

	//fprintf(stderr, "Worker with PID %d is waiting commands\n", getpid());
	do
	{
		//printf("> "); //comment out
		readLine(line, fdIn);
		char* firstWord = strtok(line, " ");

		if (strcmp(firstWord, "/exit") == 0)
		{
			loop = 0;
			bzero(reply, MSGSIZE);
			sprintf(reply, "%d", stringsFound);
			writeLine(reply, fdOut);
		}
		else if (strcmp(firstWord, "/wc") == 0)
		{
			bzero(reply, MSGSIZE);
			sprintf(reply, "%d %d %d", trie.characters, trie.words, trie.lines);
			writeLine(reply, fdOut);
		}
		else if (strcmp(firstWord, "/search") == 0)
		{
			char searchString[MSGSIZE];
			bzero(searchString, MSGSIZE);
			//Parse deadline (if exists)
			int deadline = 0;
			char *tmp;
			if ((tmp = strstr(line + strlen("/search") + 1, " -d ")) != NULL)
				deadline = atoi(tmp + strlen(" -d "));

			alarm(deadline);
			canWrite = 1; //allow normal write
			struct ListChar results;
			constructLC(&results);
			char* word = strtok(NULL, " ");
			for (int wordCounter = 0; wordCounter < 10 && word; wordCounter++)
			{
				if (strcmp(word, "-d") == 0) //stop if -d deadline
					break;
				stringsFound += df_word(&trie, word, &results, fdOut); //Reply
				if (canWrite == 0)
					break;
				strcat(searchString, word);
				strcat(searchString, " "); //meta apo kathe keyword vazw PANTA keno (tha vgalw meta to teleutaio)
				word = strtok(NULL, " ");
			}

			if (canWrite == 0)
				continue;

			bzero(line, MSGSIZE);
			sprintf(line, "end");
			writeLine(line, fdOut);
			alarm(0); //reset alarm
			sigset_t myalarm;
			sigemptyset(&myalarm);
			sigaddset(&myalarm, SIGALRM);
			sigprocmask(SIG_UNBLOCK, &myalarm, NULL);

			//Write to log
			//bgazw to teleutaio keno sto searchString
			searchString[strlen(searchString) - 1] = '\0';
			bzero(line, MSGSIZE);
			addTimestamp(line); //line has timestamp
			bzero(reply, MSGSIZE);
			sprintf(reply, "search:%s", searchString);
			strcat(line, reply); //line has timestamp and "search:searchString
			log = fopen(logFile, "at");
			struct LCNode* cur;
			for (cur = results.start; cur; cur = cur->next)
			{
				strcat(line, ":");
				strcat(line, cur->word);
			}
			fprintf(log, "%s\n", line);
			fclose(log);

			destructLC(&results);
		}
		else if (strcmp(firstWord, "/mincount") == 0)
		{
			char* word = strtok(NULL, " ");
			char searchWord[strlen(word) + 1];
			strcpy(searchWord, word);
			struct ListPost *postingList;
			postingList = searchWordTrie(&trie, word);
			int minimum = trie.words+1, count=0;
			char minimumFilename[MSGSIZE], prev[MSGSIZE];
			bzero(minimumFilename, MSGSIZE); bzero(prev, MSGSIZE);
			if (postingList != NULL)
			{
				struct LPNode *cur;
				for (cur = postingList->start; cur; cur = cur->next) //gia kathe komvo
				{
					if (strcmp(prev, cur->fileName) == 0 || strlen(prev) == 0)
						count = count + cur->freq;

					if ((strcmp(prev, cur->fileName) != 0 && strlen(prev) != 0) || cur->next == NULL)
					{
						if (count < minimum || (count == minimum && strcmp(cur->fileName, minimumFilename) < 0))
						{
							if (strlen(prev) != 0)
								strcpy(minimumFilename, prev);
							else
								strcpy(minimumFilename, cur->fileName);
							minimum = count;
							count = cur->freq;
						}
					}

					strcpy(prev, cur->fileName);
				}
			}
			else minimum = 0;

			//Write to log
			log = fopen(logFile, "at");
			bzero(line, MSGSIZE); bzero(reply, MSGSIZE);
			addTimestamp(line); //line has timestamp
			if (minimum != 0)
				sprintf(reply, "mincount:%s:%d:%s\n", searchWord, minimum, minimumFilename);
			else
				sprintf(reply, "mincount:%s:%d\n", searchWord, minimum);
			strcat(line, reply);
			fprintf(log, "%s", line);
			fclose(log);

			//Reply
			bzero(reply, MSGSIZE);
			sprintf(reply, "%d %s", minimum, minimumFilename);
			writeLine(reply, fdOut);
		}
		else if (strcmp(firstWord, "/maxcount") == 0)
		{
			char* word = strtok(NULL, " ");
			char searchWord[strlen(word) + 1];
			strcpy(searchWord, word);
			struct ListPost *postingList;
			postingList = searchWordTrie(&trie, word);
			int maximum = -1, count=0;
			char maximumFilename[MSGSIZE], prev[MSGSIZE];
			bzero(maximumFilename, MSGSIZE); bzero(prev, MSGSIZE);
			if (postingList != NULL)
			{
				struct LPNode *cur;
				for (cur = postingList->start; cur; cur = cur->next) //gia kathe komvo
				{
					if (strcmp(prev, cur->fileName) == 0 || strlen(prev) == 0)
						count = count + cur->freq;

					if ((strcmp(prev, cur->fileName) != 0 && strlen(prev) != 0) || cur->next == NULL)
					{
						if (count > maximum || (count == maximum && strcmp(cur->fileName, maximumFilename) < 0))
						{
							if (strlen(prev) != 0)
								strcpy(maximumFilename, prev);
							else
								strcpy(maximumFilename, cur->fileName);
							maximum = count;
							count = cur->freq;
						}
					}

					strcpy(prev, cur->fileName);
				}
			}
			else maximum = 0;

			//Write to log
			log = fopen(logFile, "at");
			bzero(line, MSGSIZE); bzero(reply, MSGSIZE);
			addTimestamp(line); //line has timestamp
			if (maximum != 0)
				sprintf(reply, "maxcount:%s:%d:%s\n", searchWord, maximum, maximumFilename);
			else
				sprintf(reply, "maxcount:%s:%d\n", searchWord, maximum);
			strcat(line, reply);
			fprintf(log, "%s", line);
			fclose(log);

			//Reply
			bzero(reply, MSGSIZE);
			sprintf(reply, "%d %s", maximum, maximumFilename);
			writeLine(reply, fdOut);
		}

	}while(loop == 1);

	destructTrie(&trie);
	return 0;
}

void readDirectoryFiles(const char* dirName, struct Trie* trie)
{
	//Kodikas apo tis simeiwseis tou mathimatos (diafaneies)
	DIR *dp;
	if ((dp = opendir(dirName)) == NULL) /* Open directory */
	{
		perror("opendir");
		exit(1);
	}

	struct dirent *dir;
	while ((dir = readdir(dp)) != NULL) /* Read contents till end */
	{
		if (strcmp(dir->d_name, ".") == 0)
			continue;
		if (strcmp(dir->d_name, "..") == 0)
			continue;
		if (dir->d_ino == 0) /* Ignore removed entries */
			continue;
		char* newname = malloc(strlen(dirName)+strlen(dir->d_name)+2);
		strcpy(newname, dirName); /* Compose pathname as "name/dir->d_name" */
		strcat(newname, "/");
		strcat(newname, dir->d_name);
		readTextFile(newname, trie);
		free(newname);
	}
	closedir(dp);
	//Kodikas apo tis simeiwseis tou mathimatos (diafaneies)
}

void readTextFile(char* fileName, struct Trie* trie)
{
	int lineNumber = 0;
	FILE* in = fopen(fileName, "rt");
	if (in == NULL)
	{
		perror("fopen");
		exit(0);
	}

	char line[MSGSIZE];
	do
	{
		//Read a line from input file
		bzero(line, MSGSIZE);
		fgets(line, MSGSIZE-1, in);
		trie->characters += strlen(line);
		if (strlen(line))
			line[strlen(line)-1] = '\0';

		//Parse line to words and add them to trie
		char* word = strtok(line, " ");
		while(word)
		{
			insertWordTrie(trie, word, lineNumber, fileName);
			word = strtok(NULL, " ");
		}

		lineNumber += 1; //line counter increase
	}while(!feof(in));

	trie->lines += (lineNumber-1);
	fclose(in);
}

void parseDirectories(struct ListChar dirs, struct Trie* trie)
{
	//printf("W%d: Parsing %d directories\n", getpid(), dirs.count);
	for (struct LCNode* c = dirs.start; c; c = c->next)
		readDirectoryFiles(c->word, trie);
	//printf("W%d: Parsing completed.\n", getpid());
}

void addTimestamp(char* line)
{
	//Evresi tou timestamp
		// current date/time based on current system
	time_t now = time(NULL);
	struct tm *ltm;
	ltm = localtime(&now);
	ltm->tm_year = ltm->tm_year + 1900;
	char timestamp[50];
	sprintf(timestamp, " %02d:%02d:%02d:", ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
	sprintf(line, "%d-%02d-%02d", ltm->tm_year, ltm->tm_mon, ltm->tm_mday);
	strcat(line, timestamp);
}
