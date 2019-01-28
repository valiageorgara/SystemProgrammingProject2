# In  order  to  execute  this  "Makefile " just  type  "make "
OBJS     = ListChar.o  ListPost.o  MessageInfo.o
SOURCE   = JobExecutor.c  ListChar.c  ListPost.c  MessageInfo.c  Trie.c  Worker.c
HEADER   = ListChar.h  ListPost.h	MessageInfo.h  Trie.h
OUT      = Worker JobExecutor
CC       = gcc
FLAGS    = -g -c
# -g  option  enables  debugging  mode
# -c flag  generates  object  code  for  separate  files
all: Worker JobExecutor

#  create/ compile  the  individual  files  >> separately <<

ListChar.o: ListChar.c $(HEADER)
	$(CC) $(FLAGS) ListChar.c

ListPost.o: ListPost.c $(HEADER)
	$(CC) $(FLAGS) ListPost.c

Worker.o: Worker.c 
	$(CC) $(FLAGS) Worker.c

JobExecutor.o: JobExecutor.c 
	$(CC) $(FLAGS) JobExecutor.c

Trie.o: Trie.c $(HEADER) Worker.o
	$(CC) $(FLAGS) Trie.c

MessageInfo.o: MessageInfo.c $(HEADER)
	$(CC) $(FLAGS) MessageInfo.c

Worker: $(OBJS) Worker.o Trie.o
	$(CC) -o Worker $(OBJS) Worker.o Trie.o

JobExecutor: $(OBJS) JobExecutor.o Trie.o
	$(CC) -o JobExecutor $(OBJS) JobExecutor.o

#  clean  house
clean :
	rm -f $(OBJS) $(OUT) JobExecutor.o Trie.o Worker.o

