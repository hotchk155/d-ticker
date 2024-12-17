#include <xc.h>
#include "d-ticker.h"

#define ADCON0_POT4	0b00001001
#define ADCON0_POT3	0b00010001
#define ADCON0_POT2	0b00010101
#define ADCON0_POT1	0b00011001

enum {
    POTS_COUNT = 4,
    POTS_MOVE_TOLERANCE = 2,
    POTS_MOVE_TIMEOUT = 200
};
struct {
    volatile byte reading[POTS_COUNT];
    volatile byte cur_pot;
    volatile byte scan_complete;
    volatile byte mov_detect;
    volatile byte mov_done;
    volatile int mov_timeout;    
} pots;


////////////////////////////////////////////////////////////////////////////////
static void read_next() {
	switch(pots.cur_pot) {
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
////////////////////////////////////////////////////////////////////////////////
void pots_read_isr() {
    byte reading = ADRESH;
    if(abs(reading - pots.reading[pots.cur_pot]) >= POTS_MOVE_TOLERANCE) {
        pots.mov_detect = 1;
        pots.mov_timeout = POTS_MOVE_TIMEOUT;
        pots.reading[pots.cur_pot] = reading;            
    }
    if(++pots.cur_pot >= POTS_COUNT) {
        pots.cur_pot = 0;
        pots.scan_complete = 1;
    }
    read_next();
}
////////////////////////////////////////////////////////////////////////////////
void pots_init() {
    pots.cur_pot = 0;
    pots.scan_complete = 0;
    
    read_next();
    while(!pots.scan_complete); // wait for first read of pots
    
    pots.mov_detect = 0;
    pots.mov_done = 0;
    pots.mov_timeout = 0;   
}
////////////////////////////////////////////////////////////////////////////////
inline void pots_ms_isr() {
    if(pots.mov_timeout) {
        if(!--pots.mov_timeout) {
            pots.mov_done = 1;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
inline byte pots_reading(int which) {
    return pots.reading[which];
}
////////////////////////////////////////////////////////////////////////////////
byte pots_mov_detect() {
    byte mov_detect = pots.mov_detect;
    pots.mov_detect = 0;
    return mov_detect;
}
////////////////////////////////////////////////////////////////////////////////
byte pots_mov_done() {
    byte mov_done = pots.mov_done;
    pots.mov_done = 0;
    return mov_done;
}