CC = gcc
CFLAGS = -g
LFLAGS = 
INCLUDES = -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
LIBS = -lglib-2.0 -lgthread-2.0 -lsllp -lpcidriver
SRCS = hwController.c readwriteDMA.c
OBJS = $(SRCS:.c=.o)

MAIN = hwController

.PHONY: clean
all: $(MAIN)

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

$(OBJS): $(SRCS)
	$(CC) -C $(SRCS)

clean:
	rm $(OBJS) $(MAIN) 
	

