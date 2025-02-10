# Defined variables
CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=gnu18
LOGIN = hdoll
SUBMITPATH = ~cs537-1/handin/

# all target, runs wsh and wsh-dbg
.PHONY: all
all: wsh wsh-dbg

# wsh target
wsh: wsh.c wsh.h
	$(CC) $(CFLAGS) -O2 -o $@ $^

# wsh-dbg target
wsh-dbg: wsh.c wsh.h
	$(CC) $(CFLAGS) -O2 -ggdb -o $@ $^

# clean: removes binaries, must be done before submission
.PHONY: clean 
clean:
	rm -rf wsh wsh-dbg

# submit
.PHONY: submit
submit: clean
	rm -rf $(SUBMITPATH)$(LOGIN)/p3/
	cp -r ../../p3/ $(SUBMITPATH)$(LOGIN)/
