#include <xc.h>
#include "d-ticker.h"

enum {
    MAX_TRIGS = 32
};
struct {
    unsigned int trig[MAX_TRIGS];
    int num_trigs;
} g_pat;

/////////////////////////////////////////////////////////////////////////////
void pat_set_num_trigs(int num_trigs) {
    g_pat.num_trigs = num_trigs;
}
/////////////////////////////////////////////////////////////////////////////
inline int pat_get_num_trigs() {
    return g_pat.num_trigs;
}
/////////////////////////////////////////////////////////////////////////////
inline unsigned int pat_get_trig(int pos) {
    return g_pat.trig[pos];
}
/////////////////////////////////////////////////////////////////////////////
void pat_init() {
    for(int i=0; i<MAX_TRIGS; ++i) {
        g_pat.trig[i] = 0;
    }
    g_pat.num_trigs = 32;
}

/////////////////////////////////////////////////////////////////////////////
// Recalculate the tempo map
/////////////////////////////////////////////////////////////////////////////
void pat_recalc() {
    int trig[MAX_TRIGS];
    
    // expand out the velocity(tempo) changes (defined by the pots) into a 
    // list of velocity values at each output trigger position    
    int cur_rate = 128;
    int min_rate = 128;
    for(int i=0; i<g_pat.num_trigs; ++i) {
        trig[i] = cur_rate;        
        int acc = 128-pots_reading((i*4)/g_pat.num_trigs);
        cur_rate += acc;
        if(cur_rate < min_rate) {
            min_rate = cur_rate;
        }        
    }
    
    // normalise the velocities so that they are all positive and 
    // expand out the "distance into sequence" by integrating velocity
    int dist = 0;
    for(int i=0; i<g_pat.num_trigs; ++i) {
        int norm_rate = trig[i] - min_rate + 128;
        trig[i] = dist;
        dist = dist + norm_rate;
    }
    
    // now normalise the distances so that they run from 0 - 65535
    for(int i=0; i<g_pat.num_trigs; ++i) {
        g_pat.trig[i] = (unsigned int)(((long)65535 * trig[i]) / dist);
    }
}

