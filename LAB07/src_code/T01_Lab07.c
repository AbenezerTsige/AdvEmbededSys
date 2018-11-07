#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h" //using timer1
#include "driverlib/interrupt.h" //using interrupt
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"

#include "driverlib/adc.h" //ADC for temperature

int main(void)
{
	//50MHz clock
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    //ADC0 Peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    ADCHardwareOversampleConfigure(ADC0_BASE, 64);

    //Configure and Enable Timer1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER1_BASE, TIMER_A, (SysCtlClockGet()/2)-1); //0.5 delay
    IntEnable(INT_TIMER1A);
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    //ADC Sequence, four step ADC sequencer
    ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_TS);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_TS);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_TS);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 3, ADC_CTL_TS|ADC_CTL_IE|ADC_CTL_END);

    //Enables UART0 and GPIOA
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //Transmit/Receive pins
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //UART Configure, baud rate of 115200, 8 transmit bits, 1 stop bit, 0 parity bits
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    IntMasterEnable();
    ADCSequenceEnable(ADC0_BASE, 1);    //Enable ADC0
    TimerEnable(TIMER1_BASE, TIMER_A); //Enable Timer1

    //INITALIZATION TEST
    UARTCharPut(UART0_BASE, 'E');
    UARTCharPut(UART0_BASE, 'n');
    UARTCharPut(UART0_BASE, 't');
    UARTCharPut(UART0_BASE, 'e');
    UARTCharPut(UART0_BASE, 'r');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, 'T');
    UARTCharPut(UART0_BASE, 'e');
    UARTCharPut(UART0_BASE, 'x');
    UARTCharPut(UART0_BASE, 't');
    UARTCharPut(UART0_BASE, ':');
    UARTCharPut(UART0_BASE, ' ');

    while (1) //let interrupt handler do the UART echo function
    {

    }

}

void Timer1IntHandler(void)
{

    //ADC0 Array
    uint32_t ui32ADC0Value[4];

    //Variables for Temperature: Average, Celsius, and Fahrenheit
    volatile uint32_t ui32TempAvg;
    volatile uint32_t ui32TempValueC;
    volatile uint32_t ui32TempValueF;

    //string to place temperature
    char temp[2];

    //Clear ADC Interrupt Flag and trigger ADC conversion
    ADCIntClear(ADC0_BASE, 1);
    ADCProcessorTrigger(ADC0_BASE, 1);

    //Wait for conversion to complete
    while(!ADCIntStatus(ADC0_BASE, 1, false))
    {
    }

    //Copies data from the specified sample sequencer output FIFO to a buffer in memory
    ADCSequenceDataGet(ADC0_BASE, 1, ui32ADC0Value);

    //Calculate Average, use it to get Celsuis Temp, then convert to Fahrenheit
    ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2] + ui32ADC0Value[3] + 2)/4;
    ui32TempValueC = (1475 - ((2475 * ui32TempAvg)) / 4096)/10;
    ui32TempValueF = ((ui32TempValueC * 9) + 160) / 5;

    //converts uint32_t to ascii, then prints value
    temp[0] = ui32TempValueF/10 + 0x30; // 0x30 is 0 in hex so it can convert it from int to char
    temp[1] = ui32TempValueF%10 + 0x30; // attain the 2nd figure and convert it to char
    UARTCharPut(UART0_BASE, temp[0]);
    UARTCharPut(UART0_BASE, temp[1]);
    UARTCharPut(UART0_BASE, '\r');
    UARTCharPut(UART0_BASE, '\n');
    SysCtlDelay(5000000);
}
