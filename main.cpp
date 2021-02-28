
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

// ADC Defines
//{CMD[7:6] = Device Addr}{CMD[5:2] = Register Addr}{CMD[1:0] = }
// Write/read format: {command byte, data byte, dont care command}
#define ADC_CONFIG0_WRITE 0x46 // 0x46 = 01000110 => Incremental Write CONFIG0 register
#define ADC_CONFIG3_WRITE 0x4E // 0x4E = 01001110 => Incremental Write CONFIG3 register 
#define ADC_IRQ_REG_WRITE 0x56 // 0x56 = 01010110 => Incremental Write IRQ register
#define ADC_MUX_REG_WRITE 0x5A // 0x5A = 01011010 => Incremental Write MUX register
#define ADC_CONFIG0_READ  0x45 // 0x45 = 01000101 => Static Read of CONFIG0 register
#define ADC_ADCDATA_READ  0x41 // 0x41 = 01000001 => Static Read of the ADC data register
#define ADC_CONFIG0_SETUP 0xF3 // 0xE3 = 11110011 => ADC_MODE = 11, CS_SEL = 00, CLK_SEL = 11
#define ADC_CONFIG3_SETUP 0xC0 // 0xC0 = 11000000 => ADC_MODE = Continuous
#define ADC_IRQ_REG_SETUP 0x77 // 0x77 = 01110111 => default values + no pull-up resistor
#define ADC_MUX_REG_SETUP 0x08 // 0x08 = 00001000 => MUX_VIN+ = CH0, MUX_VIN- = AGND
#define ADC_START_CONV    0x68 // 0x68 = 01101000 => Start conversion fast command
#define ADC_DONT_CARE_CMD 0x7C // 0x7C = 01111100 => DONT CARE (IGNORE)

// DAC Defines
// Write format: {address byte, command byte, data byte}
//#define DAC_DATA_CMD                0x08 // 0x08 = 00001000 => DAC Data Command
//#define DAC_DATA_ADDR_WRITE         0x94 // 0x94 = 10010100 => Write to DAC Data Address (LSB = 1 => Write)
//#define DAC_DATA_ADDR_WRITE_CMD     0x9408
//#define DAC_CMD_DATA_VALUES         0x8454500  // 0001000 1000101 1000101 00000000
//#define DAC_GAIN_CMD                0x04
//#define DAC_GAIN_DIV                0x01  // Divide VREF by 2 
//#define DAC_GAIN_GAIN               0x00  // Gain of 1       
//#define DAC_TRIG_CMD                0x05
//#define DAC_TRIG_RES                0x00
//#define DAC_TRIG_DEFAULT            0x0A  // Reset to default state   
//#define DAC_CONFIG_CMD              0x03
//#define DAC_CONFIG_REF_OFF          0x00  // Turns off VREF
//#define DAC_CONFIG_PWR_DWN          0x00  // Disables output   

#define EN_BIT   0x80 // 0x80 = 10000000 = BIT7
#define BUF_BIT  0x40 // 0x40 = 01000000 = BIT6
#define GAIN     0x20 /* 0x20 = 00100000 = BIT5 
                      If high, Vo = Vref * D/4096
                    * if low, 2(Vo = 2Vref * D/4096) */
#define SHDN     0x10 // 0x10 = 00010000 = BIT4



