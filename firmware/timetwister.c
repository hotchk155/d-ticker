
//
// HEADER FILES
//
#include <system.h>

// PIC CONFIG BITS
// - RESET INPUT DISABLED
// - WATCHDOG TIMER OFF
// - INTERNAL OSC
#pragma DATA _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _MCLRE_OFF &_CLKOUTEN_OFF
#pragma DATA _CONFIG2, _WRT_OFF & _PLLEN_OFF & _STVREN_ON & _BORV_19 & _LVP_OFF
#pragma CLOCK_FREQ 16000000

#include "uart_debug.h"
#include "tan.h"

/*
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

#define P_LED0 lata.1
#define P_CLOCKOUT latc.4
#define P_EXTCLOCK latc.5

#define P_LED1 lata.5
#define P_LED2 latc.3
#define P_LEDCOM lata.0

#define T_LED1 trisa.5
#define T_LED2 trisc.3


//               76543210
#define TRIS_A 0b11111100
#define TRIS_C 0b11000111

//
// MACRO DEFS
//

// Timer related stuff
#define TIMER_0_INIT_SCALAR		5		// Timer 0 initialiser to overlow at 1ms intervals
volatile byte ms_tick = 0;				// once per millisecond tick flag used to synchronise stuff




////////////////////////////////////////////////////////////
// INTERRUPT HANDLER CALLED WHEN CHARACTER RECEIVED AT 
// SERIAL PORT OR WHEN TIMER 1 OVERLOWS
void interrupt( void )
{
	// timer 0 rollover ISR. Maintains the count of 
	// "system ticks" that we use for key debounce etc
	if(intcon.2)
	{
		tmr0 = TIMER_0_INIT_SCALAR;
		ms_tick = 1;
		intcon.2 = 0;
	}
}

int pot[4] = {0};

#define SZ_TEMPO_MAP 16
int tempo_map[SZ_TEMPO_MAP];
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



/*
1) LED1->COM
2) COM->LED2
3) LED2->COM
4) COM->LED1
*/
void set_led(int i) {
	T_LED1 = 1;
	T_LED2 = 1;
	switch(i) {
		case 3:
			P_LED1 = 1;
			P_LEDCOM = 0;
			T_LED1 = 0;
			break;
		case 2:
			P_LED2 = 0;
			P_LEDCOM = 1;
			T_LED2 = 0;
			break;
		case 1:
			P_LED2 = 1;
			P_LEDCOM = 0;
			T_LED2 = 0;
			break;
		case 0:
			P_LED1 = 0;
			P_LEDCOM = 1;
			T_LED1 = 0;
			break;
	}
}


int get_adc(int i) {
	switch(i) {
	case 0:
		adcon0 = ADCON0_POT1;
		break;
	case 1:
		adcon0 = ADCON0_POT2;
		break;
	case 2:
		adcon0 = ADCON0_POT3;
		break;
	case 3:
		adcon0 = ADCON0_POT4;
		break;
	default:
		return 0;
	}
	adcon0.1 = 1;
	while(adcon0.1);
	return adresh;
}


	
// MAIN
void main()
{ 
	// initialise app variables
	byte tickCount = 0;
	
	// osc control / 16MHz / internal
	osccon = 0b01111010;

	// configure io
	trisa = TRIS_A;              	
    trisc = TRIS_C;              
	ansela = 0b00000100;  
	anselc = 0b00000111;
	porta=0;
	portc=0;

   // configure analog input AN2 enabled
  
        // turn on the ADC
        // ADC clock is Fosc/32
        // Result left justified (8 bit value in adresh register)
        // Voltage reference is power supply (VDD)
        adcon1=0b00100000; //fOSC/32
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
	option_reg.5 = 0; // timer 0 driven from instruction cycle clock
	option_reg.3 = 0; // timer 0 is prescaled
	option_reg.2 = 0; // }
	option_reg.1 = 1; // } 1/16 prescaler
	option_reg.0 = 1; // }
	intcon.5 = 1; 	  // enabled timer 0 interrrupt
	intcon.2 = 0;     // clear interrupt fired flag

	
	//uart_init();
	// enable interrupts	
	intcon.7 = 1; //GIE
	intcon.6 = 1; //PEIE
/*	
while(1) {
		//uart_send_string("poop\r\n");
	for(int i=0; i<4; ++i) {
		set_led(i);
		delay_ms(100);
		set_led(-1);
		delay_ms(100);
	}
}
for(int i=0; i<128; ++i) {
		uart_send_number(i);
		uart_send_string("->");
		uart_send_number((byte)tan_table[i]);
		uart_send_string("\r\n");
}*/
	//while(1) disp(321);
	// App loop
		int q=0;
		int beat_led = 0;
	int i;
	int bbb = 0;
	for(;;)
	{	
//uart_send_string("pots=");		
		for(i=0; i<4; ++i) {
			pot[i] = 255-get_adc(i);
//uart_send_number(pot[i]);		
//uart_send_string(" ");		
		}
//uart_send_string("\r\n");		
		recalc();
		bbb=0;
		P_LED0=1;
		P_EXTCLOCK=1;
		beat_led = 20;
		
		for(i=0; i<16; ++i) {
			long bpm = ((long)520*(1024+(long)tempo_map[i]))/1024;
			//float bpm = 120.0*(1024.0+tempo_map[i])/1024.0;
			long t=60000/bpm;
//uart_send_string("step ");		
//uart_send_number(i);		
//uart_send_string(" ");		
//uart_send_long(bpm);		
//uart_send_string(" ");		
//uart_send_long(t);		
//uart_send_string("\r\n");		
			set_led(i/4);
			P_CLOCKOUT = 1;

			int j=20;
			while(t>0) {
				if(ms_tick) {
					ms_tick = 0;
					if(++bbb>125) {
						bbb=0;
						P_LED0=1;
						P_EXTCLOCK=1;						
						beat_led = 20;
					}
					if(beat_led) {
						if(!--beat_led) {
							P_LED0 = 0;
							P_EXTCLOCK=0;						
						}
					}

					--t;
					if(j>0) {
						if(!--j) {
							set_led(-1);
							P_CLOCKOUT = 0;
						}
					}
				}
			}
		
		}
	
	/*
		adcon0.1=1; 
		delay_ms(10);
		int n = adc2button(adresh);
		if(key == K_NONE && n != K_NONE) {		
			key = n;
			key_2_chord(key);		
		}
		else if(n == K_NONE) {
			key = K_NONE;
		}
		*/
		if(get_adc(q) > 127) {
			set_led(q);
		}
		else {
			set_led(-1);
		}
		if(++q>3)q=0;		
	}
}