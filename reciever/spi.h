/*
 * spi.h
 *
 * Created: 12/2/2013 2:06:54 PM
 *  Author: Tuan
 */ 


#ifndef SPI_H_
#define SPI_H_


#define LEDPORT PORTD
#define LED	6
#define SS_PIN	0
#define SCLK_PIN 1
#define MOSI_PIN 2
#define MISO_PIN 3


void Setup_SPI();
void Send_SPI_byte(uint8_t data);


#endif /* SPI_H_ */