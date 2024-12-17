#ifndef D_TICKER_H
#define	D_TICKER_H

typedef unsigned char byte;



enum byte {
    RESET_MODE_RESTART,
    RESET_MODE_ONE_SHOT,
    RESET_MODE_RUN,
    RESET_MODE_RESTART_RUN
};
enum {    
    MAX_INPUT_STEPS = 16,
    MAX_OUTPUT_RATE = 4,
    MAX_TRIGS = (MAX_INPUT_STEPS * MAX_OUTPUT_RATE)
};
enum {
    TINY_LED_BLINK_MS = 1,
    SHORT_LED_BLINK_MS = 5,
    MED_LED_BLINK_MS = 30,
    LONG_LED_BLINK_MS = 50
};
enum {
    CLK_PULSE,
    CLK_RESTART
};
////////////////////////////////////////////////////////////////////////////////
inline void clk_ms_isr(void);
inline void clk_ext_pulse_isr(void);
void clk_restart(void);
void clk_init(void);
inline unsigned int clk_get_pos(void);
inline byte clk_event(void);
void clk_set_num_pulses(int num_pulses);
void clk_set_bpm(int bpm);

////////////////////////////////////////////////////////////////////////////////
void pat_set_num_trigs(int num_trigs);
inline int pat_get_num_trigs(void);
inline unsigned int pat_get_trig(int pos);
void pat_init(void);
void pat_recalc(void);

////////////////////////////////////////////////////////////////////////////////
void pots_read_isr(void);
void pots_init(void);
inline void pots_ms_isr(void);
inline byte pots_reading(int which);
inline byte pots_last_moved(void);
inline byte pots_move_pending(void);

////////////////////////////////////////////////////////////////////////////////
void leds_init(void);
inline void leds_ms_isr(void);
inline void leds_set_clock(byte state, byte timeout);
inline void leds_set_pos(byte which, byte timeout);
inline void leds_clear_pos(void);

///////////////////////////////////////////////////////////////////////////////
void out_init(void);
inline void out_ms_isr(void);
void out_trig(void);

///////////////////////////////////////////////////////////////////////////////
void ui_init(void);
inline void ui_ms_isr(void);
void ui_run(void);
void ui_trig(byte which);

////////////////////////////////////////////////////////////////////////////////
void seq_init(void);
void seq_reset_signal_isr(byte reset_signal);
void seq_run(void);
int set_get_output_trig(void);
void seq_set_reset_mode(byte reset_mode);


#endif	/* D_TICKER_H */

