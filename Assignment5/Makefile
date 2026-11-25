#Do not edit the contents of this file.
CC = gcc
CFLAGS = -Werror -Wall -g -std=gnu99
LDFLAGS = -lrt -lpthread
TARGET = banker allocation page_rep
OBJFILES = banker.o allocation.o page_rep.o
Q3_ARGS ?= 10000 10000 10 10 20 10 42

all: $(TARGET)

banker: banker.c
	$(CC) $(CFLAGS) -o banker banker.c $(LDFLAGS)

allocation: allocation.c
	$(CC) $(CFLAGS) -o allocation allocation.c

page_rep: page_rep.c
	$(CC) $(CFLAGS) -o page_rep page_rep.c

runq3: page_rep
	./page_rep $(Q3_ARGS)

runq2: allocation
	./allocation 1048576

runq1: banker
	./banker 10 5 7 8
runq2t: banker
	sh test.sh
runq3t: banker
	sh test_2.sh
clean:
	rm -f $(OBJFILES) $(TARGET)  $(TARGET) *.exe *~ *.out .q3_run1.txt .q3_run2.txt fileX

.PHONY: all runq1 runq2 runq2t runq3 runq3t clean
