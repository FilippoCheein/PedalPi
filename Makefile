ARGS=-Wall -O3 -lpthread

main: main.cpp spi.c gpio.c adc_dac_cmds.cpp
	g++ -o main main.cpp spi.c gpio.c adc_dac_cmds.cpp -I $(ARGS)
