#include <xc.h>
#include "d-ticker.h"

#define P_CLOCKLED LATAbits.LATA1


const unsigned long long MAX_TICKS = ((unsigned long long)1<<32);
const int MIN_EXT_PERIOD_MS = 100;
const int MAX_EXT_PERIOD_MS = 3000;
struct {
    int bpm;                            // total number of pulses in the pattern
    unsigned long cur_ticks;
    unsigned long ticks_per_step;
    unsigned long ticks_per_ms;
    unsigned long ticks_at_next_step;
    unsigned int ms_since_ext_clock;
    byte pending_restart;
    byte is_external_clock;
    byte is_restart;
} clk;




//////////////////////////////////////////////////////////
static void recalc() {
    // calculate the tick increment per ms
    double ms_per_step = ((60.0 * 1000.0)/clk.bpm);    
    clk.ticks_per_ms = (unsigned long)(0.5 + clk.ticks_per_step / ms_per_step);    
}

//////////////////////////////////////////////////////////
// called by interrupt when a rising ext clock edge is received 
inline void clk_ext_pulse_isr() {
    
    // currently on internal clock?
    if(!clk.is_external_clock) {
        // select external clock and flag a restart
        clk.is_external_clock = 1;
        clk.pending_restart = 1;
    }
    
    // restart is pending?
    if(clk.pending_restart) {
        clk.pending_restart = 0;
        clk.is_restart = 1;
        clk.cur_ticks = 0;
        clk.ticks_at_next_step = clk.ticks_per_step;
    }
    else 
    {
        // move the "step window" within which the internal
        // clock can run
        clk.cur_ticks = clk.ticks_at_next_step;
        clk.ticks_at_next_step += clk.ticks_per_step;
        if(clk.ticks_at_next_step < clk.ticks_per_step) {
            // rollover back to start of bar
            clk.ticks_at_next_step = 0;
        }

        // adjust the automatic tick increment to approximate the step rate
        if(clk.ms_since_ext_clock >= MIN_EXT_PERIOD_MS &&
           clk.ms_since_ext_clock <= MAX_EXT_PERIOD_MS) {
            clk.ticks_per_ms = clk.ticks_per_step / clk.ms_since_ext_clock;
        }        
    }
    
    // get ready to time the interval to the next pulse
    clk.ms_since_ext_clock = 0;
}

////////////////////////////////////////////////////////////////////////////////
// called every ms by interrupt
inline void clk_ms_isr() {
    if(!clk.is_external_clock && clk.pending_restart) {
        // perform a pending reset 
        clk.pending_restart = 0;
        clk.is_restart = 1;
        clk.cur_ticks = 0;
    }
    else 
    {
        // otherwise advance the tick counter, noting it will roll over to 0
        // at the end of the bar
        unsigned long next_ticks = (clk.cur_ticks + clk.ticks_per_ms); 
        if(!clk.is_external_clock || (next_ticks < clk.ticks_at_next_step)) {
            // when on external clock, don't allow advancement past the the 
            // start of the next step
            clk.cur_ticks = next_ticks;
        }
    }
    ++clk.ms_since_ext_clock;
}

//////////////////////////////////////////////////////////
void clk_init() {    
    clk.bpm = 120;
    clk.cur_ticks = 0;
    clk.ticks_per_step = 0;
    clk.ticks_per_ms = 0;
    clk.ticks_at_next_step = 0;
    clk.pending_restart = 0;
    clk.is_restart = 0;
    clk.is_external_clock = 0;
    clk.ms_since_ext_clock = 0;
    clk_set_num_steps(16);
}

//////////////////////////////////////////////////////////
void clk_do_restart() {
    clk.pending_restart = 1;
}

//////////////////////////////////////////////////////////
inline byte clk_is_restart() {
    byte is_restart = clk.is_restart;
    clk.is_restart = 0;
    return is_restart;
    return 0;
}
//////////////////////////////////////////////////////////
// return value is 0 .. 65535
inline unsigned int clk_get_cur_pos() {
    return (unsigned int)(clk.cur_ticks >> 16);
}
//////////////////////////////////////////////////////////
inline int clk_get_cur_step() {
    return (int)(clk.cur_ticks / clk.ticks_per_step);
}
//////////////////////////////////////////////////////////
void clk_set_num_steps(int num_steps) {
    clk.ticks_per_step = MAX_TICKS/num_steps;
    recalc();
}
//////////////////////////////////////////////////////////
void clk_set_bpm(int bpm) {
    clk.bpm = bpm;    
    clk.is_external_clock = 0;
    recalc();    
}
