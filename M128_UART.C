#include <iom128v.h>
#include <stdlib.h>
#include <string.h>	  
#include <stdio.h>	 
#include <macros.h>

#include "M128_UART.h"
#include "Main.h"

#define     COM1_BaudRate	2	// 0: 4800  1: 9600 2: 19200 3: 32800 4: 57600 5: 115200
#define     COM1_DataBit	3       // 0: 5Bit  1: 6Bit  2: 7Bit  3: 8Bit 
#define     COM1_StopBit        0       // 0: 1Bit  1: 2Bit
#define     COM1_ParityMode     0       // 0: Disabled   2: Even Parity  3: Odd Parity 
#define     Com1buffsize        256

#define     COM2_BaudRate	2	// 0: 4800  1: 9600 2: 19200 3: 32800 4: 57600 5: 115200
#define     COM2_DataBit	3       // 0: 5Bit  1: 6Bit  2: 7Bit  3: 8Bit  7: 9Bit
#define     COM2_StopBit	0       // 0: 1Bit  1: 2Bit
#define     COM2_ParityMode	0       // 0: Disabled   2: Even Parity  3: Odd Parity 
#define     Com2buffsize    	128


#define     COM1            	UDR0
#define     COM2            	UDR1

#define	    Com_BitClaer	Com2buffsize -1

#define     Com_Ck_data	  	(Main_XTAL/16)
const unsigned char BaudRate_data[] = {Com_Ck_data/4800L,Com_Ck_data/9600L,Com_Ck_data/19200L,
	  		   						Com_Ck_data/32800L,Com_Ck_data/57600L,Com_Ck_data/115200L};
									   
#pragma interrupt_handler UART0_TX_interrupt:iv_USART0_TX   
#pragma interrupt_handler UART0_RX_interrupt:iv_USART0_RX 
#pragma interrupt_handler UART1_TX_interrupt:iv_USART1_TX 
#pragma interrupt_handler UART1_RX_interrupt:iv_USART1_RX


unsigned char   Com1_RecBuff[Com1buffsize];	// USART1 RX QUEUE
unsigned char 	Com1_Rwi_posi,Com1_Rrd_posi;	

unsigned char   Com1_TriBuff[Com1buffsize];	// USART1 TX QUEUE	
unsigned char 	Com1_Twi_posi,Com1_Trd_posi;	

unsigned char   Com2_RecBuff[Com2buffsize];	// USART2 RX QUEUE
unsigned char 	Com2_Rwi_posi,Com2_Rrd_posi;	

unsigned char   Com2_TriBuff[Com2buffsize];	// USART2 TX QUEUE
unsigned char	Com2_Twi_posi,Com2_Trd_posi;	

void Com1_Init(char _rate)
{
	Com1_Rwi_posi = Com1_Rrd_posi = 0;	
	Com1_Twi_posi = Com1_Trd_posi = 0;	
    
	UBRR0L = BaudRate_data[_rate] - 1;
	UBRR0H = 0;
    
	UCSR0C = (COM1_ParityMode << 4) | (COM1_StopBit << 3) | (COM1_DataBit << 1);
	UCSR0B = (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0) | (0 << TXCIE0) ;
}

void Com1_CharTX(unsigned char Data)
{
	while(!(UCSR0A & (1<<5)));
	COM1 = Data;
}													   


void Com1_StringTX(unsigned char *str)
{
    	char count = 0;
    
    	while (str[count] != 0x00){
		while(!(UCSR0A & (1<<5)));
		COM1 = str[count];
		count++;
    	}
}

void Com1_TXDataWR(char *str)
{
    	char count = 0;

    	if((UCSR0A & (1<<5)) && (Com1_Twi_posi == Com1_Trd_posi))
	{
        	COM1 = str[0];
        	count = 1;
    	}
    	while (str[count] != 0x00)
	{
		Com1_TriBuff[Com1_Twi_posi++] = str[count];
		count++;
    	}
}

void Com1_TXDataChar(char _ch)
{	
    	if((UCSR0A & (1<<5)) && (Com1_Twi_posi == Com1_Trd_posi))COM1 = _ch;
    	else Com1_TriBuff[Com1_Twi_posi++] = _ch;
}

