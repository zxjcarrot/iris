#sources
IRIS_SRC = src/base_logger.cpp src/buffered_writer.cpp src/file_writer.cpp src/level_logger.cpp src/notifier.cpp src/stream_writer.cpp src/utils.cpp
IRIS_TEST_SRC = $(IRIS_SRC) tests/test_lfringbuffer.cpp
#object files
IRIS_OBJS = base_logger.o buffered_writer.o file_writer.o level_logger.o notifier.o stream_writer.o utils.o
IRIS_TEST_OBJS = $(IRIS_OBJS) test_lfringbuffer.o
#executable
PROGRAM = libiris.so
#PROGRAM = main
#compiler
CC = g++

CACHELINE_SIZE =
OS := $(shell uname)
ifeq ($(OS), Darwin)
CACHELINE_SIZE = $(shell sysctl -n hw.cachelinesize)
else
CACHELINE_SIZE = $(shell getconf LEVEL1_DCACHE_LINESIZE)
endif

#includes
INCLUDE = -Iinclude
#linker params
LINKPARAMS = -fpic -lpthread -shared
#linker params for tests
LINKPARAMS_TEST = -fpic -lpthread
#options for development
CFLAGS = --std=c++11 -g -O3 -Wall -Werror -fpic -DIRIS_CACHELINE_SIZE=$(CACHELINE_SIZE) -Wno-unused-private-field
#options for release
#CFLAGS = --std=c++11 -g -O2 -Wall -Werror -fpic -shared

$(PROGRAM): $(IRIS_OBJS)
	$(CC) -o $(PROGRAM) $(IRIS_OBJS) $(CFLAGS) $(LINKPARAMS)

base_logger.o: src/base_logger.cpp include/base_logger.h
	$(CC) -c src/base_logger.cpp -o base_logger.o $(CFLAGS) $(INCLUDE)

buffered_writer.o: src/buffered_writer.cpp include/buffered_writer.h
	$(CC) -c src/buffered_writer.cpp -o buffered_writer.o $(CFLAGS) $(INCLUDE)

file_writer.o: src/file_writer.cpp include/file_writer.h
	$(CC) -c src/file_writer.cpp -o file_writer.o $(CFLAGS) $(INCLUDE)

stream_writer.o: src/stream_writer.cpp include/stream_writer.h
	$(CC) -c src/stream_writer.cpp -o stream_writer.o $(CFLAGS) $(INCLUDE)

level_logger.o: src/level_logger.cpp include/level_logger.h
	$(CC) -c src/level_logger.cpp -o level_logger.o $(CFLAGS) $(INCLUDE)

utils.o: src/utils.cpp include/utils.h
	$(CC) -c src/utils.cpp -o utils.o $(CFLAGS) $(INCLUDE)

notifier.o: src/notifier.cpp include/notifier.h
	$(CC) -c src/notifier.cpp -o notifier.o $(CFLAGS) $(INCLUDE)

test: test_lfringbuffer

test_lfringbuffer: $(IRIS_TEST_OBJS)
	$(CC) -o test_lfringbuffer $(IRIS_TEST_OBJS) $(CFLAGS) $(LINKPARAMS_TEST)

test_lfringbuffer.o: tests/test_lfringbuffer.cpp
	$(CC) -c tests/test_lfringbuffer.cpp -o test_lfringbuffer.o $(CFLAGS) $(INCLUDE)

clean:
	rm -rf *.o
	rm -rf $(PROGRAM) test_lfringbuffer

.PHONY: clean test