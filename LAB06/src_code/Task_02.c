#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "inc/hw_gpio.h"
#include "driverlib/rom.h"

//55-Hz
#define PWM_FREQUENCY 55

int main(void)
{
    //PWM variables, ui8Adjust will determine position of servo motor
    volatile uint32_t ui32Load;
    volatile uint32_t ui32PWMClock;
    volatile uint16_t ui8Adjust;
    ui8Adjust = 83;

    //40-MHz clock
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
    ROM_SysCtlPWMClockSet(SYSCTL_PWMDIV_64);

    //Enables PWM1
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); //Button pins set to Port F

    //Pin F1 is PWM output
    ROM_GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);
    ROM_GPIOPinConfigure(GPIO_PF1_M1PWM5);

    //Unlock GPIO control register
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;

    //PF0 and PF4 pins as inputs
    ROM_GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_DIR_MODE_IN);
    ROM_GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    //Divide internal system clock by 64 for PWM clock
    ui32PWMClock = SysCtlClockGet() / 64;
    ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1; //get load count
    PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN); //PWM generator configure
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, ui32Load); //load ui32Load as count

    //Sets pulse width and enables PWM outputs
    ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui8Adjust * ui32Load / 1000);
    ROM_PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT, true);
    ROM_PWMGenEnable(PWM1_BASE, PWM_GEN_2); //enables PWM0

    while(1)
    {
        //Checks if SW1 was pressed
        if(ROM_GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4)==0x00)
        {
            //Increases ui8Adjust  until min limit is reached (10%)
            ui8Adjust--;
            if (ui8Adjust < 100)
            {
                ui8Adjust = 100;
            }
            //Sets pulse width
            ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui8Adjust * ui32Load / 1000);
        }
        //Checks if SW2 was pressed
        if(ROM_GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_0)==0x00)
        {
            //Increases ui8Adjust until max limit is reached (90%)
            ui8Adjust++;
            if (ui8Adjust > 900)
            {
                ui8Adjust = 900;
            }
            //Sets pulse width
            ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui8Adjust * ui32Load / 1000);
        }
        //System delay for LED
        ROM_SysCtlDelay(100000);
    }
}
