/*******************************************************
Abenezer Namaga
Midterm Code 
Date:10/29/2018

Goal: 
•	Configure I2c interface between TM4C123GXL board and TSL2951 sensor
•	Configure UART communication between TM4C123GXL and ESP8266 module
•	Send TSL2951 data readings to cloud server hosted on thingspeak.com

******************************************************
Chip type               : ARM TM4C123GH6PM
Program type            : Firmware
Core Clock frequency	: 80.000000 MHz
*******************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_i2c.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "driverlib/rom_map.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"

uint32_t ui32SysClock;


const uint8_t TSL2591address = 0x29;


//UART1
void InitConsole(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    GPIOPinConfigure(GPIO_PB0_U1RX);
    GPIOPinConfigure(GPIO_PB1_U1TX);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    UARTClockSourceSet(UART1_BASE, UART_CLOCK_PIOSC);
    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(1, 115200, 16000000);
}

void ftoa(float f,char *buf)
{
       /*Function acquired from forum:
        * http://e2e.ti.com/support/microcontrollers/stellaris_arm/f/471/p/44193/156824.aspx
        */

	/*Parses through float # and stores the value as a character array*/
    int pos=0,ix,dp,num;
    if (f<0)
    {
        buf[pos++]='-';
        f = -f;
    }
    dp=0;
    while (f>=10.0)
    {
        f=f/10.0;
        dp++;
    }
    for (ix=1;ix<8;ix++)
    {
            num = (int)f;
            f=f-num;
            if (num>9)
                buf[pos++]='#';
            else
                buf[pos++]='0'+num;
            if (dp==0) buf[pos++]='.';
            f=f*10.0;
            dp--;
    }
}


// USING I2C0 - CHECK FOR TIVAC LAUNCHPAD PINS
void i2c0_init()
{
		//enable i2c0
       MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
       //reset i2c0
       MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
       //Set pin type for PB3, configure as data pin
       MAP_GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);
       MAP_GPIOPinConfigure(GPIO_PB3_I2C0SDA);

       //Set pin type for PB2, configure as SCL
       MAP_GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
       MAP_GPIOPinConfigure(GPIO_PB2_I2C0SCL);

       //Enable and initialize i2c0 master module. Use System Clock.  Set i2C data rate at 100kbps (try true for 400)
       I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet(), false);
              while (I2CMasterBusy(I2C0_BASE));

}

void i2c0_write(uint8_t dev_addr, uint8_t dev_reg, uint16_t dev_data)
{

	//Tell device we want to write to bus
    I2CMasterSlaveAddrSet(I2C0_BASE, dev_addr, false);
    //Bitwise or command bit to tell device we want to write to dev_reg
    I2CMasterDataPut(I2C0_BASE, TSL2591_COMMAND_BIT |dev_reg);
    	//Send and wait
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
       while (I2CMasterBusy(I2C0_BASE));
       //Now write the data to the register
    I2CMasterSlaveAddrSet(I2C0_BASE, dev_addr, true);
      //place the data in Master and send
    I2CMasterDataPut(I2C0_BASE, dev_data);
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
       while (I2CMasterBusy(I2C0_BASE));

}


uint8_t i2c0_read(uint32_t slave_addr, uint8_t reg)
{
    //specify that we are writing (a register address) to the
    //slave device
    I2CMasterSlaveAddrSet(I2C0_BASE, slave_addr, false);
    while(I2CMasterBusy(I2C0_BASE));
    //specify register to be read
    I2CMasterDataPut(I2C0_BASE, TSL2591_COMMAND_BIT | reg);
    while(I2CMasterBusy(I2C0_BASE));
    //send control byte and register address byte to slave device
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND);

    //wait for MCU to finish transaction
    while(I2CMasterBusy(I2C0_BASE));
    //specify that we are going to read from slave device
    I2CMasterSlaveAddrSet(I2C0_BASE, slave_addr, true);
    while(I2CMasterBusy(I2C0_BASE));
    //send control byte and read from the register we
    //specified
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);
    //wait for MCU to finish transaction
    while(I2CMasterBusy(I2C0_BASE));
    //return data pulled from the specified register
    return I2CMasterDataGet(I2C0_BASE);
}

