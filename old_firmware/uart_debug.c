#include <system.h>
#include "uart_debug.h"

////////////////////////////////////////////////////////////
// INITIALISE SERIAL PORT FOR MIDI
void uart_init()
{
	pir1.1 = 0;		//TXIF 		
	pir1.5 = 0;		//RCIF
	
	pie1.1 = 0;		//TXIE 		no interrupts
	pie1.5 = 1;		//RCIE 		enable
	
	baudcon.4 = 0;	// SCKP		synchronous bit polarity 
	baudcon.3 = 0;	// BRG16	enable 16 bit brg
	baudcon.1 = 0;	// WUE		wake up enable off
	baudcon.0 = 0;	// ABDEN	auto baud detect
		
	txsta.6 = 0;	// TX9		8 bit transmission
	txsta.5 = 1;	// TXEN		transmit enable
	txsta.4 = 0;	// SYNC		async mode
	txsta.3 = 0;	// SEDNB	break character
	txsta.2 = 0;	// BRGH		high baudrate 
	txsta.0 = 0;	// TX9D		bit 9

	rcsta.7 = 1;	// SPEN 	serial port enable
	rcsta.6 = 0;	// RX9 		8 bit operation
	rcsta.5 = 0;	// SREN 	enable receiver
	rcsta.4 = 0;	// CREN 	continuous receive enable
		
	spbrgh = 0;		// brg high byte
	//spbrg = 51;		// brg low byte 
	spbrg = 25;		// brg low byte 
	
}

void uart_send(byte ch) 
{
	txreg = ch;
	while(!txsta.1);
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

