all: c_proto
CC = gcc
INCLUDE = .
CFLAGS = -Wall -O2

c_proto: main.o list.o connect.o event.o irc_getopt.o channel.o user.o channel.o
	$(CC) -o c_proto main.o list.o connect.o event.o irc_getopt.o channel.o user.o
main.o: main.c
	$(CC) -I$(INCLUDE) $(CFLAGS) -c main.c
list.o: list.c
	$(CC) -I$(INCLUDE) $(CFLAGS) -c list.c
event.o: event.c
	$(CC) -I$(INCLUDE) $(CFLAGS) -c event.c
irc_getopt.o: irc_getopt.c
	$(CC) -I$(INCLUDE) $(CFLAGS) -c irc_getopt.c
user.o: user.c
	$(CC) -I$(INCLUDE) $(CFLAGS) -c user.c
channel.o: channel.c
	$(CC) -I$(INCLUDE) $(CFLAGS) -c channel.c
connect.o: connect.c
	$(CC) -I$(INCLUDE) $(CFLAGS) -c connect.c


clean:
	rm main.o list.o connect.o event.o irc_getopt.o channel.o user.o
