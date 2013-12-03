/******************************************************************/
// Author: Tuan Truong
// Date created: 10/29/13
// This is the source code for the transmitter for the ECE473 project
// The transmitter is supposed to:
// - write the time to an LCD using I2C
// - Send wireless data via USART through XBEE
// - Use SPI to read in 2 encoders & 4 push buttons
// - Sense temperature and display on LCD
// - Use PWM to control gain of mic amplifier
// - send audio data via DAC to frequency divider and read output via SPI

// Hardware description
// - Push buttons & Encoder on SPIC
//	- SPIC_SS 	PC4
//	- SPIC_MOSI 	PC5
//	- SPIC_MISO	PC6
//	- SPIC_SCK	PC7
//	- SHIFT_LOAD	PE2
//	- SHIFT_LATCH	PE3
// - Frequency divider on SPID
//	- SPID_SS	PD4
//	- SPID_MOSI	PD5
//	- SPID_MISO	PD6
//	- SPID_SCK	PD7
//	- FQD_RST	PB0
//	- FQD_AUDIO	PB2
// - LCD on TWIC
//	- TWIC_SDA	PC0
//	- TWIC_SCL	PC1
//	- LCD_SS	PD0
//	- LCD_RST	PD1
//	- LCD_SIG	PD2
//	- LCD_LIGHT	PD3
// - XBEE on USARTC0
//	- USART_RXCO	PC2
//	- USART_TXC0	PC3
// - Audio in
// 	- AUD_IN 	PA1
//	- AUD_VOL	PE0
// - Temp Sensor
//	- TEMP		PA0
//
//
//
// Plan of Attack:
// pretty much just wing it... 
/******************************************************************/


//defining clock speed
#define F_CPU 32000000UL

//the includes
#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>
#include"spi.h"
#include"usart.h"
#include"timer.h"


//macros
#define SDA_PIN		PIN0_bm
#define SCL_PIN		PIN1_bm

#define LCD_SS_PIN	PIN0_bm
#define LCD_RST_PIN	PIN1_bm
#define LCD_SIG_PIN	PIN2_bm
#define LCD_LIGHT_PIN	PIN3_bm



#define AUD_IN_PIN	PIN1_bm
#define AUD_VOL_PIN	PIN0_bm

#define TEMP_PIN	PIN0_bm



#define OPEN_COM 0xFF
#define CLOSE_COM 0xBA
#define ADDR_MODE 0xCB
#define COLOR_MODE 0x0C
#define RAINBOW_MODE 0xAC

#define RED_BUTTON 5
#define GREEN_BUTTON 7
#define BLUE_BUTTON 4
#define YELLOW_BUTTON 6

#define LED_NUM 32

#define FULL_COLOR 0

#define SPECTRUM_TEST 0


uint8_t mode = 0;


//set up Data Direction Register non-serial protocols
void Setup_DDR(){
	//1 = output, 0 = input
	PORTE.DIRSET = SHIFT_LATCH_PIN | SHIFT_LOAD_PIN | AUD_VOL_PIN;
	PORTB.DIRSET = FQD_RST_PIN | FQD_AUDIO_PIN;
	PORTD.DIRSET = LCD_SS_PIN | LCD_RST_PIN | LCD_SIG_PIN | LCD_LIGHT_PIN;

	//set audio and temp sensor to input
	PORTA.DIRCLR = AUD_IN_PIN | TEMP_PIN;

	return;
}




void Setup_TWIC(){

}





void Setup_ADC(){
    
    ADC.CTRLA =  ADC_DMASEL_CH01_gc;
    
    
}

//pop buffer
uint8_t pop_buffer(uint8_t *buffer, uint8_t buffer_location){
	return buffer[buffer_location];
}

void push_buffer(uint8_t *buffer, uint8_t buffer_location, uint8_t data){
	*(buffer + buffer_location) = data;
	return;
}

//return 1 if there is a different in the two buffer
uint8_t compare_buffer(uint8_t *buffer1, uint8_t *buffer2){
	uint8_t i =0;

	while (*buffer1) {
		if (*buffer1 != *buffer2) {
			return 1;
		}
	}
	return 0;

}




//reads buttons and figure what to do with it...
void checkbutton(uint8_t *encoders, uint8_t *array){
    uint8_t input =0;
    uint8_t i =0;
    input = Read_Buttons();
    if(input & 0xF0){
        if(input & (1<<RED_BUTTON)){
            *(array+1) = COLOR_MODE;
            *(array+2) = 35;
        }
        else if(input & (1<<GREEN_BUTTON)){
            *(array+1) = COLOR_MODE;
            *(array+2) = 108;
        }
        else if(input & (1<<BLUE_BUTTON)){
            *(array+1) = COLOR_MODE;
            *(array+2) = 180;
        }
        else if(input & (1<<YELLOW_BUTTON)){
            
            //mode = FULL_COLOR;
            *(array+1) = RAINBOW_MODE;
        
           
        }
        
        
        usart_send_string(array, 4);
        _delay_ms(10);
        //while((Read_Buttons() & 0xF0));
     
    }
    
    
    uint8_t *prev = encoders;
    uint8_t *curr = encoders +1;
    *curr = input & 0x03;
    
    if (((*prev &0x03) == 0x03) &&  ((*curr&0x03)==0x02)){
        *(array+2) = *(array+2) +2;
        if (*(array+2) > 252) {
            *(array+2) = 252;
            return;
        }
        usart_send_string(array, 4);
        //_delay_ms(5);
    }
    else if (((*prev &0x03) == 0x03) &&  ((*curr&0x03)==0x01)){
        *(array+2) = *(array+2) -2;
        if (*(array+2) < 1) {
            *(array+2) = 1;
            return;
        }
        usart_send_string(array, 4);
        //_delay_ms(5);

    }
    
    
    *prev = *curr;
    

}





int main(){
	//set 32MHz clock
	setClockTo32MHz();

	_delay_ms(200);

	//set up
	Setup_DDR();
	Setup_SPIC();
	Setup_SPID();
	Setup_USARTC();

	//Setup_PWM();
    //Setup_TWI();
    //Setup_ADC();

    //i am in no hurry...
	_delay_ms(200);
	_delay_ms(200);
    
    //some variables
    uint8_t i =0;
    uint8_t temp;
	uint8_t encoders[2];
	uint8_t array[50];
    
	array[0] = OPEN_COM;
	array[1] = 0;
	array[2] = 0x00;
	array[3] = CLOSE_COM;
    
    //send a rainbow to start, i just wanna see if it works...
    usart_send_byte(OPEN_COM);
    _delay_ms(3);
    usart_send_byte(ADDR_MODE);
    _delay_ms(3);
    for(i =0; i<LED_NUM; i++){
		usart_send_byte((i+1) * 6);
        _delay_ms(3);
	}
    usart_send_byte(CLOSE_COM);
    
    //lets take this slow shall we?
    _delay_ms(500);
	
	
	while(1){
#if SPECTRUM_TEST
        array[1] = COLOR_MODE;
        array[2] = temp;
        usart_send_string(array, 4);
        _delay_ms(50);
        temp++;
#else
        //_delay_ms(50);
        checkbutton(encoders, array);
#endif
        
        
        
    }

}
