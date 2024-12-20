#include <xc.h>
#include "d-ticker.h"

static struct {
    int cur_trig;
    int prev_step;
    unsigned int prev_pos;
    volatile byte reset_state;
    volatile byte reset_mode;
    volatile byte output_enabled;
    volatile int output_trig;
} seq;

////////////////////////////////////////////////////////////////////////////////
void seq_init() {    
    seq.reset_mode = RESET_MODE_RESTART;
    seq.reset_state = 1;
    seq.output_enabled = 1;
    seq.output_trig = -1;
    seq.prev_pos = 0;
    seq.prev_step = 0;
    seq.cur_trig = 0;
}

////////////////////////////////////////////////////////////////////////////////
void seq_reset_signal_isr(byte reset_signal) {
    seq.reset_state = reset_signal;
    if(reset_signal) { // rising edge
        seq.output_enabled = 1;
        switch(seq.reset_mode){
            case RESET_MODE_RESTART:
            case RESET_MODE_ONE_SHOT:
            case RESET_MODE_RESTART_RUN:
                clk_do_restart();
                break;
            case RESET_MODE_RUN:
                break;
        }
    }
    else {
        switch(seq.reset_mode){
            case RESET_MODE_RESTART:
            case RESET_MODE_ONE_SHOT:
                break;
            case RESET_MODE_RESTART_RUN:
            case RESET_MODE_RUN:
                seq.output_enabled = 0;
                break;
        }        
    }
}

////////////////////////////////////////////////////////////////////////////////
void seq_set_reset_mode(byte reset_mode) {
    seq.reset_mode = reset_mode;
    switch(seq.reset_mode){
        case RESET_MODE_RESTART:
        case RESET_MODE_ONE_SHOT:
            break;
        case RESET_MODE_RESTART_RUN:
        case RESET_MODE_RUN:
            seq.output_enabled = seq.reset_state;
            break;
    }        
}

////////////////////////////////////////////////////////////////////////////////
int seq_get_output_trig() {
    int output_trig = seq.output_trig;
    seq.output_trig = -1;
    return output_trig;
}
////////////////////////////////////////////////////////////////////////////////
void seq_run() {
    // fetch the current position in the pattern (0-65535)
    unsigned int new_pos = clk_get_cur_pos();
    
    // check if the clock has been restarted
    if(clk_is_restart()) {
        leds_set_clock(1, LONG_LED_BLINK_MS);
        seq.cur_trig = 0;        
    }
    // check if the clock has rolled around
    else if(new_pos < seq.prev_pos) {
        if(seq.output_enabled) {
            // action any remaining trigs
            while(seq.cur_trig++ < pat_get_num_trigs()) {                   
                out_trig();
            }                
        }
        leds_set_clock(1, MED_LED_BLINK_MS);
        seq.cur_trig = 0;
    }
    else {
        while(seq.cur_trig < pat_get_num_trigs()) {
            if(pat_get_trig(seq.cur_trig) > new_pos) {
                break;
            }
            if(seq.output_enabled) {
                out_trig(); 
                leds_set_pos((byte)((4*seq.cur_trig)/pat_get_num_trigs()), SHORT_LED_BLINK_MS);
            }
            ++seq.cur_trig;
        }
 
        int cur_step = clk_get_cur_step();
        if(cur_step != seq.prev_step) {
            //leds_set_clock(1, (cur_step%4) ? SHORT_LED_BLINK_MS : MED_LED_BLINK_MS);
            seq.prev_step = cur_step;
        }
    }
    seq.prev_pos = new_pos;
}