// Main program
int main(int argc, char *argv[])
{
    // Clk frequency = 44.1kHz
    // SCLK = Core Clock / CDIV;

    printf("in main\n");
    
    /* Initialize SPI0, 44.1khz, Mode 0, no semaphore locks */
    SPI_HANDLE spi0 = SpiOpenPort(0x1, 0x0, 8, 44100, SPI_MODE_0, false); 
    printf("spi0 handle: %d\n", spi0);
    
    /* Initialize SPI0, 44.1khz, Mode 0, no semaphore locks */
    SPI_HANDLE spi6 = SpiOpenPort(0x0, 0x3, 8, 44100, SPI_MODE_0, false); 
    printf("spi6 handle: %d\n", spi6);


    if (spi0 && spi6)
    {
      //while(1)
      //{
            uint8_t ADDRESS_CMD = 0x01;
            // 0x33 = 110011 
            uint8_t config0_buf[3] = {ADC_CONFIG0_WRITE, ADC_CONFIG0_SETUP, ADC_DONT_CARE_CMD};	// Sets CONFIG0 Register bits
            uint8_t config0Val_buf[3] = {ADC_CONFIG0_READ, 0x45, ADC_DONT_CARE_CMD};
            uint8_t config3_buf[3] = {ADC_CONFIG3_WRITE, ADC_CONFIG3_SETUP, ADC_DONT_CARE_CMD};	// Sets CONFIG3 Register bits
            // might need to change since the read value is 16 bits
            uint8_t irq_buf[3] = {ADC_IRQ_REG_WRITE, ADC_IRQ_REG_SETUP, ADC_DONT_CARE_CMD}; // Sets IRQ Register bits
            uint8_t mux_buf[3] = {ADC_MUX_REG_WRITE, ADC_MUX_REG_SETUP, ADC_DONT_CARE_CMD}; // Sets IRQ Register bits
            //uint16_t adcdata_buf[2] = {ADC_ADCDATA_READ, 0x01};	// Sets up read of ADCDATA Register bits
            //uint8_t fast_buf[1] = {ADC_START_CONV};
            
            uint16_t config0_Rxbuf[3] = {0x45, 0x45, 0x45};	// Sets CONFIG0 Register bits
            uint16_t config0Val_Rxbuf[3] = {0x45, 0x45, 0x45};
            uint16_t config3_Rxbuf[2] = {0x45, 0x45};	// Sets CONFIG3 Register bits
            // might need to change since the read value is 16 bits
            uint16_t irq_Rxbuf[2] = {0x45, 0x45}; // Sets IRQ Register bits
            uint16_t mux_Rxbuf[2] = {0x45, 0x45}; // Sets IRQ Register bits
        
            //printf("adcdata_buf: %d, %d\n", adcdata_buf[0], adcdata_buf[1]);
            int irq_val = SpiWriteAndRead(spi0, &irq_buf[0], &irq_Rxbuf[0], 2, false);   // Transfer buffer data to SPI call
            printf("irq return value: %d\n",irq_val);
            printf("irq buf values: %d, %d\n",irq_Rxbuf[0], irq_Rxbuf[1]);
            
            int mux_val = SpiWriteAndRead(spi0, &mux_buf[0], &mux_Rxbuf[0], 2, false);   // Transfer buffer data to SPI call
            printf("mux return value: %d\n",mux_val);
            printf("mux buf values: %d, %d\n",mux_Rxbuf[0], mux_Rxbuf[1]);
            
            int config0_val = SpiWriteAndRead(spi0, &config0_buf[0], &config0_Rxbuf[0], 3, false);   // Transfer buffer data to SPI call
            printf("config0 return value: %d\n",config0_val);
            printf("config0Rx buf values: %d, %d, %d\n",config0_Rxbuf[0], config0_Rxbuf[1], config0_Rxbuf[2]);
            
            int config0_data = SpiWriteAndRead(spi0, &config0Val_buf[0], &config0Val_Rxbuf[0], 3, false);   // Transfer buffer data to SPI call
            printf("config0 read return value: %d\n",config0_data);
            printf("config0 readRx buf values: %d, %d, %d\n",config0Val_Rxbuf[0], config0Val_Rxbuf[1], config0Val_Rxbuf[2]);
            
            int config3_val = SpiWriteAndRead(spi0, &config3_buf[0], &config3_Rxbuf[0], 2, false);   // Transfer buffer data to SPI call
            printf("config3 return value: %d\n",config3_val);
            printf("config3 buf values: %d, %d\n",config3_Rxbuf[0], config3_Rxbuf[1]);
            
             //while(1){ 
            
            //SpiWriteAndRead(spi, &fast_buf[0], &fast_buf[0], 1, false);
            
            uint8_t adcdata_buf[3] = {ADC_ADCDATA_READ, 0x45, ADC_DONT_CARE_CMD};	// Sets up read of ADCDATA Register bits    
            uint16_t adcdata_Rxbuf[3] = {0x45, 0x45, 0x45};
            int adcdata_val = SpiWriteAndRead(spi0, &adcdata_buf[0], &adcdata_Rxbuf[0], 3, false);   // Transfer buffer data to SPI call
            printf("adcdata return value: %d\n",adcdata_val);
            printf("adc_Rxvalues: %d, %d\n", adcdata_Rxbuf[0], adcdata_Rxbuf[1]);
            //printf("spi handle tx value: %d\n", spi->tx_buf);
            //printf("spi handle rx calue: %d\n", spi->rx_buf);
            
            //uint8_t DAC_DATA_VALUES = 0x45; 
            // Writes the value(s) in the Tx buffer to the DAC data register
            //uint8_t dacdata_buf[3] = {DAC_DATA_ADDR_WRITE, DAC_DATA_CMD, DAC_DATA_VALUES}; 
            
            //uint16_t dacdata_Rxbuf[3] = {0x45, 0x45, 0x45}; // DAC Rx data write buffer
            
            //int dacdata_val = SpiWriteAndRead(spi6, &dacdata_buf[0], &dacdata_Rxbuf[0], 3, false);   // Transfer buffer data to SPI call
            //printf("dac data return value: %d\n",dacdata_val);
            //printf("dacdata_Rxbuf values: %d, %d, %d\n",dacdata_Rxbuf[0], dacdata_Rxbuf[1], dacdata_Rxbuf[2]);
      //}
        SpiClosePort(spi0);
    }

    if (spi6)
    {
      
            uint16_t output;
            uint8_t loByte;
            uint8_t hiByte;
            output = 0;
            for (int i = 0; i < 100; i++)
            {
                  for (int i = 0; i < 509; i++)
                  {
                        output = output + 8;
                        
                        /* Get the lower 8 bits of the input and store it in loByte */
                        loByte = output & 0xFF;

                        /* get the higher 8 bits and store it in hiByte */
                        hiByte = (output >> 8) & 0xFF;

                        /* To send a command to the DAC, the highest 4 bits should be:
                        * 0, BUF, ~GA, and ~SHDN */
                        hiByte &= ~(EN_BIT | BUF_BIT);
                        hiByte |= GAIN | SHDN;


                        uint8_t dacdata_buf[2] = {hiByte, loByte}; 
                        //printf("dac data buff: %x, %x\n",dacdata_buf[0], dacdata_buf[1]);
                        uint16_t dacdata_Rxbuf[2] = {0x45, 0x45}; // DAC Rx data write buffer
                        
                        int dacdata_val = SpiWriteAndRead(spi6, &dacdata_buf[0], &dacdata_Rxbuf[0], 3, false);   // Transfer buffer data to SPI call// printf("dac data return value: %d\n",dacdata_val);
                        //printf("dac data return value: %d\n",dacdata_val);
                        //printf("dacdata_Rxbuf values: %d, %d\n",dacdata_Rxbuf[0], dacdata_Rxbuf[1]);
                  }
            }
      //}
        SpiClosePort(spi6);
    }

}
