all: main.cpp
	g++ main.cpp -o graph -std=c++20 -Ofast -s -Wall -Wno-missing-braces -I include/ -lraylib -lGL -lm -pthread -ldl -lrt -lX11
