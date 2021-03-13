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
#include "adc_dac_cmds.h"

#define SINGLE    0x00 // Single channel input for ADC
#define DIFF      0x01 // Differential channel input for ADC

// Main program
int main(int argc, char *argv[])
{
      // Contiguous chunck of memory for loop sample values
      uint16_t* loop_samples = (uint16_t*) malloc(52920000); 
      uint32_t loop_idx = 0;
      uint32_t current_loop_idx = 0;
      uint16_t read_timer = 0;
      uint16_t rec_read_timer = 0;
      uint32_t adc_timer = 0;
      uint8_t loop_idx_rst = 1;
      bool footswitch_val;
      bool current_footswitch;
      uint16_t adc_read_val = 0;
      uint16_t new_input = 0;
      uint16_t loop_val = 0;
      uint32_t mix_val = 0;
      uint16_t out_signal = 0;
      uint8_t first_rec_flag = 1;

      /* Initialize SPI0, 1.25MHz, Mode 0, no semaphore locks */
      SPI_HANDLE spi0 = SpiOpenPort(0x1, 0x0, 8, 1250000, SPI_MODE_0, false); 
      printf("spi0 handle: %s\n", spi0 ? "true" : "false");      

      /* Initialize SPI6, 1.25MHz, Mode 0, no semaphore locks */
      SPI_HANDLE spi6 = SpiOpenPort(0x0, 0x3, 8, 1250000, SPI_MODE_0, false); 
      printf("spi6 handle: %s\n", spi6 ? "true" : "false");

      /* Initialize gpio pins for input/output */
      GPIO_HANDLE gpio = GPIO_Open(0x3F200000, 0x1000);
      printf("gpio handle: %s\n", gpio ? "true" : "false");
      
      // Check that the spi and gpio handles are valid
      if (gpio && spi0 && spi6)
      {                      
            // Loop Pedal
            // Set GPIO 12 as input for reading footswitch level        
            GPIO_Setup(gpio, 12, GPIO_INPUT); 
            // Check initial footswitch level (true = high, false = low)         
            footswitch_val = GPIO_Input(gpio, 12); // Check footswitch level
            current_footswitch = GPIO_Input(gpio, 12); // Check footswitch level 
            printf("initial footswitch value %s\n", footswitch_val ? "true" : "false");
          
            while(1){ // main loop
                  read_timer++;

                  // Check footswitch level every 0.25 seconds for a press
                  if (read_timer == 50000)
                  {               
                      // Check footswitch level
                      current_footswitch = GPIO_Input(gpio, 12);                       
                      read_timer = 0;
                  }

                  // New loop recording
                  while(current_footswitch != footswitch_val && loop_idx < 52920000) 
                  {
                        first_rec_flag = 0;
                        // Rest loop index value on first iteration of a new recording                        
                        if (loop_idx_rst == 1)
                        {
                              loop_idx = 0;                              
                              loop_idx_rst = 0;
                              first_rec_flag = 0;
                        }
                        
                        // Read the ADC for a new sample value
                        // Single Channel ADC read configuration                                                
                        adc_read_val = ADC_READ(SINGLE, spi0);
                        
                        // Set the samples array equal to the ADC value at the current index
                        loop_samples[loop_idx] = adc_read_val;
                        
                        // Increment new footswitch read timer
                        rec_read_timer++;
                        
                        if (rec_read_timer == 50000)
                        {
                              // Check footswitch level
                              current_footswitch = GPIO_Input(gpio, 12); 
                              rec_read_timer = 0;
                        }
                        
                        // Increment the loop index
                        loop_idx++;
                  }
                  // Reset the flag for proper use on the next recording 
                  if (loop_idx_rst == 0)
                  {
                        loop_idx_rst == 1;
                        // Reset the footswitch read timer
                        rec_read_timer = 0;
                        // Set the current loop index value to 0
                        current_loop_idx = 0;
                  }
                  
                  // Read new input to be mixed with the recorded samples
                  new_input = ADC_READ(SINGLE, spi0);
                  // Value at current_loop_idx
                  loop_val = loop_samples[current_loop_idx];

                  // Mix the new input with the recorded sample
                  if(new_input < 512 && loop_val < 512)
                  {
                        mix_val = (new_input * loop_val)>>9;
                  }
                  
                  else
                  {
                        mix_val = (2*(new_input + loop_val)) - ((new_input * loop_val)>>9) - 1024;
                  }
                              
                  out_signal = mix_val;
                  
                  // Only pass through if there has not been an initial recording
                  if (first_rec_flag)
                  {
                        out_signal = new_input;
                  }
                  
                  // Write the mixed value to the DAC
                  DAC_WRITE(out_signal, spi6);
                  
                  // Increment the current loop index

		  // Decimate every third sample 
                  if (current_loop_idx % 3 == 0){
                  current_loop_idx += 2; 
                  }
                  
                  else{
                        current_loop_idx += 1;
                  }
                  
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
