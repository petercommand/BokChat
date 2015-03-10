CFLAGS  = -Wall -O2 -pthread --std=gnu11
LDFLAGS = -lpthread

SRCS = main.c list.c connect.c bokchat_getopt.c channel.c user.c command.c
OBJS = $(SRCS:%.c=%.o)
BIN = bokchat
%.o: %.c
	$(CC) $(CFLAGS) -c $<

all: $(BIN)


bokchat: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(BIN) $(OBJS)
