#ifndef adc_dac_cmds_h
#define adc_dac_cmds_h

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif

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

#include <stdbool.h>		// C standard unit for bool, true, false
#include <stdint.h>		// C standard unit for uint32_t etc
#include <fcntl.h>		// Needed for SPI port
#include <sys/ioctl.h>		// Needed for SPI port
#include <linux/spi/spidev.h>	// Needed for SPI port
#include <unistd.h>		// Needed for SPI port
#include <stdio.h>		// neded for sprintf_s
#include <pthread.h>		// Posix thread unit
#include <semaphore.h>		// Linux Semaphore unit
#include "spi.h"		// This units header
#include "gpio.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <stdbool.h>							// C standard unit for bool, true, false
#include <stdint.h>								// C standard unit for uint32_t etc

#define SPI_DRIVER_VERSION 1100					// Version number 1.10 build 0

typedef struct spi_device* SPI_HANDLE;			// Define an SPI_HANDLE pointer to opaque internal struct

#define NSPI 2									// 2 SPI devices supported

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

void DAC_WRITE(uint16_t output, SPI_HANDLE spiHandle);

uint16_t ADC_READ(uint8_t MODE, SPI_HANDLE spiHandle);


#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif
