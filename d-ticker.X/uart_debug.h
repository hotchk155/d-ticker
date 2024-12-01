typedef unsigned char byte;
void uart_init();
void uart_send(byte ch);
void uart_send_string(byte *ch) ;
void uart_send_number(int ch);
void uart_send_long(long ch);
void uart_send_hex(unsigned long ch);
void uart_send_binary(unsigned long data);
