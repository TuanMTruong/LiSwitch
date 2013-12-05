/****************************************************************/
// Author: Tuan Truong
// Date created: 10/27/13
// This is the source code for the reciever for the ECE473 project
// The Reciever is supposed to:
// - wait for incoming data from XBEE via USART
// - parce data
// - send data out via SPI to 6 RGB LEDs

// Hardware description
// - SPI SS 	PB0
// - SPI SCLK 	PB1
// - SPI MOSI	PB2
// - SPI MISO	PB3
// - USART RXD1	PD2
// - USART TXD1	PD3
// - LED	PD6

/****************************************************************/


//define CPU clock
#define F_CPU 16000000UL

//includes
#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>
#include "spi.h"
#include "usart.h"
#include "hs_converter.h"

// macros
#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))
#define CPU_16MHz 0x00
#define CPU_4MHz 0x02

#define LED_PIN 6
#define LED_PORT PORTD

#define OPEN_COM 0xFF
#define CLOSE_COM 0xBA
#define ADDR_MODE 0xCB
#define COLOR_MODE 0x0C
#define RAINBOW_MODE 0xAC
#define RAINBOW2_MODE 0xCA
#define TIME_MODE 0xEE
#define TIME_MODE_2 0xAA

#define LED_NUM 32

//debug mode
#define HS_TESTING 0
#define SINGLE_COLOR_TESTING 1
#define TIME_TESTING 0

#define t_sec 8
#define t_min 7
#define t_hour 6
#define t_count 9

uint8_t time_array[35];

uint8_t flag = 0;
/****************************************************************/

//Set up Data Direction Registers for LEDs, enables, and latches
void Setup_DDR(){
	//1 = output, 0 = input
	//on board LED
	DDRD = (1<<LED_PIN);
    
}

//used to fill an a array with a single byte
void Fill_array(uint8_t *array, uint8_t lenght, uint8_t data){
	uint8_t counter;
    
	for (counter =0; counter<lenght; counter++) {
		*(array+counter) = data;
	}
	return;
}

void Setup_Timer(){
    TCCR1A = 0;
    TCCR1B = (1<<WGM12) | (4<<CS10);
    TIMSK1 = 1<<OCIE1A;
    TCNT1 = 0;
    OCR1A = 60000;
    return;
    
}



void file_time(uint8_t hour, uint8_t min){
    uint8_t i =0;
    
    uint8_t hour_1 = (hour / 10)<<2;
    uint8_t hour_0 = (hour % 10)<<4;
    uint8_t min_1 = (min /10);
    uint8_t min_0 = (min % 10)<<2;
    
    
    time_array[2] = hour_1;
    time_array[3] = hour_0;
    time_array[4] = min_1;
    time_array[5] = min_0;
   
    
    return;
}

/****************************************************************/


//USART receiving interrupt
//When data is received it will be stored in a buffer waiting to be accessed
ISR(USART1_RX_vect){
	//Store data
	uint8_t temp = UDR1;
	if (temp == COLOR_MODE || temp == ADDR_MODE || temp == RAINBOW_MODE || temp == TIME_MODE || temp ==TIME_MODE_2){
		if (ReadData(get_buff_location()-1) == OPEN_COM){
			set_buff_location(0);
			PushData(OPEN_COM);
		}
	}
    
    //if data received is a start communication byte then wait for the second byte to confirm
    //mode byte
    /**    if (temp ==  OPEN_COM){
     while (!(UCSR1A & (1<<RXC1))) {}
     temp = UDR1;
     //if mode byte then set location to 0
     if (temp == COLOR_MODE || temp == ADDR_MODE || temp == RAINBOW_MODE){
     set_buff_location(0);
     
     }
     //start pushing data to the buffer
     PushData(OPEN_COM);
     }
     **/
    //push, push, push, just keep pushing
	PushData(temp);
}

//testing
ISR(USART1_UDRE_vect){
	LED_PORT ^= (1<<LED_PIN);
	UDR1 = 'a';
	_delay_ms(1);
}

//Bad interrupt catcher
ISR(BADISR_vect){
	while(1){
		LED_PORT ^= (1<<LED_PIN);
		_delay_ms(100);
	}
}

