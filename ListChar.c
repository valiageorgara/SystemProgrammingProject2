#include "ListChar.h"


void constructLC(struct ListChar* lc)
{
	lc->end = NULL;
	lc->count = 0;
	lc->start = lc->end;
}

void destructLC(struct ListChar* lc)
{
	while (lc->count != 0)
	{
		//pop first node
		if (lc->count == 1)
		{
			free(lc->end->word);
			free(lc->end);
			constructLC(lc);
		}
		else
		{
			struct LCNode* secondNode = lc->start->next;
			free(lc->start->word);
			free(lc->start);
			lc->start = secondNode;
			lc->count = lc->count - 1;
		}
	}
}

void insertLC(struct ListChar* lc, char* word)
{
	struct LCNode* n = malloc(sizeof(struct LCNode));
	n->marked = false;
	n->next = NULL;
	n->word = malloc(strlen(word) + 1);
	strcpy(n->word, word);

	if (lc->count == 0)
		lc->start = n;
	else
		lc->end->next = n;
	lc->end = n;
	lc->count = lc->count + 1;
}

void printLC(struct ListChar lc)
{
	struct LCNode* n = lc.start;
	while(n)
	{
		if (n->marked == false)
			printf("%s ", n->word);
		else
			printf("|%s| ", n->word);
		n = n->next;
	}
	printf("\n");
}

Bool searchLC(struct ListChar lc, char* word)
{
	struct LCNode* n = lc.start;
	while(n)
		if (strcmp(n->word, word) == 0)
			return true;
		else
			n = n->next;
	return false;
}

void markLC(struct ListChar toBeMarked, struct ListChar words)
{
	struct LCNode* n = toBeMarked.start;
	while(n)
	{
		if (searchLC(words, n->word) == true)
			n->marked = true;
		n = n->next;
	}
}

void unmarkLC(struct ListChar lc)
{
	struct LCNode* n = lc.start;
	while(n)
	{
		n->marked = false;
		n = n->next;
	}
}
