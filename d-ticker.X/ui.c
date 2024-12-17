#include <xc.h>
#include "d-ticker.h"

#define P_SWITCH PORTAbits.RA3

enum {
    UI_NUM_PULSES,
    UI_NUM_TRIGS,
    UI_RESET_MODE,
    UI_BPM
};

static const int DEBOUNCE_MS = 20;
static const int DOUBLE_CLICK_MS = 200;
static const int POT_MOVE_TIMEOUT_MS = 1000;

static const byte num_pulses_menu[4] = {4, 8, 16, 32};
static const byte num_trigs_menu[4] = {4, 8, 16, 32};
static const byte reset_mode_menu[4] = { 
    RESET_MODE_RESTART, 
    RESET_MODE_ONE_SHOT, 
    RESET_MODE_RUN, 
    RESET_MODE_RESTART_RUN 
};

static struct {
    byte is_menu;
    byte is_dirty;
    byte button_state;
    int debounce_timeout;
    int double_click_timeout;
    int pot_move_timeout;
} ui;

////////////////////////////////////////////////////////////////////////////////
inline void ui_ms_isr() {
    if(ui.debounce_timeout) {
        --ui.debounce_timeout;
    }
    if(ui.double_click_timeout) {
        --ui.double_click_timeout;
    }
}

////////////////////////////////////////////////////////////////////////////////
void ui_init() {
    ui.is_menu = 0;
    ui.is_dirty = 0;
    ui.button_state = 0;
    ui.debounce_timeout = 0;
    ui.double_click_timeout = 0;
    ui.pot_move_timeout = 0;    
}

////////////////////////////////////////////////////////////////////////////////
void ui_run() {
    
    // deal with the switch state
    if(!ui.debounce_timeout) {
        byte button_state = !P_SWITCH;
        if(button_state != ui.button_state) {
            ui.button_state = button_state;
            ui.debounce_timeout = DEBOUNCE_MS;            
            if(ui.double_click_timeout) {
                // double clicking the button forces a restart 
                seq_set_reset_mode(RESET_MODE_RESTART);
                clk_restart();
                ui.double_click_timeout = 0;
                
            }
            else if(button_state) {
                ui.double_click_timeout = DOUBLE_CLICK_MS;
            }            
        }
    }
    
    // get info from the pots
    byte pot_move_pending = pots_move_pending();
    byte pot_moved = pots_last_moved();        
    int pot_reading = pots_reading(pot_moved);
    if(pot_move_pending) {
        ui.is_dirty = 1;
    }
    
    // MENU MODE
    if(ui.is_menu) {
        byte menu_pos = (byte)(pot_reading/64);
        leds_set_pos(menu_pos,0);
        
        if(ui.is_dirty && !pot_move_pending) {
            // pot has stopped moving so commit the value
            switch(pot_moved) {
                case UI_NUM_PULSES:
                    clk_set_num_pulses(num_pulses_menu[menu_pos]);
                    break;
                case UI_NUM_TRIGS:
                    pat_set_num_trigs(num_trigs_menu[menu_pos]);
                    break;
                case UI_RESET_MODE:
                    seq_set_reset_mode(reset_mode_menu[menu_pos]);
                    break;
                case UI_BPM:
                    clk_set_bpm(15+2*pot_reading);
                    break;
            }
            ui.is_dirty = 0;
            if(!ui.button_state) {
                // button no longer pressed so exit from menu
                leds_clear_pos();
                ui.is_menu = 0;
            }
        }
        
    }
    else if(ui.button_state && pot_move_pending) {
        // pot moved with a button pressed so enter menu mode
        ui.is_menu = 1;
    }
    else {
        // normal running mode
        int cur_trig = set_get_output_trig();
        if(cur_trig >= 0) {
            leds_set_pos((byte)((4*cur_trig)/pat_get_num_trigs()), 
                (cur_trig%4) ? SHORT_LED_BLINK_MS : MED_LED_BLINK_MS
            );
        }
        if(ui.is_dirty && pot_move_pending) {
            // recalculate the pattern when pot stops moving
            pat_recalc();
            ui.is_dirty = 0;            
        }
    }      
}
