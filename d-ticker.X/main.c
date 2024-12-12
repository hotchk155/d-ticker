
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
typedef unsigned int TCOUNT;

enum {    
    MAX_INPUT_STEPS = 16,
    MAX_OUTPUT_RATE = 4,
    MAX_TRIGS = (MAX_INPUT_STEPS * MAX_OUTPUT_RATE)
};

int tt[MAX_TRIGS];
TCOUNT g_trig[MAX_TRIGS];
int g_num_trigs;
int e_num_input_steps;
int e_output_rate;
int g_trigs_per_section;


volatile byte pot_mov_detect;
volatile byte pot_mov_done;
volatile int pot_mov_timeout;

enum {
    POT_MOV_THRESHOLD = 1,
    POT_MOV_TIMEOUT = 200
};
enum {
    INPUT_STEPS_4,
    INPUT_STEPS_8,
    INPUT_STEPS_16,
    INPUT_STEPS_32
};
enum {
    OUTPUT_RATE_DIV2,
    OUTPUT_RATE_X1,
    OUTPUT_RATE_X2,
    OUTPUT_RATE_X4
};


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

#define CLOCK_LED_TIMEOUT 10
volatile byte clock_led_timeout = 0;

#define OUTPUT_PULSE_MS 10
#define OUTPUT_PULSE_LOW_MS 5
volatile byte trig_out_count = 0;   // number of trigger pulses to generate
volatile byte trig_out_timeout = 0;


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

        // flag the end of potentiometer movement
        if(pot_mov_timeout) {
            if(!--pot_mov_timeout) {
                pot_mov_done = 1;
            }
        }

		// manage clock LED
		if(clock_led_timeout) {
			if(!--clock_led_timeout) {
				P_CLOCKLED = 0;
			}
		}
        
        // manage trig output
        if(trig_out_timeout) {
            --trig_out_timeout;
            if(trig_out_timeout <= OUTPUT_PULSE_LOW_MS) {
                P_CLOCKOUT = 0;
            }
        }
        else if(trig_out_count) {
            --trig_out_count;
            P_CLOCKOUT = 1;
            trig_out_timeout = (OUTPUT_PULSE_MS + OUTPUT_PULSE_LOW_MS);
        }
        
                
		INTCONbits.T0IF = 0;
	}
	
	// ADC reading complete
    if(PIR1bits.ADIF) {
	//if(pir1.6) { 
        byte reading = ADRESH;
        if(abs(reading - pot[which_pot]) > 1) {
            pot_mov_detect = 1;
            pot_mov_timeout = 200;
            pot[which_pot] = reading;            
        }
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
    

    g_num_trigs = 16; //TODO
    
    
    // expand out the velocity(tempo) changes (defined by the pots) into a 
    // list of velocity values at each output trigger position    
    int cur_rate = 128;
    int min_rate = 128;
    int trig;
    for(trig=0; trig<g_num_trigs; ++trig) {
        tt[trig] = cur_rate;        
        int acc = 128-pot[(trig*4)/g_num_trigs];
        cur_rate += acc;
        if(cur_rate < min_rate) {
            min_rate = cur_rate;
        }        
    }
    
    // normalise the velocities so that they are all positive and 
    // expand out the "distance into sequence" by integrating velocity
    int dist = 0;
    for(trig=0; trig<g_num_trigs; ++trig) {
        int norm_rate = tt[trig] - min_rate + 128;
        tt[trig] = dist;
        dist = dist + norm_rate;
    }
    
    // now normalise the distances so that they run from 0 - 65535
    for(trig=0; trig<g_num_trigs; ++trig) {
        g_trig[trig] = (TCOUNT)(((long)65535 * tt[trig]) / dist);
    }
}

void test_pots() {
	for(;;) {
		int j = pot[3];
		set_led(j/64);
		//set_led(0);
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
	
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;

    
    
    pot_mov_detect = 0;
    pot_mov_done = 0;
    pot_mov_timeout = 0;

	// poll pots
	adc_begin();
	
    
    
 
    //e_num_input_steps = INPUT_STEPS_16;
    //e_output_rate = OUTPUT_RATE_X1;
    recalc();
    
    int led_count = 0;
    int cur_trig = 0;
    TCOUNT timer = 0;
    TCOUNT rate = 8;    
	for(;;) {
        if(ms_tick) {
            ms_tick = 0;
            if(pot_mov_done) {
                P_CLOCKLED = 1;
                recalc();
                P_CLOCKLED = 0;
                pot_mov_done = 0;
            }
            while(cur_trig<g_num_trigs) {
                if(g_trig[cur_trig] > timer) {
                    break;
                }
                set_led((4*cur_trig)/g_num_trigs);
                //P_CLOCKLED = 1;
                led_count = 10;
                ++cur_trig;
            }
            timer += rate;
            if(timer <= rate) {
                cur_trig = 0;
            }
            if(led_count) {
                if(!--led_count) {
                    //P_CLOCKLED = 0;
                    set_led(-1);
                }
            }
        }
	}
}