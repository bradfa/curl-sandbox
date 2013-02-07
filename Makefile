CFLAGS = -Wextra -Werror -g -std=gnu99
LDFLAGS = -lcurl -pthread

.PHONY: all
all : tester

tester : tester.o

tester.o : tester.c

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf tester
