#include "Trie.h"
#include <signal.h>

//TrieNode functions
void constructTnode(struct Tnode* tn, char value, struct Tnode *prev)
{
	//init list of children
	tn->next = malloc(sizeof(struct ChildList));
	constructChildList(tn->next);
	//init posting list
	tn->pl = malloc(sizeof(struct ListPost));
	constructLP(tn->pl);

	tn->prev = prev;
	tn->value = value;
}

void destructTnode(struct Tnode* tn)
{
	//Free posting list
	destructLP(tn->pl);
	free(tn->pl);
	tn->pl = NULL;

	//recursive call on child nodes and free memory
	struct ChildListNode* current = tn->next->start;
	while(current != NULL)
	{
		destructTnode(current->tnode);
		free(current->tnode);
		current = current->next;
	}

	//Free child node list
	destructChildList(tn->next);
	free(tn->next);
	tn->next = NULL;
}

struct Tnode* childExists(struct Tnode* tn, char value)
{
	return searchChildList(tn->next, value);
}

struct Tnode* childAdd(struct Tnode* tn, char value)
{
	struct Tnode* child = malloc(sizeof(struct Tnode));
	constructTnode(child, value, tn);
	insertChildList(tn->next, child);
	return child;
}

char* getWord(struct Tnode* tn)
{
	//find length of word first
	struct Tnode *temp = tn;
	int wordLength = 0;
	while(temp->value != ' ')
	{
		wordLength += 1;
		temp = temp->prev;
	}

	//Allocate memory for word
	char* word = malloc(wordLength + 1);

	//Go up to root
	temp = tn;
	int i = 0;
	while(temp->value != ' ')
	{
		word[i++] = temp->value;
		temp = temp->prev;
	}
	word[i] = '\0';

	//reverse string
	strrev(word);
	return word;
}

void documentFrequency(struct Tnode* tn)
{
	//if posting list exists on current node, print word and count
	if (tn->pl != NULL && tn->pl->count > 0)
	{
		char *word = getWord(tn);
		if (word != NULL && strlen(word) > 0)
			printf("%s %d\n", word, tn->pl->count);
		free(word);
	}
	//recursive call on child nodes
	struct ChildListNode* current = tn->next->start;
	while(current != NULL)
	{
		documentFrequency(current->tnode);
		current = current->next;
	}
}

//List of children functions
void constructChildListNode(struct ChildListNode* cln, struct Tnode *tn)
{
	cln->tnode = tn;
	cln->next = NULL;
}

void constructChildList(struct ChildList* cl)
{
	cl->start = NULL;
	cl->end = NULL;
	cl->count = 0;
}

void destructChildList(struct ChildList* cl)
{
	while (cl->count != 0)
	{
		//pop first node
		if (cl->count == 1)
		{
			free(cl->end);
			constructChildList(cl);
		}
		else
		{
			struct ChildListNode* secondNode = cl->start->next;
			free(cl->start);
			cl->start = secondNode;
			cl->count = cl->count - 1;
		}
	}
}

void insertChildList(struct ChildList *cl, struct Tnode* n)
{
	struct ChildListNode* cln = malloc(sizeof(struct ChildListNode));
	constructChildListNode(cln, n);

	if (cl->count == 0)
		cl->start = cln;
	else
		cl->end->next = cln;
	cl->end = cln;
	cl->count = cl->count + 1;
}

struct Tnode* searchChildList(struct ChildList* cl, char value)
{
	struct ChildListNode* current = cl->start;
	while(current)
	{
		if (current->tnode->value == value)
			return current->tnode;
		current = current->next;
	}
	return NULL;
}

void strrev(char* str) //reverse string str
{
	size_t len = strlen(str);
	for (size_t i = 0; i < len/2; i+=1)
	{
		//swap i with len-i-1
		char tmp = *(str+i);
		*(str+i) = *(str+len-i-1);
		*(str+len-i-1) = tmp;
	}
}

//Trie functions
void constructTrie(struct Trie* t)
{
	t->root = malloc(sizeof(struct Tnode));
	constructTnode(t->root, ' ', NULL);
	t->characters = t->words = t->lines = 0;
}

void destructTrie(struct Trie* t)
{
	destructTnode(t->root);
	free(t->root);
	t->root = NULL;
}

void insertWordTrie(struct Trie* t, const char* word, int lineNum, char* fileName)
{
	struct Tnode* current = t->root, *tmp;
	size_t i = 0;
	for (i = 0; i < strlen(word); i += 1) //for all characters of word
		//if word[i] exists as child of current
		if ((tmp = childExists(current, word[i])) != NULL)
			current = tmp; //visit child
		//if word[i] does NOT exist
		else //add as child and visit it
			current = childAdd(current, word[i]);

	//word has been added, update or add posting list
	if (current->pl == NULL)
	{
		current->pl = malloc(sizeof(struct ListPost));
		constructLP(current->pl);
	}
	searchAndInsert(current->pl, lineNum, fileName);
	t->words++;
}

struct ListPost* searchWordTrie(struct Trie* t, const char* word)
{
	struct Tnode* current = t->root, *tmp;
	size_t i = 0;
	for (i = 0; i < strlen(word); i += 1) //for all characters of word
		//if word[i] exists as child of current
		if ((tmp = childExists(current, word[i])) != NULL)
			current = tmp; //visit child
		//if word[i] does NOT exist
		else //return NULL
			return NULL;
	return current->pl; //word found, return posting List pointer
}

int df_word(struct Trie *t, const char* word, struct ListChar* paths, int fdOut)
{
	struct ListPost* postingList = searchWordTrie(t, word);
	char line[MSGSIZE]; bzero(line, MSGSIZE);
	if (postingList != NULL && postingList->count > 0)
	{
		sigset_t myalarm;
		sigemptyset(&myalarm);
		sigaddset(&myalarm, SIGALRM);

		struct LPNode *cur;
		for (cur = postingList->start; cur; cur = cur->next) //gia kathe komvo
		{
			/*
			// Test alarm
			if (rand() % 100 < 40)
			{
				printf("Sleeping 3...\n");
				sleep(3);
			}
			*/
			//Write line to fdOut
			bzero(line, MSGSIZE);
			sprintf(line, "%s %d ", cur->fileName, 1 + cur->lineNumber);
			readNthLine(cur->fileName, cur->lineNumber, line + strlen(line), MSGSIZE - strlen(line) - 1);
			if (canWrite == 0)
				return 0;
			sigprocmask(SIG_BLOCK, &myalarm, NULL);
			writeLine(line, fdOut);

			//Check if found path exists to paths
			if (searchLC(*paths, cur->fileName) == false)
				insertLC(paths, cur->fileName);
			sigprocmask(SIG_UNBLOCK, &myalarm, NULL);
		}
		sigprocmask(SIG_BLOCK, &myalarm, NULL);
		return 1;
	}
	return 0;
}

void df(struct Trie *t)
{
	documentFrequency(t->root);
}

void readNthLine(char* filename, int lineNumber, char* line, int size)
{
	int i;
	FILE* f;
	if ((f = fopen(filename, "rt")) == NULL) { perror("fopen"); exit(1); }
	for (i = 0; i < lineNumber+1; ++i)
	{
		bzero(line, size);
		fgets(line, size-1, f);
		if (strlen(line))
			line[strlen(line)-1] = '\0';
	}
	fclose(f);
}
