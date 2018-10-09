#include <stdint.h>     	// Var def for the C99 std
#include <stdbool.h>    	// Boolean def for the C99 std
#include "inc/hw_memmap.h" // Macros def memory map of Tiva C Series.
#include "inc/hw_types.h"  // Defines common types and macros
#include “driverlib/sysctl.h" //Defines macros for System Control API
#include "driverlib/gpio.h" 
#include “driverlib/interrupt.h"// Def macros for interrupt controller
#include "driverlib/timer.h"
#include “inc/tm4c123gh6pm.h" 

int main(void)
{
    uint32_t ui32Period;

    // system clock runs at 40MHz ()
    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

    // Enable the GPIO peripheral and configure the pins connected to the LEDs as outputs.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);

	// Enable clock to the peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0); 
	
	// Configure Timer0 as 32 bit timer in periodic mode.
   TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    /* Calculate Delay GPIO at 2Hz 75% duty cycle 
generate an interrupt at 
 3/4 of the desired period.
     * First calculate the # of clock cycles required for 2Hz period by calling SysCtclockGet() and dividing desired frequency.
     * Then divide the result by (0.75 = 3/4).
     * Finally load the value you get into timers interval load register
     */
    ui32Period = (SysCtlClockGet() / 2) * 0.75;
    TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);
    IntEnable(INT_TIMER0A);  // Enable the interrupt in the timer module
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT); // Enables a specific event within the timer to generate an interrupt.
    IntMasterEnable(); // Enables the specific vector associated with Timer0A.

    TimerEnable(TIMER0_BASE, TIMER_A); // Enable timer

    while(1)
    {
    }
}

void Timer0IntHandler(void)
{
    // Clear the timer interrupt
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    // Read the current state of the GPIO pin and
    // write back the opposite state
    if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2))
    {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
    }
    else
    {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 4);
    }
}
