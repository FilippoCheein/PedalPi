ARGS=-Wall -O3 -lpthread

main: main.cpp spi.c gpio.c
	g++ -o main main.cpp spi.c gpio.c -I $(ARGS)