void TSL2591_init()
{
       uint8_t DevID;
       i2c0_write(TSL2591address,TSL2591address,1); //literally no idea what this does


       DevID = i2c0_read(TSL2591address,TSL2591_REGISTER_DEVICE_ID); //read device ID
       //Alert user if errors reading ID

    	// Set Gain and Timing
       i2c0_write(TSL2591address,0x01, 0xB0 );//CONFIG medium gain and reset the device (self-clearing)
       i2c0_write(TSL2591address, 0x00, 0x03);//ENABLE Power, Oscillator,

}

void TSL2591_disable()
{
	i2c0_write(TSL2591address,0xA0, 0x00); //turn off TSL2591
}

float getLuminosity ()
{
  float    atime = 100.0F, again=25.0F; //For 100ms integration time and med gain
  float    cpl, lux1, lux2, lux;
  uint16_t ch0 ;
  uint16_t ch1 ;

// Get full luminosity
  //read channel 0 into x0
  uint16_t x0;
  uint16_t y0;


  y0 = i2c0_read(TSL2591address,0x14);
  while(I2CMasterBusy(I2C0_BASE));
 // UARTprintf("\n\nChannel 0L: %d", y0);
  x0 = i2c0_read(TSL2591address,0x15); //C0DataHigh
  while(I2CMasterBusy(I2C0_BASE));
 // UARTprintf("\nChannel 0H: %d", x0);
  x0 <<= 8;


  uint16_t x1;
  uint16_t y1;
  //read channel 1 into x1
  y1 = i2c0_read(TSL2591address, 0x16);
  while(I2CMasterBusy(I2C0_BASE));
 // UARTprintf("\nChannel 1L: %d", y1);
  x1 = i2c0_read(TSL2591address, 0x17);
  while(I2CMasterBusy(I2C0_BASE));
 // UARTprintf("\nChannel 1H: %d", x1);
  x1 <<= 8;

// Disable Sensor


  ch0 = x0+y0;
  ch1 = x1+y1;
//Calculate Lux value from sensor
	cpl = (atime * again) / TSL2591_LUX_DF;
  lux1 = ( (float)ch0 - (TSL2591_LUX_COEFB * (float)ch1) ) / cpl;
  lux2 = ( ( TSL2591_LUX_COEFC * (float)ch0 ) - ( TSL2591_LUX_COEFD * (float)ch1 ) ) / cpl;
  lux = lux1 > lux2 ? lux1 : lux2;

  return lux;
}

void ESPSENDPREP()
{
	SysCtlDelay(2000000);
	UARTprintf("AT+CIPMUX=1\n\r");
	SysCtlDelay(2000000);
	UARTprintf("AT+CIPSTART=4,\"TCP\",\"184.106.153.149\",80\n\r");
	SysCtlDelay(2000000);
//	SysCtlDelay(20000000);
//	UARTprintf("GET /update?key=N034S3W0VGVGGI5I&field1=50\n\r");
//	SysCtlDelay(20000000);
//	UARTprintf("GET /update?key=N034S3W0VGVGGI5I&field1=\n\r");

}

int main(void)
{
	char myLux[48] = "GET /update?key=N034S3W0VGVGGI5I&field1=";//52 bit string, 42 instr, 6 are lux. (append \n\r at end)
    float lux_read;
  ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                  SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                  SYSCTL_CFG_VCO_480), 120000000);
  InitConsole();
  i2c0_init();

  while(1)
  {

	// UARTprintf("AT\n");

	 TSL2591_init();
	 SysCtlDelay(2000000);
	 lux_read = getLuminosity();
	 ftoa(lux_read, &myLux[40]);
	 myLux[47]='\0';
TSL2591_disable();
	 ESPSENDPREP();

		UARTprintf("AT+CIPSEND=4,49\n\r");
		SysCtlDelay(20000000);
	 UARTprintf("%s\n\r",&myLux);
	 SysCtlDelay(20000000);
		UARTprintf("AT+CIPSEND=4,49\n\r");
		SysCtlDelay(20000000);
	 UARTprintf("%s\n\r",&myLux);



	 SysCtlDelay(5000000*20); //slightly more than 15 seconds per so
  }
}