ISR(TIMER1_COMPA_vect){
    LED_PORT ^= 1<<LED_PIN;
    
    time_array[16] ^= 0xff;
    time_array[17] ^= 0xff;

   
    time_array[t_sec]++;
    if (time_array[t_sec] > 60) {
        time_array[t_sec] =0;
        time_array[t_min]++;
        if (time_array[t_min] > 60) {
            time_array[t_min] =0;
            time_array[t_hour]++;
                
        }
    }

    
    TCNT1=0;
    
}

/****************************************************************/

uint8_t verify_buff(){
    
	if (ReadData(0) == OPEN_COM ){
		if (ReadData(1) == COLOR_MODE || ReadData(1) == RAINBOW_MODE|| ReadData(1) == TIME_MODE || ReadData(1) ==TIME_MODE_2){
			if (ReadData(3) == CLOSE_COM){
				return 1;
			}
		}
		else if (ReadData(1) == ADDR_MODE){
			if (ReadData(LED_NUM+2) == CLOSE_COM){
				return 1;
			}
		}
	}
	return 0;
    
}

void display_mode(uint8_t *data, uint8_t *disp_buff){
	uint8_t i =0;
	
	if (*(disp_buff+1)== COLOR_MODE){
		hs_convert(*(disp_buff+2), 0, data);
		_delay_ms(1);
		for(i=0; i<LED_NUM; i++){
            //cli();
			Send_SPI_array(data, (data+1), (data+2),1);
            //            sei();
		}
	}
	else if (*(disp_buff+1) == ADDR_MODE){
		_delay_ms(1);
		for(i=0; i<LED_NUM; i++){
            //          cli();
			hs_convert(*(disp_buff+2+i), 0, data);
			Send_SPI_array(data, (data+1), (data+2),1);
            //        sei();
            
		}
	}
    else if (ReadData(1) == RAINBOW_MODE){
        _delay_ms(1);
        for(i=0; i<LED_NUM; i++){
            //      cli();
			hs_convert((i+1)*7, 0, data);
			Send_SPI_array(data, (data+1), (data+2),1);
            //    sei();
            
		}
    }
    else if (ReadData(1) == TIME_MODE) {
        /**
        if (flag == 0){
        TCCR1B &= ~(7<<CS10);
        TCCR1B |= (4<<CS10);
            time_array[6] = 12;
            time_array[7] = 11;
            time_array[t_sec] = 0;
            for (i=0; i<96; i++) {
                Send_SPI_byte(0x00);
            }
            flag = 200;
        }
         **/
        *data = 0xff;
        *(data+1) = 0;
        file_time(time_array[6],time_array[7]);
        _delay_ms(1);
        for(i=9; i>=4; i--){
            if(time_array[2] & (1<<(i-2))){
                Send_SPI_array(data, (data+1), (data+1),1);
            }
            else{
                Send_SPI_array((data+1), (data+1), (data+1),1);
            }
            
		}
        Send_SPI_byte(0xFF);
        Send_SPI_byte(0xFF);
        Send_SPI_byte(0xFF);
        
     
        Send_SPI_byte(0x00);
        Send_SPI_byte(0x00);
        Send_SPI_byte(0x00);
   
        for(i=9; i>=6; i--){
            if(time_array[3] & (1<<(i-2))){
                Send_SPI_array((data+1), (data+0), (data+1),1);
            }
            else{
                Send_SPI_array((data+1), (data+1), (data+1),1);
            }
		}
        Send_SPI_byte(0xFF);
        Send_SPI_byte(0xFF);
        Send_SPI_byte(0xFF);
        
        
        Send_SPI_byte(0x00);
        Send_SPI_byte(0x00);
        Send_SPI_byte(0x00);
        
        
        Send_SPI_byte(time_array[16]);
        Send_SPI_byte(0x00);
        Send_SPI_byte(0x00);
        Send_SPI_byte(time_array[16]);
        Send_SPI_byte(0x00);
        Send_SPI_byte(0x00);
        
        for(i=9; i>=3; i--){
            if(time_array[4] & (1<<(i-3))){
                Send_SPI_array((data+1), (data+1), (data+0),1);
            }
            else{
                Send_SPI_array((data+1), (data+1), (data+1),1);
            }
		}
        
        Send_SPI_byte(0xFF);
        Send_SPI_byte(0xFF);
        Send_SPI_byte(0xFF);
        
        
        Send_SPI_byte(0x00);
        Send_SPI_byte(0x00);
        Send_SPI_byte(0x00);
        
        
        for(i=9; i>=5; i--){
            if(time_array[5] & (1<<(i-3))){
                Send_SPI_array((data+0), (data+0), (data+1),1);
            }
            else{
                Send_SPI_array((data+1), (data+1), (data+1),1);
            }
		}
        Send_SPI_byte(0xFF);
        Send_SPI_byte(0xFF);
        Send_SPI_byte(0xFF);
        
        
        Send_SPI_byte(0x00);
        Send_SPI_byte(0x00);
        Send_SPI_byte(0x00);
        
    }
    /**
    else if (ReadData(1) == TIME_MODE_2){
        TCCR1B &= ~(7<<CS10);
        TCCR1B |= (2<<CS10);
        flag = 0;
        _delay_ms(100);
        ReadData(1) == TIME_MODE;
    }
     **/
     
}


