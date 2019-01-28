#ifndef LISTCHAR_H_
#define LISTCHAR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum Bool
{
	false,
	true
}Bool;

struct LCNode
{
	enum Bool marked;
	struct LCNode* next;
	char* word;
};

struct ListChar
{
	struct LCNode *start;
	struct LCNode *end;
	int count;
};

void constructLC(struct ListChar*);
void destructLC(struct ListChar*);
void insertLC(struct ListChar*, char*);
void printLC(struct ListChar);
Bool searchLC(struct ListChar, char*);
void markLC(struct ListChar, struct ListChar);
void unmarkLC(struct ListChar);

#endif
