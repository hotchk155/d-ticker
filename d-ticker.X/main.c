/*
 * File:   main.c
 * Author: jason
 *
 * Created on 28 November 2024, 14:30
 */



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

typedef unsigned char byte;
#include "tan.h"


/*

1  VDD
2  RA5				LED1
3  RA4/SDO			RESET_IN	
4  RA3/MCLR#/VPP	SWITCH
5  RC5/RX			CLOCK_IN
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
#define ADCON0_POT4	0b00001001
#define ADCON0_POT3	0b00010001
#define ADCON0_POT2	0b00010101
#define ADCON0_POT1	0b00011001

//
// TYPE DEFS
//
//typedef unsigned char byte;

#define P_CLOCKLED LATAbits.LATA1
#define P_CLOCKOUT LATCbits.LATC4

#define P_LED1 LATAbits.LATA5
#define P_LED2 LATCbits.LATC3
#define P_LEDCOM LATAbits.LATA0

#define T_LED1 TRISAbits.TRISA5
#define T_LED2 TRISCbits.TRISC3
#define T_LEDCOM TRISAbits.TRISA0

#define P_EXTCLOCK PORTCbits.RC5
#define P_EXTRESET PORTAbits.RA4
#define P_SWITCH PORTAbits.RA3

#define WPUA_BITS 0b00001000

//               76543210
#define TRIS_A 0b11111100
#define TRIS_C 0b11100111

//
// MACRO DEFS
//

// Timer related stuff
#define TIMER_0_INIT_SCALAR		5		// Timer 0 initialiser to overlow at 1ms intervals
volatile byte ms_tick = 0;				// once per millisecond tick flag used to synchronise stuff


// This array contains 8-bit ADC readings of the four potentiometers
// and it is kept updated by 
#define NUM_POTS 4
volatile byte pot[NUM_POTS] = {0};
volatile int which_pot = 0;

#define DEBOUNCE_CLOCK_IN 10
volatile byte debounce_clock_in = 0;
volatile byte prev_clock_in = 0;
volatile byte clock_in_signal = 0;

#define DEBOUNCE_RESET_IN 10
volatile byte debounce_reset_in = 0;
volatile byte prev_reset_in = 0;
volatile byte reset_in_signal = 0;

#define CLOCK_LED_TIMEOUT 10
volatile byte clock_led_timeout = 0;

#define SZ_TEMPO_MAP 16
int tempo_map[SZ_TEMPO_MAP];

// define the time base used for scheduling output triggers...
// this will divide the full bar into 65536 units
#define MAX_TICKS 0xFFFF
typedef unsigned int TICKS;
volatile TICKS cur_ticks = 0;

// The schedule for each tick
#define SZ_TRIG_SCHEDULE 16
TICKS trig_schedule[SZ_TRIG_SCHEDULE];

void adc_begin() {
	switch(which_pot) {
	case 0:
		ADCON0 = ADCON0_POT1;
		break;
	case 1:
		ADCON0 = ADCON0_POT2;
		break;
	case 2:
		ADCON0 = ADCON0_POT3;
		break;
	case 3:
		ADCON0 = ADCON0_POT4;
		break;
	default:
		return;
	}
	ADCON0bits.GO_nDONE = 1;
}

////////////////////////////////////////////////////////////
void __interrupt() ISR()
{
	// timer 0 rollover ISR. Maintains the count of 
	// "system ticks" that we use for key debounce etc
    
	if(INTCONbits.T0IF)
	{
		TMR0 = TIMER_0_INIT_SCALAR;
		ms_tick = 1;
		
		// poll the clock input
		if(debounce_clock_in) {
			--debounce_clock_in;
		}
		else if(P_EXTCLOCK) {
			if(!prev_clock_in) {
				prev_clock_in = 1;
				clock_in_signal = 1;
				debounce_clock_in = DEBOUNCE_CLOCK_IN;
				
P_CLOCKLED = 1;
clock_led_timeout = CLOCK_LED_TIMEOUT;
			}
		}
		else {
			prev_clock_in = 0;
		}
		
		// poll the reset input
		if(debounce_reset_in) {
			--debounce_reset_in;
		}
		else if(P_EXTRESET) {
			if(!prev_reset_in) {
				prev_reset_in = 1;
				reset_in_signal = 1;
				debounce_reset_in = DEBOUNCE_RESET_IN;
			}
		}
		else {
			prev_reset_in = 0;
		}
		
		// manage clock LED
		if(clock_led_timeout) {
			if(!--clock_led_timeout) {
				P_CLOCKLED = 0;
			}
		}
		INTCONbits.T0IF = 0;
	}
	
	// ADC reading complete
    if(PIR1bits.ADIF) {
	//if(pir1.6) { 
		pot[which_pot] = ADRESH;
		if(++which_pot >= NUM_POTS) {
			which_pot = 0;
		}
		PIR1bits.ADIF = 0;
		adc_begin();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Turn on one of four LEDs by tri-stating 3 lines 
// 1) LED1->COM
// 2) COM->LED2
// 3) LED2->COM
// 4) COM->LED1
/////////////////////////////////////////////////////////////////////////////
void set_led(int i) {
	T_LED1 = 1;
	T_LED2 = 1;
	T_LEDCOM = 1;
	switch(i) {
		case 0:
			P_LED1 = 1;
			P_LEDCOM = 0;
			T_LED1 = 0;
			T_LEDCOM = 0;
			break;
		case 1:
			P_LED2 = 0;
			P_LEDCOM = 1;
			T_LED2 = 0;
			T_LEDCOM = 0;
			break;
		case 2:
			P_LED2 = 1;
			P_LEDCOM = 0;
			T_LED2 = 0;
			T_LEDCOM = 0;
			break;
		case 3:
			P_LED1 = 0;
			P_LEDCOM = 1;
			T_LED1 = 0;
			T_LEDCOM = 0;
			break;
	}
}


/////////////////////////////////////////////////////////////////////////////
// Recalculate the tempo map
/////////////////////////////////////////////////////////////////////////////
void recalc() {

	// We read each of the four potentiometers as an 8 bit value
	// -127 to +127 which is treated as an angle -45 to +45 degrees
	// for one of four straight line sections running end to end from
	// y-position zero
	//
	//          |_________|         |
	//         /|         |---      |
	//       /  |         |   ---   |       
	//     /    |         |      ---|	___---
	//   /      |         |         |---
	// /        |         |         |
	// ----+----|----+----|----+----|----+----
	//  256 256 | 256 256 | 256 256 | 256 256
	//
	// 
	int i;
	// calculate the y-displacement along each of the four line
	// sections, based on pot input and 512 unit width
	int dy[4];
	int base_dy = 0;
	for(i=0; i<4; ++i) {	
		int q = pot[i];
//uart_send_string(" //q=");		
//uart_send_number(q);		
		int t;
		int index;
		if(q>127) { // positive angle/increasing tempo
			index = q-128;
			t =  (byte)tan_table[index];
		}
		else { // negative angle/dec reasing tempo
			index = 127-q;
			t =  -(byte)tan_table[index];
		}		
		
//uart_send_string(" index=");		
//uart_send_number(index);		
//uart_send_string(" t=");		
//uart_send_number(t);		
		dy[i] = 2*t;
//uart_send_string(" dy=");		
//uart_send_number(dy[i]);		
//uart_send_string(" ");		
	}
//uart_send_string("\r\n");		
	
	// calculate the area under the graph.. maximum area is 1/2(0 + 2048)2048 = 0x200000
	// which corresponds to all four pots being at the same full extent
	int y1 = 0;
	long area = 0;
	for(i=0; i<4; ++i) {	
		int y2 = y1 + dy[i];
		area += ((long)256*(y1+y2)); // 1/2(a+b)h
		y1=y2;
	}
//uart_send_string("area=");		
//uart_send_long(area);		
//uart_send_string("\r\n");		
	
	// now calculate the y offset needed to all the points so that the area under
	// the graph would be zero. To do this, we'll take half the area under the graph
	// and divide it by length of the x-axis (2048). If the graph is then offset down
	// the y-axis by this value the area under the graph will be zero
	int y_offset = -area/2048;

//uart_send_string("offset=");		
//uart_send_long((long)y_offset);		
//uart_send_string("\r\n");		
	
	// now we can build the tempo map... each element in this array represents an offset of 
	// the input BPM to be applied during that step of the map	
	for(int i=0; i<SZ_TEMPO_MAP; ++i) {
		int section = ((i/4)%4);
		
		// for this tempo map slot we'll use the average y-value along the tempo line section
		// within the current "slot". There are 4 slots per section (each 128 units wide)
		// so we look at the midpoint of the slot (64 units in, or 1/8 of the full y offset
		// over the whole slot
		// 
		//
		// |           ----|
		// |       ----    |
		// |   ----        |
		// |---            |
		// |  256  |  256  |
		// |128|128|128|128|
		// | |  
		
		tempo_map[i] = y_offset + dy[section]/8;
		y_offset = y_offset + dy[section]/4;		
//uart_send_string("tempo[");		
//uart_send_number(i);		
//uart_send_string("]=");		
//uart_send_number(tempo_map[i]);		
//uart_send_string("\r\n");		
		
	}
}





void test_pots() {
	for(;;) {
		int j = pot[0];
		set_led(j/64);
		//set_led(0);
	}
}

void test_leds() {
    int count = 0;
    for(;;) {
        set_led(count++%4);        
        for(int i=0; i<500; ++i) {
            ms_tick = 0;
            while(!ms_tick);
            P_CLOCKOUT = count&1;
            P_CLOCKLED = count&1;
        }
    }
}	
void test_inputs() {
    for(;;) {
        P_CLOCKLED = P_SWITCH;
    }
}	

// MAIN
void main()
{ 
	// initialise app variables
	byte tickCount = 0;
	
	// osc control / 16MHz / internal
	OSCCON = 0b01111010;

	// configure io
	TRISA = TRIS_A;              	
    TRISC = TRIS_C;              
	ANSELA = 0b00000100;  
	ANSELC = 0b00000111;
	PORTA=0;
	PORTC=0;

   // configure analog input AN2 enabled
  
        // turn on the ADC
        // ADC clock is Fosc/32
        // Result left justified (8 bit value in adresh register)
        // Voltage reference is power supply (VDD)
        ADCON1=0b00100000; //fOSC/32
  //      adcon0=0b00001001; 


/*
	// Configure timer 1 (controls tempo)
	// Input 4MHz
	// Prescaled to 500KHz
	tmr1l = 0;	 // reset timer count register
	tmr1h = 0;
	t1con.7 = 0; // } Fosc/4 rate
	t1con.6 = 0; // }
	t1con.5 = 1; // } 1:8 prescale
	t1con.4 = 1; // }
	t1con.0 = 1; // timer 1 on
	pie1.0 = 1;  // timer 1 interrupt enable
	*/
	
	// Configure timer 0 (controls systemticks)
	// 	timer 0 runs at 4MHz
	// 	prescaled 1/16 = 250kHz
	// 	rollover at 250 = 1kHz
	// 	1ms per rollover	
	//option_reg.5 = 0; // timer 0 driven from instruction cycle clock
	//option_reg.3 = 0; // timer 0 is prescaled
	//option_reg.2 = 0; // }
	//option_reg.1 = 1; // } 1/16 prescaler
	//option_reg.0 = 1; // }
 
    WPUA = WPUA_BITS;
    WPUC = 0;
    
    OPTION_REGbits.TMR0CS = 0;  // timer 0 driven from instruction cycle clock
    OPTION_REGbits.PSA = 0;     // timer 0 is prescaled
    OPTION_REGbits.PS = 0b011;  // 1/16 prescaler
    OPTION_REGbits.nWPUEN = 0;
    
    INTCONbits.T0IE = 1;    // enabled timer 0 interrrupt
    INTCONbits.T0IF = 0;    // clear interrupt fired flag
	//intcon.5 = 1; 	  // enabled timer 0 interrrupt
	//intcon.2 = 0;     // clear interrupt fired flag

    PIR1bits.ADIF = 0;
    PIE1bits.ADIE = 1; // enable the ADC interrupt
	//pir1.6 = 0;
	//pie1.6 = 1; // enable the ADC interrupt
	
	//uart_init();
	// enable interrupts	
	//intcon.7 = 1; //GIE
	//intcon.6 = 1; //PEIE
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;

    test_inputs();
    
	// poll pots
	adc_begin();
	
	for(;;) {
	}
}