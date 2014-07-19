all: c_proto
CC = gcc
INCLUDE = .
CFLAGS = -g -Wall -O2

c_proto: main.o list.o connect.o irc_getopt.o channel.o user.o command.o
	$(CC) -lpthread -o c_proto main.o list.o connect.o irc_getopt.o channel.o user.o command.o
main.o: main.c
	$(CC) -lpthread -I$(INCLUDE) $(CFLAGS) -c main.c
list.o: list.c
	$(CC) -lpthread -I$(INCLUDE) $(CFLAGS) -c list.c
irc_getopt.o: irc_getopt.c
	$(CC) -lpthread -I$(INCLUDE) $(CFLAGS) -c irc_getopt.c 
user.o: user.c
	$(CC) -lpthread -I$(INCLUDE) $(CFLAGS) -c user.c
channel.o: channel.c
	$(CC) -lpthread -I$(INCLUDE) $(CFLAGS) -c channel.c
connect.o: connect.c
	$(CC) -lpthread -I$(INCLUDE) $(CFLAGS) -c connect.c
command.o: command.c
	$(CC) -lpthread -I$(INCLUDE) $(CFLAGS) -c command.c

clean:
	rm main.o list.o connect.o irc_getopt.o channel.o user.o command.o
