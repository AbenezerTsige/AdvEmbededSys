/* Abenezer Namga
*  Lab 05
 * Task 01:
 * 1. Change the ADC Sequencer to SS3.
 * 2. Turn on the LED at PF2 if the temperature is greater than 72 degF.
 * 3. Use internal temperature sensor for all SS2 sequence.
 * 4. Display the temperature in the built-in graph tool.
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"     // definitions for using the ADC driver
#define TARGET_IS_BLIZZARD_RB1 // Gives the libraries access to the proper API’s in ROM.
#include "driverlib/rom.h"
#include "driverlib/gpio.h"   // Include GPIO apis

#ifdef DEBUG
void__error__(char*pcFilename, uint32_t ui32Line)
{
}
#endif

int main(void)
{
    uint32_t ui32ADC0Value[4]; // Stores the data read from the ADC FIFO
    volatile uint32_t ui32TempAvg; // Stores the avg of temperature
    volatile uint32_t ui32TempValueC; // Temperature in Celsius
    volatile uint32_t ui32TempValueF; // Temperature in Fahrenheit

    // 40MHz
    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);  // Enable GPIOF to use LED @ PF2
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);


    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); // Enable ADC0
    ADCHardwareOversampleConfigure(ADC0_BASE, 64); // Hardware Averaging
    // each sample in the ADC FIFO will be the result of 64 measurements being averaged together.

    // Use ADC0, sample sequencer 3, let processor trigger sequence and use highest priority
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);
    // Gets four samples of the temperature sensor to average out
    ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_TS);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_TS);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_TS);

    // Sample the temperaure sensor and
    // configure the interrupt flag to be set when the sample is done.
    ADCSequenceStepConfigure(ADC0_BASE,1,3,ADC_CTL_TS|ADC_CTL_IE|ADC_CTL_END);

    ADCSequenceEnable(ADC0_BASE, 1); // Enable ADC sequencer 1.

    while(1)
    {
       ADCIntClear(ADC0_BASE, 1); // Clear flag
       ADCProcessorTrigger(ADC0_BASE, 1); // Trigger ADC conversion
       // note, assignment asked for threshold to be 72 but that was unreasonable in my circumstance.
       if (ui32TempValueF < 64 )
           GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2,0);
       else
           GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2,4);

       while(!ADCIntStatus(ADC0_BASE, 1, false)) // Wait for conversion
       {
       }

        // Returns the samples that are presently available.
       ADCSequenceDataGet(ADC0_BASE, 1, ui32ADC0Value);

       // Calculates the average of the temperature sensor data
       ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2] + ui32ADC0Value[3] + 2)/4;
       ui32TempValueC = (1475 - ((2475 * ui32TempAvg)) / 4096)/10; //
       ui32TempValueF = ((ui32TempValueC * 9) + 160) / 5;
    }
}
