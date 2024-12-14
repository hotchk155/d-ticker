#include <xc.h>
#include "d-ticker.h"

const double MAX_TICKS = 65535.0;
const double MAX_EXT_PERIOD_MS = 3000.0;
struct {
    int bpm;                            // total number of pulses in the pattern
    int num_pulses;                     // total number of pulses in the pattern    
    int cur_pulse;                      // 
    
    double cur_ticks;            // the current running tick count
    double ticks_at_next_pulse;  // the number of ticks at the next pulse
    double ticks_per_pulse;      // increment per ext clock pulse
    double ticks_per_ms;         // increment per ms to current tick count
    
    unsigned long cur_ms;        // 
    unsigned long last_ext_ms;        // 

    byte ext_clk_present;
    byte event; 
} clk;

//////////////////////////////////////////////////////////
static void recalc() {
    clk.ticks_per_pulse = MAX_TICKS/clk.num_pulses;
    double ms_per_pulse = ((60.0 * 1000.0)/clk.bpm);    
    clk.ticks_per_ms = clk.ticks_per_pulse / ms_per_pulse;
}

////////////////////////////////////////////////////////////////////////////////
inline void clk_ms() {
    ++clk.cur_ms;
    if(clk.ext_clk_present) {
        // running on external clock... tick along at the appropriate
        // rate but do not pass the threshold of the next expected 
        // clock pulse
        clk.cur_ticks += clk.ticks_per_ms;
        if(clk.cur_ticks > clk.ticks_at_next_pulse) {
            clk.cur_ticks = clk.ticks_at_next_pulse;
        }
    }
    else {
        // running on the internal clock
        clk.cur_ticks += clk.ticks_per_ms;
        if(clk.cur_ticks >= MAX_TICKS) {
            clk.cur_ticks -= MAX_TICKS;
            clk.cur_pulse = 0;
            clk.ticks_at_next_pulse = clk.ticks_per_pulse;
            clk.event |= CLK_STEP4;
        }
        else if(clk.cur_ticks >= clk.ticks_at_next_pulse) {
            ++clk.cur_pulse;
            clk.ticks_at_next_pulse += clk.ticks_per_pulse;
            clk.event |= ((clk.cur_pulse%4)? CLK_STEP1:CLK_STEP4);
        }
    }
    
}
//////////////////////////////////////////////////////////
// called when a clock pulse is received
void clk_ext_pulse() {
    if(!clk.ext_clk_present) {
        // we were on internal clock, so switch to external
        // clock and restart the sequence
        clk_reset();
    }
    else {
        clk.ticks_at_next_pulse += clk.ticks_per_pulse;
        ++clk.cur_pulse;
        clk.event |= ((clk.cur_pulse%4)? CLK_STEP1:CLK_STEP4);
    }
    
    if(clk.last_ext_ms) {
        double ms_per_pulse = (clk.cur_ms - clk.last_ext_ms);
        if(ms_per_pulse > MAX_EXT_PERIOD_MS) {
            ms_per_pulse = MAX_EXT_PERIOD_MS;
        }
        clk.ticks_per_ms = clk.ticks_per_pulse / ms_per_pulse;
    }
    clk.last_ext_ms = clk.cur_ms;
}
//////////////////////////////////////////////////////////
void clk_init() {    
    clk.bpm = 120;
    clk.num_pulses = 16;    
    clk.ext_clk_present = 0; // start on internal clock
    clk.event = 0;
    recalc();
    clk_reset();
}
//////////////////////////////////////////////////////////
void clk_reset() {
    clk.cur_ticks = 0.0;
    clk.cur_pulse = 0;
    clk.cur_ms = 0;
    clk.ticks_at_next_pulse = clk.ticks_per_pulse;    
    clk.event = CLK_RESET;
}

//////////////////////////////////////////////////////////
inline unsigned int clk_get_pos() {
    unsigned long x = (unsigned long)clk.cur_ticks;
    return(x > 65535)? 65535 : (unsigned int)x;
}
//////////////////////////////////////////////////////////
inline byte clk_get_event() {
    byte event = clk.event;
    clk.event = 0;
    return event;
}
//////////////////////////////////////////////////////////
void clk_set_internal() {
    clk.ext_clk_present = 0;    
    recalc();
}
//////////////////////////////////////////////////////////
void clk_set_length(int length) {
    clk.num_pulses = length;
    recalc();
}
//////////////////////////////////////////////////////////
int clk_get_length() {
    return clk.num_pulses;
}
//////////////////////////////////////////////////////////
void clk_set_bpm(int bpm) {
    clk.bpm = bpm;    
    recalc();    
}
////////////////////////////////////////////////////////////////////////////////
int clk_get_bpm() {
    return clk.bpm;
}
