#include "MessageInfo.h"

void readLine(char* line, int fd)
{
	if (fd == 0)
	{
		fgets(line, MSGSIZE-1, stdin);
		if (strlen(line))
			line[strlen(line)-1] = '\0';
		return;
	}
	int totalRemaining = MSGSIZE, totalReceived = 0;
	while(totalRemaining > 0)
	{
		int bytes = read(fd, line + totalReceived, totalRemaining);
		if (bytes > 0)
		{
			totalRemaining -= bytes;
			totalReceived += bytes;
		}
	}
}

void writeLine(char* line, int fd)
{
	if (fd == 1)
	{
		printf("%s\n", line);
		fflush(stdout);
		return;
	}
	int totalRemaining = MSGSIZE, totalSent = 0;
	while(totalRemaining > 0)
	{
		int bytes = write(fd, line + totalSent, totalRemaining);
		if (bytes > 0)
		{
			totalRemaining -= bytes;
			totalSent += bytes;
		}
	}
}

void composePipeNames(int id, char *fifoIn, char* fifoOut)
{
	sprintf(fifoIn, "fifo/np%d_in",  id);
	sprintf(fifoOut, "fifo/np%d_out", id);
}
