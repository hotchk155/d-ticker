#include <xc.h>
#include "d-ticker.h"

enum {
    UI_RUN,
    UI_SET_LENGTH,
    UI_SET_MULT,
    UI_SET_RESET,
    UI_SET_BPM
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