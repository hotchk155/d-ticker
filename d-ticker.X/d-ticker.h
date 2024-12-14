#ifndef D_TICKER_H
#define	D_TICKER_H

/*

1  VDD
2  RA5				CLOCK_IN        exLED1
3  RA4/SDO			RESET_IN	
4  RA3/MCLR#/VPP	SWITCH
5  RC5/RX			LED1            exCLOCK_IN
6  RC4/TX			CLOCK_OUT
7  RC3/SS#			LED2
8  RC2				POT1/AN6
9  RC1/SDA/SDI		POT2/AN5
10 RC0/SCL/SCK		POT3/AN4
11 RA2/INT			POT4/AN2
12 RA1/ICPCLK		LED0
13 RA1/ICPDAT		LED_COM
14 VSS

RA2	POT4	AN2
RC0	POT3	AN4
RC1	POT2	AN5
RC2	POT1	AN6
*/
#define ADCON0_POT4	0b00001001
#define ADCON0_POT3	0b00010001
#define ADCON0_POT2	0b00010101
#define ADCON0_POT1	0b00011001

//
// TYPE DEFS
//
//typedef unsigned char byte;

#define P_CLOCKLED LATAbits.LATA1
#define P_CLOCKOUT LATCbits.LATC4

#define P_LED1 LATCbits.LATC5
#define P_LED2 LATCbits.LATC3
#define P_LEDCOM LATAbits.LATA0

#define T_LED1 TRISCbits.TRISC5
#define T_LED2 TRISCbits.TRISC3
#define T_LEDCOM TRISAbits.TRISA0

#define P_EXTCLOCK PORTAbits.RA5
#define P_EXTRESET PORTAbits.RA4
#define P_SWITCH PORTAbits.RA3

#define WPUA_BITS 0b00001000

#define IOCAN_BITS 0b00110000
#define IOCAP_BITS 0b00000000
#define IOCAF_EXTCLOCK IOCAFbits.IOCAF5
#define IOCAF_EXTRESET IOCAFbits.IOCAF4

//               76543210
#define TRIS_A 0b11111101
#define TRIS_C 0b11101111

//
// MACRO DEFS
//

// Timer related stuff
#define TIMER_0_INIT_SCALAR		5		// Timer 0 initialiser to overlow at 1ms intervals

typedef unsigned char byte;





enum {
    CLK_STEP1 = 0x01,
    CLK_STEP4 = 0x02,
    CLK_RESET = 0x04
};

inline void clk_ms_isr(void);
void clk_ext_pulse_isr(void);
void clk_init(void);
void clk_reset_isr(void);
inline unsigned int clk_get_pos(void);
inline byte clk_get_event(void);
void clk_set_internal(void);
void clk_set_length(int length);
int clk_get_length(void);
void clk_set_bpm(int bpm);
int clk_get_bpm(void);

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
byte pots_mov_detect(void);
byte pots_mov_done(void);

////////////////////////////////////////////////////////////////////////////////
void leds_init(void);
inline void leds_ms_isr(void);
inline void leds_set_clock(byte state, byte timeout);
inline void leds_clear_pos(void);
inline void leds_set_pos(byte which, byte timeout);


///////////////////////////////////////////////////////////////////////////////
void out_init(void);
inline void out_ms_isr(void);
void out_trig(void);

#endif	/* D_TICKER_H */

