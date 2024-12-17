
// PIC16F1825 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = OFF      // MCLR Pin Function Select (MCLR/VPP pin function is digital input)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Memory Code Protection (Data memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = ON        // Internal/External Switchover (Internal/External Switchover mode is enabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = ON       // PLL Enable (4x PLL enabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config DEBUG = OFF      // In-Circuit Debugger Mode (In-Circuit Debugger disabled, ICSPCLK and ICSPDAT are general purpose I/O pins)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include "d-ticker.h"


/*

1  VDD
2  RA5				CLOCK_IN        exLED1
3  RA4/SDO			RESET_IN	
4  RA3/MCLR#/VPP	SWITCH
5  RC5/RX			LED1            exCLOCK_IN
6  RC4/TX			CLOCK_OUT
7  RC3/SS#			LED2
8  RC2				POT1/AN6
9  RC1/SDA/SDI		POT2/AN5
10 RC0/SCL/SCK		POT3/AN4
11 RA2/INT			POT4/AN2
12 RA1/ICPCLK		LED0
13 RA1/ICPDAT		LED_COM
14 VSS

RA2	POT4	AN2
RC0	POT3	AN4
RC1	POT2	AN5
RC2	POT1	AN6
*/

//
// TYPE DEFS
//
//typedef unsigned char byte;



#define P_EXTCLOCK PORTAbits.RA5
#define P_EXTRESET PORTAbits.RA4

#define WPUA_BITS 0b00001000

#define IOCAN_BITS 0b00110000
#define IOCAP_BITS 0b00010000
#define IOCAF_EXTCLOCK IOCAFbits.IOCAF5
#define IOCAF_EXTRESET IOCAFbits.IOCAF4

//               76543210
#define TRIS_A 0b11111101
#define TRIS_C 0b11101111

//
// MACRO DEFS
//

// Timer related stuff
#define TIMER_0_INIT_SCALAR		5		// Timer 0 initialiser to overlow at 1ms intervals

////////////////////////////////////////////////////////////
void __interrupt() ISR()
{
	// timer 0 rollover ISR. Maintains the count of 
	// "system ticks" that we use for key debounce etc
    
	if(INTCONbits.T0IF)
	{
		TMR0 = TIMER_0_INIT_SCALAR;
        out_ms_isr();
        clk_ms_isr();
        pots_ms_isr();
        leds_ms_isr();
        INTCONbits.T0IF = 0;
	}
	
    ////////////////////////////////////////////////////////
	// ADC reading complete
    if(PIR1bits.ADIF) {
        pots_read_isr();
		PIR1bits.ADIF = 0;
	}
    
    ////////////////////////////////////////////////////////
    // detect input from external clock/reset
    if(INTCONbits.IOCIF) {
        if(IOCAF_EXTCLOCK){
            clk_ext_pulse_isr();
            IOCAF_EXTCLOCK = 0;
        }
        if(IOCAF_EXTRESET){
            seq_reset_signal_isr(!P_EXTRESET);
            IOCAF_EXTRESET = 0;
        }
        INTCONbits.IOCIF = 0;
    }

}



//void test_pots() {
	//for(;;) {
//		int j = pots_reading(3);
//		set_led(j/64);
//		//set_led(0);
//	}
//}


// MAIN
void main()
{ 
	// osc control / 16MHz / internal
	OSCCON = 0b01111010;

	// configure io
	TRISA = TRIS_A;              	
    TRISC = TRIS_C;              
	ANSELA = 0b00000100;  
	ANSELC = 0b00000111;
	PORTA=0;
	PORTC=0; 
    WPUA = WPUA_BITS;
    WPUC = 0;
    
    // turn on the ADC
    // ADC clock is Fosc/32
    // Result left justified (8 bit value in adresh register)
    // Voltage reference is power supply (VDD)
    ADCON1=0b00100000; //fOSC/32
	// Configure timer 0 (controls systemticks)
	// 	timer 0 runs at 4MHz
	// 	prescaled 1/16 = 250kHz
	// 	rollover at 250 = 1kHz
	// 	1ms per rollover	    
    OPTION_REGbits.TMR0CS = 0;  // timer 0 driven from instruction cycle clock
    OPTION_REGbits.PSA = 0;     // timer 0 is prescaled
    OPTION_REGbits.PS = 0b011;  // 1/16 prescaler
    OPTION_REGbits.nWPUEN = 0;
    
    INTCONbits.T0IE = 1;    // enabled timer 0 interrrupt
    INTCONbits.T0IF = 0;    // clear interrupt fired flag

    PIR1bits.ADIF = 0;
    PIE1bits.ADIE = 1; // enable the ADC interrupt

    // interrupt on change
    IOCAN = IOCAN_BITS;
    IOCAP = IOCAP_BITS;
    INTCONbits.IOCIF = 0;
    INTCONbits.IOCIE = 1;

	
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;

    out_init();
    leds_init();
    clk_init();
    pat_init();
    pots_init();   
    ui_init();
    seq_init();
    
    seq_run();
    
}