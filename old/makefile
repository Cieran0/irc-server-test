ifeq ($(OS),Windows_NT)
    output := win_server.exe
    libs := -lws2_32
else
    output := linux_server
    libs := -lpthread
endif

all:
	g++ -std=c++20 *.cpp -Wall $(libs) -o $(output) -Iinclude