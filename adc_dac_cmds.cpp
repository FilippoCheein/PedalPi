/*********************************************************
 * File: ADC_DAC_CMDS.cpp
 * 
 * Description: Functions for writing to the DAC and reading the ADC
 * over SPI communication protocol. Requires a valid SPI HANDLE generated
 * from  the spi.c library file.
 * 
 * Authors: Izak Walker, Filippo Cheein
 * 
 * Revisions: N/A
 * 
 **********************************************************/

#include <stdbool.h>			// C standard unit for bool, true, false
#include <stdint.h>				// C standard unit for uint32_t etc
#include <fcntl.h>				// Needed for SPI port
#include <sys/ioctl.h>			// Needed for SPI port
#include <linux/spi/spidev.h>	// Needed for SPI port
#include <unistd.h>				// Needed for SPI port
#include <stdio.h>				// neded for sprintf_s
#include <pthread.h>			// Posix thread unit
#include <semaphore.h>			// Linux Semaphore unit
#include "spi.h"				// This units header
#include "gpio.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "adc_dac_cmds.h"

// ADC Defines
#define START			0x01 // 0x01 = 00000001 => Start read 
#define SINGLE_CHO		0x80 // 0x80 = 10000000 => VIN = CH0 - Vss
#define	DIFF_CH0_CH1	0x00 // 0x00 = 00000000 => VIN = CH0 - CH1
#define DONT_CARE		0xFF // 0xFF = 11111111 => DONT CARE CMD

// DAC Defines
#define EN_BIT   0x80 // 0x80 = 10000000 = BIT7
#define BUF_BIT  0x40 // 0x40 = 01000000 = BIT6
#define GAIN     0x20 /* 0x20 = 00100000 = BIT5 
                      If high, Vo = Vref * D/4096
                    * if low, 2(Vo = 2Vref * D/4096) */
#define SHDN     0x10 // 0x10 = 00010000 = BIT4

void DAC_WRITE(uint16_t output, SPI_HANDLE spiHandle)
{
	uint8_t loByte;
	uint8_t hiByte;

	/* Get the lower 8 bits of the input and store it in loByte */
	loByte = output & 0xFF;

	/* get the higher 8 bits and store it in hiByte */
	hiByte = (output >> 8) & 0xFF;

	/* To send a command to the DAC, the highest 4 bits should be:
	* 0, BUF, ~GA, and ~SHDN */
	hiByte &= ~(EN_BIT | BUF_BIT);
	hiByte |= GAIN | SHDN;

	uint8_t dac_data_buf[2] = {hiByte, loByte};
	
	uint8_t dac_data_Rxbuf[2] = {0x45, 0x45}; // DAC Rx data write buffer
                        
    SpiWriteAndRead(spiHandle, &dac_data_buf[0], &dac_data_Rxbuf[0], 2, false);   
}

uint16_t ADC_READ(uint8_t MODE, SPI_HANDLE spiHandle)
{
	uint16_t read_val;
	uint8_t loByte;
	uint8_t hiByte;	
	
	// Default MODE = 0x00 (Single Channel)
	uint8_t adc_data_buf[3] = {START, SINGLE_CHO, DONT_CARE};
	uint8_t adc_data_Rxbuf[3] = {0x45, 0x45, 0x45};
	// MODE = 0x01 => Differential input (CH0 = VIN+ ; CH1 = VIN-)
	if (MODE == 0x01)
	{
		uint8_t adc_data_buf[3] = {START, DIFF_CH0_CH1, DONT_CARE};
	}
		
	SpiWriteAndRead(spiHandle, &adc_data_buf[0], &adc_data_Rxbuf[0], 3, false); 
	// The Low Byte is in the last index of the Rx buffer
	loByte = adc_data_Rxbuf[2];
	// The 2 most signoficant bits are in the second to last Rx buffer
	hiByte = adc_data_Rxbuf[1];
	// Only want the first two bits
	hiByte = hiByte & 0x03;
	// read_val = {hiByte,loByte}
	read_val = (hiByte << 8) + loByte;
	
	return read_val;
}
