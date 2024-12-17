#include <xc.h>
#include "d-ticker.h"

#define P_CLOCKLED LATAbits.LATA1

#define P_LED1 LATCbits.LATC5
#define P_LED2 LATCbits.LATC3
#define P_LEDCOM LATAbits.LATA0

#define T_LED1 TRISCbits.TRISC5
#define T_LED2 TRISCbits.TRISC3
#define T_LEDCOM TRISAbits.TRISA0

struct {
    int clock_timeout;
    int pos_timeout;
} leds;

/////////////////////////////////////////////////////////////////////////////
// Turn on one of four LEDs by tri-stating 3 lines 
// 1) LED1->COM
// 2) COM->LED2
// 3) LED2->COM
// 4) COM->LED1
/////////////////////////////////////////////////////////////////////////////
static void set_pos_leds(int i) {
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
void leds_init() {
    leds.clock_timeout = 0;
    leds.pos_timeout = 0;
 	T_LED1 = 1;
	T_LED2 = 1;
	T_LEDCOM = 1;
}
/////////////////////////////////////////////////////////////////////////////
inline void leds_ms_isr() {
    if(leds.pos_timeout && !--leds.pos_timeout) {
         set_pos_leds(-1);
     }
     if(leds.clock_timeout && !--leds.clock_timeout) {
         P_CLOCKLED = 0;
     }    
}
/////////////////////////////////////////////////////////////////////////////
inline void leds_set_clock(byte state, byte timeout) {
    if(state) {
        P_CLOCKLED = 1;
        leds.clock_timeout = timeout;
    }
    else {
        P_CLOCKLED = 0;
        leds.clock_timeout = 0;        
    }
}
/////////////////////////////////////////////////////////////////////////////
inline void leds_clear_pos() {
    set_pos_leds(-1);
    leds.pos_timeout = 0;
}
/////////////////////////////////////////////////////////////////////////////
inline void leds_set_pos(byte which, byte timeout) {
    set_pos_leds(which);
    leds.pos_timeout = timeout;
}