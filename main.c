/*
 * File:   main.c
 * Author: Antti Järveläinen, Rasmus Riihimäki, Arttu Salmijärvi, Julius Kähkönen, Esko Vuorinen
 *
 * Created on 15 December 2020, 21:50
 */

#define F_CPU 3333333
#include <avr/io.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#define SERVO_PWM_PERIOD (0x1046)
#define SERVO_PWM_DUTY_NEUTRAL (0x0138)

void SLPCTRL_init(void);

volatile uint16_t adcVal = 0;

void ADC0_init(void);
uint16_t ADC0_read(void);
void ADC0_start(void);

void ADC0_init(void)
{
    /* Disable digital input buffer */
    PORTE.PIN0CTRL &= ~PORT_ISC_gm;
    PORTE.PIN0CTRL |= PORT_ISC_INPUT_DISABLE_gc;

    /* Disable pull-up resistor */
    PORTE.PIN0CTRL &= ~PORT_PULLUPEN_bm;

    // CLK_PER divided by 16, internal reference
    ADC0.CTRLC = ADC_PRESC_DIV16_gc | ADC_REFSEL_INTREF_gc;

    /* ADC Enable: enabled, 10-bit mode */
    ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc;

    /* Select ADC channel */
    ADC0.MUXPOS = ADC_MUXPOS_AIN8_gc;
    
    // Set internal reference
    VREF.CTRLA |= VREF_ADC0REFSEL_1V5_gc; 

    // Enable ADC interrupts
    ADC0.INTCTRL |= ADC_RESRDY_bm;
}
// Read adc value
uint16_t ADC0_read(void)
{
    return ADC0.RES;
}
void ADC0_start(void)
{
    /* Start conversion */
    ADC0.COMMAND = ADC_STCONV_bm;
}

int main(void) 
{
    ADC0_init();

    // Route TCA0 PWM waveform to PORTB
    PORTMUX.TCAROUTEA |= PORTMUX_TCA0_PORTB_gc;

    // Set 0-WO2 (PB2) as digital output
    PORTB.DIRSET = PIN2_bm;

    // Set TCA0 prescaler value to 16 (~208 kHz)
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV16_gc;

    // Set single-slop PWM generation mode
    TCA0.SINGLE.CTRLB |= TCA_SINGLE_WGMODE_SINGLESLOPE_gc;

    // Using double-buffering, set PWM period (20 ms)
    TCA0.SINGLE.PERBUF = SERVO_PWM_PERIOD;

    // Set initial servo arm position as neutral (0 deg)
    TCA0.SINGLE.CMP2BUF = SERVO_PWM_DUTY_NEUTRAL;

    // Enable Compare Channel 2
    TCA0.SINGLE.CTRLB |= TCA_SINGLE_CMP2EN_bm;

    // Enable TCA0 peripheral
    TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;
   
    // Set port for LDR
    PORTE.IN = PORTE.IN | PIN0_bm;
    
    // Set IDLE sleep mode
    set_sleep_mode(SLPCTRL_SMODE_IDLE_gc);

    // Enable interrupts
    sei();
    
    ADC0_start();
 
    while (1)
    {
        cli();

        // Jump if there is cactus
        if (adcVal > 790)
        {
             TCA0.SINGLE.CMP2BUF = 208;
            _delay_ms(300);
            TCA0.SINGLE.CMP2BUF = 300;
            _delay_ms(280);
        }
        sei();
        sleep_mode();
    }

}

ISR(ADC0_RESRDY_vect)
{
    adcVal = ADC0_read();
    ADC0_start();
}
