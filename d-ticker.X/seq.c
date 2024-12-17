#include <xc.h>
#include "d-ticker.h"

static struct {
    byte reset_mode;
    byte reset_edge;
    byte reset_state;
    byte output_enabled;
    int output_trig;
} seq;

////////////////////////////////////////////////////////////////////////////////
void seq_init() {    
    seq.reset_mode = RESET_MODE_RESTART;
    seq.reset_state = 1;
    seq.output_enabled = 1;
    seq.output_trig = -1;
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
                clk_restart();
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
int set_get_output_trig() {
    int output_trig = seq.output_trig;
    seq.output_trig = -1;
    return output_trig;
}
////////////////////////////////////////////////////////////////////////////////
void seq_run() {
    pat_recalc();
    
    int cur_pulse = 0;
    int cur_trig = 0;
    unsigned int cur_pos = 0;
	for(;;) {
        
        // run the UI
        ui_run();

        // get the updated clock position
        unsigned int new_pos = clk_get_pos();        
        switch(clk_event()) {
            case CLK_RESTART:
                cur_pos = 0;
                cur_pulse = 0;
                cur_trig = 0;
                leds_set_clock(1, LONG_LED_BLINK_MS);
                break;
            case CLK_PULSE:                    
                cur_pulse = (cur_pulse + 1)&3;
                leds_set_clock(1, cur_pulse ? SHORT_LED_BLINK_MS : MED_LED_BLINK_MS);
                break;
        }
        if(new_pos < cur_pos) { // wrap around to start of pattern
            // send any remaining triggers
            while(cur_trig++ < pat_get_num_trigs()) {                   
                out_trig();
            }                
            cur_trig = 0;
        }
        cur_pos = new_pos;

        while(cur_trig < pat_get_num_trigs()) {
            if(pat_get_trig(cur_trig) > cur_pos) {
                break;
            }
            ++cur_trig;
            if(seq.output_enabled) {
                out_trig(); 
                seq.output_trig = cur_trig;
            }
        }
	}
}
