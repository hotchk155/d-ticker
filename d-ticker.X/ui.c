#include <xc.h>
#include "d-ticker.h"

#define P_SWITCH PORTAbits.RA3


// number of pulses corresponding to each menu option
static const byte num_pulses_menu[4] = {
    4, 
    8, 
    16, 
    32
};

// number of trigs corresponding to each menu option
static const byte num_trigs_menu[4] = {
    4, 
    8, 
    16, 
    32
};

// reset modss mapped to the menu
static const byte reset_mode_menu[4] = { 
    RESET_MODE_RESTART, 
    RESET_MODE_ONE_SHOT, 
    RESET_MODE_RUN, 
    RESET_MODE_RESTART_RUN 
};
enum {
    UI_NUM_PULSES,
    UI_NUM_TRIGS,
    UI_RESET_MODE,
    UI_BPM,
    UI_PATTERN
};

static const int DEBOUNCE_MS = 20;
static const int DOUBLE_CLICK_MS = 200;
static const int POT_MOVE_TIMEOUT_MS = 1000;

static struct {
    volatile byte mode;                      // are we in a "menu"
    volatile byte pot_move_done;                  // has a pot been moved but not actioned?
    volatile byte button_state;              // is button pressed?
    volatile int debounce_timeout;           // counter for debouncing button
    volatile int double_click_timeout;       // counter for timing double click
    volatile int pot_move_timeout;       
} ui;


////////////////////////////////////////////////////////////////////////////////
void ui_init() {
    ui.mode = UI_PATTERN;
    ui.button_state = 0;
    ui.debounce_timeout = 0;
    ui.double_click_timeout = 0;
    ui.pot_move_timeout = 0;
    ui.pot_move_done = 0;
}

////////////////////////////////////////////////////////////////////////////////
void xui_run() {
    
    // deal with the switch state
    if(!ui.debounce_timeout) {
        byte button_state = !P_SWITCH;
        if(button_state != ui.button_state) {
            ui.button_state = button_state;
            ui.debounce_timeout = DEBOUNCE_MS;            
            if(ui.double_click_timeout) {
                // double clicking the button forces a restart 
                seq_set_reset_mode(RESET_MODE_RESTART);
//                clk_restart();
                ui.double_click_timeout = 0;
                
            }
            else if(button_state) {
                ui.double_click_timeout = DOUBLE_CLICK_MS;
            }            
        }
    }
    
    // has a pot moved since the last call?
    int cur_pot = pots_moved();
    if(cur_pot >= 0) {
        ui.pot_move_timeout = POT_MOVE_TIMEOUT_MS;
    }
    
    // are we in the normal running mode?
    if(ui.mode == UI_PATTERN) {
                
        if(ui.button_state && cur_pot >= 0) {
            // pot moved with a button held so enter the menu mode
            ui.mode = (byte)cur_pot;
        }
        else {
            // normal running mode
            int cur_trig = seq_get_output_trig();
            if(cur_trig >= 0) {
                leds_set_pos((byte)((4*cur_trig)/pat_get_num_trigs()), 
                    (cur_trig%4) ? SHORT_LED_BLINK_MS : MED_LED_BLINK_MS
                );
            }
            // recalculate the pattern if a pot moves
            if(ui.pot_move_done) {
                ui.pot_move_done = 0;
                pat_recalc();
            }
        }      
    }
    else {
        // read the pot for this menu
        byte pot_reading = pots_reading(ui.mode);
        byte option = pot_reading/64;
        leds_set_pos(option,0);
        
        // has a pot been moved?
        if(ui.pot_move_done) {
            ui.pot_move_done = 0;
            
            // commit the value
            switch(ui.mode) {
                case UI_NUM_PULSES:
                    clk_set_num_pulses(num_pulses_menu[option]);
                    break;
                case UI_NUM_TRIGS:
                    pat_set_num_trigs(num_trigs_menu[option]);
                    break;
                case UI_RESET_MODE:
                    seq_set_reset_mode(reset_mode_menu[option]);
                    break;
                case UI_BPM:
                    clk_set_bpm(15+2*pot_reading);
                    break;
            }

            // when button is released, exit back to pattern mode
            if(!ui.button_state) {
                leds_clear_pos();
                ui.mode = UI_PATTERN;
            }
        }
    }
}



////////////////////////////////////////////////////////////////////////////////
void ui_run() {
    if(ui.debounce_timeout) {
        --ui.debounce_timeout;
    }
    if(ui.double_click_timeout) {
        --ui.double_click_timeout;
    }
    if(ui.pot_move_timeout) {
        if(!--ui.pot_move_timeout) {
            ui.pot_move_done = 1;
        }
    }    
    // has a pot moved since the last call?
    int cur_pot = pots_moved();
    if(cur_pot >= 0) {
        ui.pot_move_done = 0;
        ui.pot_move_timeout = POT_MOVE_TIMEOUT_MS;
    }
    // recalculate the pattern if a pot moves
    if(ui.pot_move_done) {
        ui.pot_move_done = 0;
        pat_recalc();
    }
}
