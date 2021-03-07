/*********************************************************
 * File: main.cpp
 * 
 * Description: Utilizes an ADC and a DAC with SPI communication to 
 * create a loop guitar effet pedal. 
 * 
 * Library Files Used: spi.c, spi.h, gpio.c, gpio.h														}
 *      Copyright(c): Leon de Boer(LdB) 2019, 2020							}
 *      Version: 1.10
 *      github:
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

#define SINGLE    0x00 // Single channel input for ADC
#define DIFF      0x01 // Differential channel input for ADC

/* Sine wave look up table*/
static uint16_t sine_table[254] =
{
 2048,2099,2150,2201,2251,2302,2353,2403,2453,2503,
 2553,2602,2651,2700,2748,2796,2843,2890,2936,2982,
 3027,3071,3115,3158,3201,3243,3284,3324,3364,3402,
 3440,3477,3513,3548,3583,3616,3648,3680,3710,3739,
 3767,3795,3821,3846,3870,3892,3914,3934,3953,3972,
 3988,4004,4018,4032,4044,4054,4064,4072,4079,4085,
 4089,4092,4094,4095,4094,4092,4089,4085,4079,4072,
 4064,4054,4044,4032,4018,4004,3988,3972,3953,3934,
 3914,3892,3870,3846,3821,3795,3767,3739,3710,3680,
 3648,3616,3583,3548,3513,3477,3440,3402,3364,3324,
 3284,3243,3201,3158,3115,3071,3027,2982,2936,2890,
 2843,2796,2748,2700,2651,2602,2553,2503,2453,2403,
 2353,2302,2251,2201,2150,2099,2048,1996,1945,1894,
 1844,1793,1742,1692,1642,1592,1542,1493,1444,1395,
 1347,1299,1252,1205,1159,1113,1068,1024,980,937,
 894,852,811,771,731,693,655,618,582,547,
 512,479,447,415,385,356,328,300,274,249,
 225,203,181,161,142,123,107,91,77,63,
 51,41,31,23,16,10,6,3,1,0,
 1,3,6,10,16,23,31,41,51,63,
 77,91,107,123,142,161,181,203,225,249,
 274,300,328,356,385,415,447,479,512,547,
 582,618,655,693,731,771,811,852,894,937,
 980,1024,1068,1113,1159,1205,1252,1299,1347,1395,
 1444,1493,1542,1592,1642,1692,1742,1793,1844,1894,
 1945,1996,2048,
};


// Main program
int main(int argc, char *argv[])
{
      // 2bytes * 44.1k samples/sec * 120 sec = 105840000 bytes
      uint16_t* loop_samples = (uint16_t*) malloc(105840000); 
      uint32_t loop_idx = 0;
      uint32_t current_loop_idx = 0;
      uint32_t read_timer = 0;
      uint32_t adc_timer = 0;
      uint8_t loop_idx_rst = 0;
      bool footswitch_val;
      bool current_footswitch;
      uint16_t adc_read_val = 0;
      uint16_t new_input = 0;
      uint16_t loop_val = 0;
      uint16_t mix_val = 0;
      
      printf("in main\n");

      /* Initialize SPI0, 44.1khz, Mode 0, no semaphore locks */
      SPI_HANDLE spi0 = SpiOpenPort(0x1, 0x0, 8, 1000000, SPI_MODE_0, false); 
      printf("spi0 handle: %s\n", spi0 ? "true" : "false");      

      /* Initialize SPI0, 44.1khz, Mode 0, no semaphore locks */
      SPI_HANDLE spi6 = SpiOpenPort(0x0, 0x3, 8, 1000000, SPI_MODE_0, false); 
      printf("spi6 handle: %s\n", spi6 ? "true" : "false");

      /* Pi2/3 peripheral address ... 4096 byte block */
      /* Address isn't used in this simple version you can just use 0 */
      /* As you get deeper into it you will work out why you may want address */
      GPIO_HANDLE gpio = GPIO_Open(0x3F200000, 0x1000);
      printf("gpio handle: %s\n", gpio ? "true" : "false");
      
      if (gpio && spi0 && spi6)
      {          
            /* Test that GPIO is working for debugging purposes
            GPIO_Setup(gpio, 8, GPIO_OUTPUT); //gpio 19 to output
            GPIO_Output(gpio,  8,  0);   // Gpio 8 off
            sleep(1); // Waits 1 second
            GPIO_Output(gpio,  8,  1);   // Gpio 8 on
            sleep(1); // Waits 1 second
            */
            
            // Loop Pedal
            // Set GPIO 12 as input for reading footswitch level        
            GPIO_Setup(gpio, 12, GPIO_INPUT); 
            // Check initial footswitch level (true = high, false = low)         
            footswitch_val = GPIO_Input(gpio, 12); // Check footswitch level
            printf("initial footswitch value %s\n", footswitch_val ? "true" : "false");
          
            while(1){ // main loop
                  read_timer++;

                  // Check footswitch level every 0.25 seconds for a press
                  if (read_timer == 50000)
                  {               
                      current_footswitch = GPIO_Input(gpio, 12); // Check footswitch level
                      //printf("footswitch %s\n", footswitch ? "true" : "false"); // left for debugging
                      read_timer = 0;
                  }

                  // New loop recording
                  while(current_footswitch != footswitch_val) 
                  {
                        // Rest loop index value on first iteration of a new recording                        
                        if (loop_idx_rst == 0)
                        {
                              loop_idx = 0;
                              loop_idx_rst = 1;
                        }
                        
                        // Read the ADC for a new sample value
                        // Single Channel ADC read configuration                                                
                        adc_read_val = ADC_READ(SINGLE, spi0);
                        
                        // Set the samples array equal to the ADC value at the current index
                        loop_samples[loop_idx] = adc_read_val;
                        
                        // Increment new footswitch read timer
                        read_timer++;
                        
                        if (read_timer == 50000)
                        {
                              current_footswitch = GPIO_Input(gpio, 12); // Check footswitch level
                              read_timer = 0;
                        }
                        
                        // Increment the loop index
                        loop_idx++;
                  }
                  // Reset the flag for proper use on the next recording 
                  if (loop_idx_rst == 1)
                  {
                        loop_idx_rst == 0;
                        // Reset the footswitch read timer
                        read_timer = 0;
                        // Set the current loop index value to 0
                        current_loop_idx = 0;
                  }
                  
                  // Read new input to be mixed with the recorded samples
                  new_input = ADC_READ(SINGLE, spi0);
                  // Value at current_loop_idx
                  loop_val = loop_samples[current_loop_idx];
                  
                  // Mix the new input with the recorded sample
                  mix_val = (2*(new_input + loop_val)) - ((new_input + loop_val)/512) - 1024;
                                    
                  // Write the mixed value to the DAC
                  DAC_WRITE(mix_val, spi6);
                  
                  // Increment the current loop index
                  current_loop_idx++; 
                  
                  // Reset the current loop index to resart the loop
                  if (current_loop_idx > loop_idx)
                  {
                        current_loop_idx = 0;
                  }
            }
      }
      
      else
      {
            printf("Error one or more port handles are invalid");
            printf("gpio handle: %s\n", gpio ? "true" : "false");
            printf("spi0 handle: %s\n", spi0 ? "true" : "false");
            printf("spi6 handle: %s\n", spi6 ? "true" : "false");
          
            if(gpio)
            {
                  GPIO_Close(gpio); // Close gpio
            }
            
            else if(spi0)
            {
                  SpiClosePort(spi0); // Close spi0
            }
            
            else if(spi6)
            {
                  SpiClosePort(spi6); // Close spi6
            }
      }
}
