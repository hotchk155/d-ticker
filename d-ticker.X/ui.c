#include <xc.h>
#include "d-ticker.h"

#define P_SWITCH PORTAbits.RA3

enum {
    UI_RUN,
    UI_SET_LENGTH,
    UI_SET_MULT,
    UI_SET_RESET,
    UI_SET_BPM
};
enum {
    INPUT_STEPS_4,
    INPUT_STEPS_8,
    INPUT_STEPS_16,
    INPUT_STEPS_32
};
enum {
    OUTPUT_RATE_DIV2,
    OUTPUT_RATE_X1,
    OUTPUT_RATE_X2,
    OUTPUT_RATE_X4
};
enum {
    RESET_MODE_RESTART,
    RESET_MODE_ONE_SHOT,
    RESET_MODE_RUN,
    RESET_MODE_RESTART_RUN
};

static struct {
    byte button_state;
} ui;
void ui_init() {
    ui.button_state = 0;
}
void ui_run() {
    //byte button_state = !P_SWITCH;
    //if()
    //    if(pots_mov_done()) {
    //        pat_recalc();
    //    }
    //
}
void ui_trig(byte which) {
    leds_set_pos(which,SHORT_LED_BLINK_MS);     
}