#ifndef LISTPOST_H_
#define LISTPOST_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int canWrite; //metavliti gia to deadline tou search

struct LPNode
{
	struct LPNode* next;
	int freq, lineNumber;
	char* fileName;
};

struct ListPost
{
	struct LPNode *start;
	struct LPNode *end;
	int count;
};

void constructLP(struct ListPost*);
void destructLP(struct ListPost*);
void insertLP(struct ListPost*, int, int, char*);
struct LPNode* searchLP(struct ListPost, int, char*);
void searchAndInsert(struct ListPost*, int, char*);

#endif
