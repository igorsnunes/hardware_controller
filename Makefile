TEST_SRC = tests/hwController_tests.c
TEST_OBJ = $(TEST_SRC:.c=.o)
CC = gcc
CFLAGS = -g -Wall
LFLAGS = 
INCLUDES = -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
LIBS = -lglib-2.0 -lgthread-2.0 -lsllp -lpcidriver
HWC_SRC = hwController.c 
RWDMA_SRC = readwriteDMA.c

RWDMA_HEAD = $(RWDMA_SRC:.c=.h)

HWC_OBJ = $(HWC_SRC:.c=.o)
RWDMA_OBJ = $(RWDMA_SRC:.c=.o)

MAIN = hwController

.PHONY: clean test
all: $(MAIN)

$(MAIN): $(HWC_OBJ) $(RWDMA_OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(HWC_OBJ) $(RWDMA_OBJ) $(LIBS)

$(HWC_OBJ): $(HWC_SRC) $(RWDMA_HEAD)
	$(CC) -c $(CFLAGS) $(INCLUDES) $(HWC_SRC)

$(RWDMA_OBJ): $(RWDMA_SRC) $(RWDMA_HEAD)
	$(CC) -c  $(RWDMA_SRC) -lpcidriver

test: $(TEST_SRC) all
	$(CC) -Wall -g $(INCLUDES) -L/usr/local/lib -o $(TEST_OBJ) $(TEST_SRC) $(RWDMA_OBJ) -lcunit -lglib-2.0 -lpcidriver
	
clean:
	rm $(HWC_OBJ) $(RWDMA_OBJ) $(MAIN) 
	

