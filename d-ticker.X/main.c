
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

static struct { 
    byte reset_signal;
} dt;

enum {    
    MAX_INPUT_STEPS = 16,
    MAX_OUTPUT_RATE = 4,
    MAX_TRIGS = (MAX_INPUT_STEPS * MAX_OUTPUT_RATE)
};

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
            if(P_EXTRESET) {
                dt.reset_signal = 0;
            }
            else {
                dt.reset_signal = 1;
            }
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
    
    pat_recalc();
    
    dt.reset_signal = 0;
    
    // main application loop
    int cur_trig = 0;
    unsigned int pos = 0;
	for(;;) {
        
        // run the UI
        ui_run();
        
        // get the updated clock position
        unsigned int new_pos = clk_get_pos();
        byte event = clk_get_event();
        if(event & CLK_STEP4) {                    
            P_CLOCKLED = 1;
            leds_set_clock(1, MED_LED_BLINK_MS);
        }
        else if(event & CLK_STEP1) {                    
            leds_set_clock(1, SHORT_LED_BLINK_MS);
        }
        if(event & CLK_RESTART) { // reset signal
            // reset the pattern ignoring remaining trigs
            cur_trig = 0;
        }            
        else if(new_pos < pos) { // wrap around to start of pattern
            // send any remaining triggers
            while(cur_trig++ < pat_get_num_trigs()) {                   
                out_trig();
            }                
            cur_trig = 0;
        }
        pos = new_pos;

        
        while(cur_trig < pat_get_num_trigs()) {
            if(pat_get_trig(cur_trig) > pos) {
                break;
            }
            ++cur_trig;
            out_trig(); 
            ui_trig((byte)((4*cur_trig)/pat_get_num_trigs()));
        }
	}
}