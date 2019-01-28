#ifndef MESSAGEINFO_H_
#define MESSAGEINFO_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MSGSIZE 30000

void readLine(char*, int);
void writeLine(char*, int);

void composePipeNames(int id, char *fifoIn, char* fifoOut);

#endif /* MESSAGEINFO_H_ */
