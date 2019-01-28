#include "ListPost.h"

void constructLP(struct ListPost* lp)
{
	lp->end = NULL;
	lp->count = 0;
	lp->start = lp->end;
}

void destructLP(struct ListPost* lp)
{
	while (lp->count != 0)
	{
		//pop first node
		if (lp->count == 1) //only 1 node
		{
			free(lp->end->fileName);
			free(lp->end);
			constructLP(lp);
		}
		else //more than 1 nodes
		{
			struct LPNode* secondNode = lp->start->next;
			free(lp->start->fileName);
			free(lp->start);
			lp->start = secondNode;
			lp->count = lp->count - 1;
		}
	}
}

void insertLP(struct ListPost* lp, int freq, int lineNum, char* fileName)
{
	struct LPNode* n = malloc(sizeof(struct LPNode));
	n->lineNumber = lineNum;
	n->next = NULL;
	n->freq = freq;
	n->fileName = malloc(strlen(fileName) + 1);
	strcpy(n->fileName, fileName);

	if (lp->count == 0)
		lp->start = n;
	else
		lp->end->next = n;
	lp->end = n;
	lp->count = lp->count + 1;
}

struct LPNode* searchLP(struct ListPost lp, int lineNum, char* fileName)
{
	struct LPNode* n = lp.start;
	while(n)
		if (n->lineNumber == lineNum && strcmp(fileName, n->fileName) == 0)
			return n;
		else
			n = n->next;
	return NULL;
}

void searchAndInsert(struct ListPost* lp, int lineNum, char* fileName)
{
	struct LPNode* n = searchLP(*lp, lineNum, fileName);
	if (!n)
		insertLP(lp, 1, lineNum, fileName);
	else
		n->freq++;
}

