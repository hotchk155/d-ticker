#include <xc.h>
#include "uart_debug.h"

////////////////////////////////////////////////////////////
// INITIALISE SERIAL PORT FOR MIDI
void uart_init()
{
    PIR1bits.TXIF = 0;
    PIR1bits.RCIF = 0;
    PIE1bits.TXIE = 0;
    PIE1bits.RCIE = 1;

    BAUDCONbits.SCKP = 0;//synchronous bit polarity 
    BAUDCONbits.BRG16 = 0;// enable 16 bit brg
    BAUDCONbits.WUE = 0;// wake up enable off
    BAUDCONbits.ABDEN = 0;// auto baud detect

    TXSTAbits.TX9 = 0;// 8 bit transmission
    TXSTAbits.TXEN = 1;// transmit enable
    TXSTAbits.SYNC = 0;// async mode
    TXSTAbits.SENDB = 0;// break character
    TXSTAbits.BRGH = 0;// high baudrate 
    TXSTAbits.TX9D = 0;// bit 9

    RCSTAbits.SPEN = 1;// serial port enable
    RCSTAbits.RX9 = 0;// 8 bit operation
    RCSTAbits.SREN = 0;// enable receiver
    RCSTAbits.CREN = 0;// continuous receive enable

    SPBRGH = 0;// brg high byte
    SPBRG = 25;// brg low byte 
}

void uart_send(byte ch) 
{
	TXREG = ch;
	while(!TXSTAbits.TRMT);
}

void uart_send_string(byte *ch) 
{
	while(*ch) {
		uart_send(*ch);
		++ch;
	}
}

void uart_send_number(int ch) {
	if(ch<0) {
		uart_send('-');
		ch=-ch;
	}
	uart_send('0' + ch/10000);
	ch %= 10000;
	uart_send('0' + ch/1000);
	ch %= 1000;
	uart_send('0' + ch/100);
	ch %= 100;
	uart_send('0' + ch/10);
	ch %= 10;
	uart_send('0' + ch);
}

void uart_send_long(long ch) {
	if(ch<0) {
		uart_send('-');
		ch=-ch;
	}
	uart_send('0' + ch/10000000L);
	ch %= 10000000L;
	uart_send('0' + ch/1000000L);
	ch %= 1000000L;
	uart_send('0' + ch/100000L);
	ch %= 100000L;
	uart_send('0' + ch/10000);
	ch %= 10000;
	uart_send('0' + ch/1000);
	ch %= 1000;
	uart_send('0' + ch/100);
	ch %= 100;
	uart_send('0' + ch/10);
	ch %= 10;
	uart_send('0' + ch);
}

void uart_send_hex(unsigned long ch) {
	const char lat[] = "0123456789abcdef";
	uart_send(lat[ch/0x100000]);
	ch &= 0x0FFFFF;
	uart_send(lat[ch/0x10000]);
	ch &= 0x0FFFF;
	uart_send(lat[ch/0x1000]);
	ch &= 0x0FFF;
	uart_send(lat[ch/0x100]);
	ch &= 0x00FF;
	uart_send(lat[ch/0x10]);
	ch &= 0x000F;
	uart_send(lat[ch]);
}

void uart_send_binary(unsigned long data) {
	unsigned long  mask = 0x80000000;
	for(int i=0; i<32; ++i) {
		if(!!(data&mask)) {
			uart_send('1');
		}
		else {
			uart_send('0');
		}
		mask>>=1;
	}
}