unsigned char Com1_rd_char(void)
{
    	char v = 0;
    	if(Com1_Rrd_posi != Com1_Rwi_posi)
		v = Com1_RecBuff[Com1_Rrd_posi++];
    	return(v);
}

#pragma interrupt_handler UART0_TX_interrupt:iv_USART0_TX 
void UART0_TX_interrupt(void)
{
    if(Com1_Twi_posi != Com1_Trd_posi)
        COM1 = Com1_TriBuff[Com1_Trd_posi++];
}

#pragma interrupt_handler UART0_RX_interrupt:iv_USART0_RX 
void UART0_RX_interrupt(void)
{
	PORTA ^= 0x01;
 	Com1_RecBuff[Com1_Rwi_posi++] = COM1;
}

void Com2_Init(char _rate)
{
    	Com2_Rwi_posi = Com2_Rrd_posi = 0;
    	Com2_Twi_posi = Com2_Trd_posi = 0;
    
    	UBRR1L = BaudRate_data[_rate] - 1;
    	UBRR1H = 0;
    
    	UCSR1C = (COM2_ParityMode << 4) | (COM2_StopBit << 3) | (COM2_DataBit << 1);
    	UCSR1B = (1 << RXCIE1) | (1 << RXEN1) | (1 << TXEN1) | (0 << TXCIE1);  
}

void Com2_CharTX(unsigned char Data)
{
	while(!(UCSR1A & (1<<5)));
	COM2 = Data;
}													   


void Com2_StringTX(unsigned char *str)
{
    	char count = 0;
    
    	while (str[count] != 0x00)
	{
		while(!(UCSR1A & (1<<5)));
		COM2 = str[count];
		count++;
    	}
}

void Com2_TXDataWR(unsigned char *str)
{
	    char count = 0;
	    if((UCSR1A & (1<<5)) && (Com2_Twi_posi == Com2_Trd_posi)){
		COM2 = str[0];
		count = 1;
	    }

	    while (str[count] != 0x00){
			Com2_TriBuff[Com2_Twi_posi++] = str[count];
			count++;
	    }
}

//unsigned char Com2_TXDataChar;

void Com2_TXDataChar(unsigned char _ch)
{
	if((UCSR1A & (1<<5)) && (Com2_Twi_posi == Com2_Trd_posi))COM2 = _ch;
    else Com2_TriBuff[Com2_Twi_posi++] = _ch;
}

unsigned char Com2_rd_char(void)
{
    char v = 0;
    if(Com2_Rrd_posi != Com2_Rwi_posi)
	   	v = Com2_RecBuff[Com2_Rrd_posi++];
    return(v);
}

unsigned char Com2_rd_string(char *str,unsigned char St_add)
{
	
	if(Com2_Rrd_posi != Com2_Rwi_posi){ 		
		str[St_add++] = Com2_RecBuff[Com2_Rrd_posi++];
		if(St_add == 10)PORTF ^= 0x01;
	    	Com2_Rrd_posi &= Com_BitClaer;
    	}
	
    	return(St_add);
}


unsigned char Tx_Count,Rx_Count;

unsigned char Com2_TxBuff_Ck(void)
{
	unsigned char bu;

	bu = Tx_Count;
	Tx_Count -= bu;
	return(bu);
}

unsigned char Com2_RXBuff_Ck(void)
{
	unsigned char bu;

	bu = Rx_Count;
	Rx_Count -= bu;
	return(bu);
}

#pragma interrupt_handler UART1_TX_interrupt:iv_USART1_TX 
void UART1_TX_interrupt(void)
{
    if(Com2_Twi_posi != Com2_Trd_posi)
        COM2 = Com2_TriBuff[Com2_Trd_posi++];	
    Tx_Count++;
}

#pragma interrupt_handler UART1_RX_interrupt:iv_USART1_RX 
void UART1_RX_interrupt(void)
{
	
	Com2_RecBuff[Com2_Rwi_posi++] = COM2;
}

