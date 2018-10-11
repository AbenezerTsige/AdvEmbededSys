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
    volatile uint8_t ui8Adjust;
    ui8Adjust = 83;

    //40-MHz clock
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
    ROM_SysCtlPWMClockSet(SYSCTL_PWMDIV_64);

    //Enables PWM1
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); //Button pins set to Port F

    //Pin F1,F2, and F3 are PWM output
    ROM_GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
    ROM_GPIOPinConfigure(GPIO_PF1_M1PWM5);
    ROM_GPIOPinConfigure(GPIO_PF2_M1PWM6);
    ROM_GPIOPinConfigure(GPIO_PF3_M1PWM7);

    //Divide internal system clock by 64 for PWM clock
    ui32PWMClock = SysCtlClockGet() / 64;
    ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1; //get load count
    PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN); //PWM generator configure
    PWMGenConfigure(PWM1_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN);
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, ui32Load); //load ui32Load as count
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_3, ui32Load);

    //Sets pulse width and enables PWM outputs
    ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5_BIT, ui8Adjust * ui32Load / 1000);
    ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6_BIT, ui8Adjust * ui32Load / 1000);
    ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7_BIT, ui8Adjust * ui32Load / 1000);
    ROM_PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT|PWM_OUT_6_BIT|PWM_OUT_7_BIT, true);
    ROM_PWMGenEnable(PWM1_BASE, PWM_GEN_2); //enables PWM0
    ROM_PWMGenEnable(PWM1_BASE, PWM_GEN_3);

    while(1)
    {
        //90% PWM
        uint16_t a = 900; //RED LED
        uint16_t b = 900; //BLUE LED
        uint16_t c = 900; //GREEN LED

        //Three nested for loops, change to change to 10%
        for(;a>100;--a) //for loops ends with LED being red
        {
            ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, a * ui32Load / 1000);
            ROM_SysCtlDelay(10000);
            for(;b>100;--b) //LED with be purple without green
            {
                ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, b * ui32Load / 1000);
                ROM_SysCtlDelay(10000);
                for(;c>100;--c) //for loop begins with LED being white
                {
                    ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, c * ui32Load / 1000);
                    ROM_SysCtlDelay(10000);
                }
            }
        }
    }
}


