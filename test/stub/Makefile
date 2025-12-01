CC = gcc
DEBUG_FLAGS = -g3 -ggdb
INCLUDE_FLAGS = -I$(CURDIR)/../../inc
LIB_DIR = $(CURDIR)/../../stub/src/.libs
LDFLAGS =  -L$(LIB_DIR) -lsai -Wl,-rpath=$(LIB_DIR)
EXEC_ALL = lag_ut

all: lag_ut

lag_ut: lag_ut.o
	$(CC) $^ -o $@ $(LDFLAGS)
  
lag_ut.o: lag_ut.c
	$(CC) $(INCLUDE_FLAGS) -c $^ $(DEBUG_FLAGS) -o $@

clean:
	rm -f $(EXEC_ALL) *.o
