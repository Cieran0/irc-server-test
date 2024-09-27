ifeq ($(OS),Windows_NT)
    output := win_server.exe
    libs := -lws2_32
    nproc := 6 # set to 6 threads for windows
else
    output := linux_server
    libs := -lpthread
    nproc := $(shell nproc)  # set to number of cpu threads for linux
endif

MAKEFLAGS += -j$(nproc)

# Object files
objects := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

# Parallelized build
all: $(output)

$(output): $(objects)
	g++ -std=c++20 $(objects) -Wall $(libs) -o $(output) -Iinclude
	rm -f $(objects)

%.o: %.cpp
	g++ -std=c++20 -c $< -o $@ -Iinclude

clean: $(ouput)
	rm -f $(objects)

.PHONY: all clean