int main(){
	//Set clock to 16 Mhz
	CPU_PRESCALE(CPU_16MHz);
	Setup_DDR();
	Setup_SPI();
	Setup_USART();
    Setup_Timer();
    
    //lots of variables, some unused and some are needed
    
	uint8_t hs_rgb[3];
    uint8_t disp_buff[50];
    
    
	uint8_t red_array[50];
	uint8_t blue_array[50];
	uint8_t green_array[50];
    
    
	uint8_t temp=0;
	uint8_t i = 0;
	_delay_ms(200);
    
    //give me a light to show you are alive
	//LED_PORT |= (1<<LED_PIN);
    
	Fill_array(red_array, 32, 0x00);
	Fill_array(blue_array, 32, 0x00);
	Fill_array(green_array, 32, 0x00);
    
    //lets start out by filling buffer with rainbow color
    //its a pretty boot_up
	set_buff_location(0);
	PushData(OPEN_COM);
	PushData(COLOR_MODE);
	for(i =0; i<LED_NUM; i++){
		PushData((i+1) * 7);
	}
	PushData(CLOSE_COM);
    
    
    
    
    //let the interrupts interrupt
	sei();
    
    //got all the time in the world...
	_delay_ms(200);
    
    for (i =2; i<34; i++) {
        time_array[i] = 0;
    }
    
    time_array[6] = 12;
    time_array[7] = 11;
    time_array[t_sec] = 0;
    
	while(1){
        
        //used in earlier development for testing
#if HS_TESTING
		_delay_ms(100);
        
        
        
		Send_SPI_array(red_array, green_array, blue_array, 32);
        
        
		hs_convert(temp, 0, hs_rgb);
		Fill_array(red_array, 32, *hs_rgb);
		Fill_array(green_array, 32, *(hs_rgb+1));
		Fill_array(blue_array, 32, *(hs_rgb+2));
		temp++;

#endif
#if TIME_TESTING
        time_array[6] = 12;
        time_array[7] = 11;
        time_array[t_sec] = 0;
        while (1) {
            _delay_ms(100);
            display_mode(hs_rgb, disp_buff);
        }
        
#endif
        
#if SINGLE_COLOR_TESTING
        //Check if data buffer recieved from xbee has a start, mode, and end byte
		if(verify_buff()){
            //if so pop a light for me
			LED_PORT ^= 1<<LED_PIN;
            //WIP: need to copy data from recieving buffer to a disaply buffer so data doesn't change
            //when recieving wireless data while in the middle of displaying data
            for (i=0; i<50; i++) {
                disp_buff[i] = ReadData(i);
            }
            
			
            //speaking of displaying data.. display date
            //give 3 element array to store rgb temp data
            display_mode(hs_rgb, disp_buff);
		}
        

        
#endif
        
	}
    
}
