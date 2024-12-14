/* 
 * File:   d-ticker.h
 * Author: jason
 *
 * Created on 13 December 2024, 15:49
 */

#ifndef D_TICKER_H
#define	D_TICKER_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef unsigned char byte;
typedef unsigned int TCOUNT;

enum {
    CLK_STEP1 = 0x01,
    CLK_STEP4 = 0x02,
    CLK_RESET = 0x04
};

inline void clk_ms(void);
void clk_ext_pulse(void);
void clk_init(void);
void clk_reset(void);
inline unsigned int clk_get_pos(void);
inline byte clk_get_event(void);
void clk_set_internal(void);
void clk_set_length(int length);
int clk_get_length(void);
void clk_set_bpm(int bpm);
int clk_get_bpm(void);



#ifdef	__cplusplus
}
#endif

#endif	/* D_TICKER_H */

