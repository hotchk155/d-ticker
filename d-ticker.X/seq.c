#include <xc.h>
#include "d-ticker.h"

static struct {
    byte reset_signal;
} seq;

void seq_init() {
    seq.reset_signal = 0;
}

void seq_reset_signal_isr(byte reset_signal) {
    seq.reset_signal = reset_signal;    
}

void seq_run() {
    pat_recalc();
    
    // main application loop
    int cur_trig = 0;
    unsigned int pos = 0;
	for(;;) {
        
        // run the UI
        ui_run();
        
        // get the updated clock position
        unsigned int new_pos = clk_get_pos();
        byte event = clk_get_event();
        if(event & CLK_STEP4) {                    
            leds_set_clock(1, MED_LED_BLINK_MS);
        }
        else if(event & CLK_STEP1) {                    
            leds_set_clock(1, SHORT_LED_BLINK_MS);
        }
        if(event & CLK_RESTART) { // reset signal
            // reset the pattern ignoring remaining trigs
            cur_trig = 0;
        }            
        else if(new_pos < pos) { // wrap around to start of pattern
            // send any remaining triggers
            while(cur_trig++ < pat_get_num_trigs()) {                   
                out_trig();
            }                
            cur_trig = 0;
        }
        pos = new_pos;

        
        while(cur_trig < pat_get_num_trigs()) {
            if(pat_get_trig(cur_trig) > pos) {
                break;
            }
            ++cur_trig;
            out_trig(); 
            ui_trig((byte)((4*cur_trig)/pat_get_num_trigs()));
        }
	}
}
