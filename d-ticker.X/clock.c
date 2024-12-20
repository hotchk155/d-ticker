#include <xc.h>
#include "d-ticker.h"

#define P_CLOCKLED LATAbits.LATA1

const double MAX_TICKS = 65535.0;
const int LEADING_CLOCK_TIMEOUT_MS = 5;    
const int MIN_EXT_PERIOD_MS = 10;
const int MAX_EXT_PERIOD_MS = 3000;
struct {
    int bpm;                            // total number of pulses in the pattern
    double cur_ticks;
    double ticks_at_next_step;
    double ticks_per_step;
    double ticks_per_ms;
    unsigned int ms_since_ext_clock;
    unsigned int ms_leading_clock_timeout;
    byte pending_restart;       // restart at the next clock pulse
    byte is_external_clock;
    byte is_rollover;
    byte is_restart;
} clk;




//////////////////////////////////////////////////////////
static void recalc() {
    // calculate the tick increment per ms
    double ms_per_step = ((60.0 * 1000.0)/clk.bpm);    
    clk.ticks_per_ms = clk.ticks_per_step / ms_per_step;    
}

/*
 Normally we want to action a pending reset at the next clock pulse, however
 the clock and reset signals may be "simultaneous" from the clock master..
 In this case we'll register one before the other
 
 If the clock is seen before the reset, we have a problem since the reset is 
 delayed by a whole clock step
 
 Therefore is we see a reset within a very short 
 i.e.
 
  X  clock
 X reset 
 ..........or
 X  clock
  X reset 
 
 in the first case we can 
 
 */

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
        // move the "step window" within which the internal clock can run
        double ticks_at_next_step = clk.ticks_at_next_step + clk.ticks_per_step;
        if(ticks_at_next_step >= MAX_TICKS) {  
            clk.is_rollover = 1;
            clk.cur_ticks = 0.0;
            clk.ticks_at_next_step = clk.ticks_per_step; // rollover
            
        }
        else {
            clk.cur_ticks = clk.ticks_at_next_step;
            clk.ticks_at_next_step = ticks_at_next_step;
        }

        // adjust the automatic tick increment to approximate the step rate
        if(clk.ms_since_ext_clock >= MIN_EXT_PERIOD_MS &&
           clk.ms_since_ext_clock <= MAX_EXT_PERIOD_MS) {
            clk.ticks_per_ms = clk.ticks_per_step / clk.ms_since_ext_clock;
        }        
    }
    
    // get ready to time the interval to the next pulse
    clk.ms_since_ext_clock = 0;
    clk.ms_leading_clock_timeout = LEADING_CLOCK_TIMEOUT_MS;
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
        double next_ticks = clk.cur_ticks + clk.ticks_per_ms;
        if(next_ticks > MAX_TICKS) {
            next_ticks -= MAX_TICKS;
        }
        if(!clk.is_external_clock || next_ticks < clk.ticks_at_next_step) {
            clk.cur_ticks = next_ticks;
        }        
    }
    ++clk.ms_since_ext_clock;
    if(clk.ms_leading_clock_timeout) {
        --clk.ms_leading_clock_timeout;
    }
}

//////////////////////////////////////////////////////////
inline void clk_ext_restart_isr() {
    if(clk.ms_leading_clock_timeout) {
        clk.pending_restart = 0;
        clk.is_restart = 1;
        clk.cur_ticks = 0;
        clk.ticks_at_next_step = clk.ticks_per_step;        
    }
    else {
        clk.pending_restart = 1;
    }
}

//////////////////////////////////////////////////////////
void clk_manual_restart() {
    clk.pending_restart = 1;    
}

//////////////////////////////////////////////////////////
void clk_init() {    
    clk.bpm = 120;
    clk.cur_ticks = 0.0;
    clk.ticks_per_step = 0.0;
    clk.ticks_per_ms = 0.0;
    clk.ticks_at_next_step = 0.0;
    clk.pending_restart = 0;
    clk.is_restart = 0;
    clk.is_rollover = 0;
    clk.is_external_clock = 0;
    clk.ms_since_ext_clock = 0;
    clk.ms_leading_clock_timeout = 0;
    clk_set_num_steps(16);
}

//////////////////////////////////////////////////////////
inline byte clk_is_restart() {
    di();
    byte is_restart = clk.is_restart;
    clk.is_restart = 0;
    ei();
    return is_restart;
}
//////////////////////////////////////////////////////////
inline byte clk_is_rollover() {
    di();
    byte is_rollover = clk.is_rollover;
    clk.is_rollover = 0;
    ei();
    return is_rollover;
}
//////////////////////////////////////////////////////////
// return value is 0 .. 65535
inline unsigned int clk_get_cur_pos() {
    static unsigned int last_pos;
     unsigned int pos;
    if(clk.cur_ticks >= MAX_TICKS) {
        pos = (unsigned int)MAX_TICKS;
    }
    else {
        pos =  (unsigned int)(0.5+clk.cur_ticks);
    }
     if(pos < last_pos && !pos) {
          leds_set_clock(1, MED_LED_BLINK_MS);         
     }
     last_pos = pos;
     return pos;
}
//////////////////////////////////////////////////////////
inline int clk_get_cur_step() {
    return (int)(0.5 + clk.cur_ticks / clk.ticks_per_step);
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
