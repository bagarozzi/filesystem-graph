CFLAGS = -std=c++20 -Ofast -Wall -Wpedantic -Werror -Wno-missing-braces -Iinclude
CLIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

all: main.cpp
	g++ main.cpp -o graph ${CFLAGS} ${CLIBS}
