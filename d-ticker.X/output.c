#include <xc.h>
#include "d-ticker.h"

#define P_CLOCKOUT LATCbits.LATC4

enum {
    OUTPUT_PULSE_MS = 10,
    OUTPUT_PULSE_LOW_MS = 5
};
struct {
    volatile byte pending;
    volatile byte timeout;    
} g_out;

static void start_trig() {
    P_CLOCKOUT = 1;
    g_out.timeout = (OUTPUT_PULSE_MS + OUTPUT_PULSE_LOW_MS);    
}

///////////////////////////////////////////////////////////////////////////////
void out_init() {
    g_out.pending = 0;
    g_out.timeout = 0;
};
///////////////////////////////////////////////////////////////////////////////
inline void out_ms_isr() {
    // manage trig output
    if(g_out.timeout) {
        --g_out.timeout;
        if(g_out.timeout <= OUTPUT_PULSE_LOW_MS) {
            P_CLOCKOUT = 0;
        }
    }
    else if(g_out.pending) {
        start_trig();
        --g_out.pending;
    }
}
///////////////////////////////////////////////////////////////////////////////
void out_trig() {    
    di();
    if(g_out.timeout) {
        ++g_out.pending;
    }
    else {
        start_trig();
    }
    ei();    
}
