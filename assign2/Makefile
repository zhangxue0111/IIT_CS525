CC=gcc
CFLAGS=-I.
DEPS = dberror.h storage_mgr.h buffer_mgr.h dt.h buffer_mgr_stat.h test_helper.h
OBJ = dberror.o storage_mgr.o buffer_mgr.o buffer_mgr_stat.o 

all: test_assign2_1 test_assign2_2

test_assign2_1.o: test_assign2_1.c
	$(CC) -c test_assign2_1.c

test_assign2_2.o: test_assign2_2.c
	$(CC) -c test_assign2_2.c

test_assign2_1: $(OBJ) test_assign2_1.o
	$(CC) -o $@ $^ $(CFLAGS)

test_assign2_2: $(OBJ) test_assign2_2.o
	$(CC) -o $@ $^ $(CFLAGS)

dberror.o: dberror.c dberror.h
	$(CC) -c dberror.c

buffer_mgr.o: buffer_mgr.c buffer_mgr.h dberror.h storage_mgr.h
	$(CC) -c buffer_mgr.c

buffer_mgr_stat.o: buffer_mgr_stat.c buffer_mgr_stat.h buffer_mgr.h
	$(CC) -c buffer_mgr_stat.c

storage_mgr.o: storage_mgr.c storage_mgr.h dberror.h
	$(CC) -c storage_mgr.c

.PHONY : clean
clean :
	$(RM) *.o test_assign2_1 -r
	$(RM) *.o test_assign2_2 -r

