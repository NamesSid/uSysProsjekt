#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "USART.h"

#define S_MAX 4500UL
#define S_MIN 800UL

//Port where DHT sensor is connected
#define DHT_DDR DDRB
#define DHT_PORT PORTB
#define DHT_PIN PINB
#define DHT_INPUTPIN 2

//timeout retries
#define DHT_TIMEOUT 200

ISR (PCINT0_vect){ //External interrupt_zero ISR
	if (!(PINB & (1<<PINB0)))
	{
		PORTD |= (1<<PORTD7);
	}
	else{
		PORTD &= ~(1<<PORTD7);
	}
}

static inline void Timer1Servo(){
	TCCR1A = (1 << COM1A1)|(1 <<WGM11);
	TCCR1B = (1 << WGM13)|(1 << WGM12)|(1 << CS11);
	// TCNT1 = 0;
	ICR1 = 40000;
	DDRB |= (1 << DDB1);
}

void iniADC(uint16_t channel){
	ADMUX = (1 << REFS0)|channel;
	ADCSRA = (1 << ADEN)|(1 <<ADPS1)|(1 << ADPS0)|(1 << ADPS2)|(1 << ADIE);
	ADCSRA |= (1 << ADSC);
}

uint16_t read_adc(uint8_t channel){
	//ADMUX &= 0xF0; //Clear the older channel that was read
	ADMUX |= channel; //Defines the new ADC channel to be read
	ADCSRA |= (1<<ADSC); //Starts a new conversion
	while(ADCSRA & (1<<ADSC)); //Wait until the conversion is done
	return ADC; //Returns the ADC value of the chosen channel
}

int8_t dht_GetTemp(int8_t *temperature, int8_t *humidity) {
	uint8_t bits[5];
	uint8_t i,j = 0;

	memset(bits, 0, sizeof(bits));

	//prepare correct port and pin of DHT sensor
	DHT_DDR |= (1 << DHT_INPUTPIN); //output
	DHT_PORT |= (1 << DHT_INPUTPIN); //high
	_delay_ms(100);

	//begin send request
	DHT_PORT &= ~(1 << DHT_INPUTPIN); //low
	_delay_ms(18);

	DHT_PORT |= (1 << DHT_INPUTPIN); //high
	DHT_DDR &= ~(1 << DHT_INPUTPIN); //input
	_delay_us(40);

	//check first start condition
	if((DHT_PIN & (1<<DHT_INPUTPIN))) {
		return -1;
	}
	_delay_us(80);
	
	//check second start condition
	if(!(DHT_PIN & (1<<DHT_INPUTPIN))) {
		return -1;
	}
	_delay_us(80);

	//read-in data
	uint16_t timeoutcounter = 0;
	for (j=0; j<5; j++) { //for each byte (5 total)
		uint8_t result = 0;
		for(i=0; i<8; i++) {//for each bit in each byte (8 total)
			timeoutcounter = 0;
			while(!(DHT_PIN & (1<<DHT_INPUTPIN))) { //wait for an high input (non blocking)
				timeoutcounter++;
				if(timeoutcounter > DHT_TIMEOUT) {
					return -1;
				}
			}
			_delay_us(30);
			if(DHT_PIN & (1<<DHT_INPUTPIN))
			result |= (1<<(7-i));
			timeoutcounter = 0;
			while(DHT_PIN & (1<<DHT_INPUTPIN)) {
				timeoutcounter++;
				if(timeoutcounter > DHT_TIMEOUT) {
					return -1;
				}
			}
		}
		bits[j] = result;
	}

	//reset port
	DHT_DDR |= (1<<DHT_INPUTPIN); //output
	DHT_PORT |= (1<<DHT_INPUTPIN); //low
	_delay_ms(100);

	//compare checksum
	if ((uint8_t)(bits[0] + bits[1] + bits[2] + bits[3]) == bits[4]) {
		//return temperature and humidity
		*temperature = bits[2];
		*humidity = bits[0];
		
		return 0;
	}

	return -1;
}

int8_t temperature_int = 0;
int8_t humidity_int = 0;
char temperature_str[8];
char humidity_str[8];

int8_t dht_GetTempUtil(int8_t *temperature, int8_t *humidity) {
	return dht_GetTemp(temperature, humidity);
}

int main(void){
	
	initUSART();
	PORTB |= (1<<PORTB0);//Innput pull up på poert B0
	DDRD |= (1<<DDD7);//Pinne PD7 er output
	DDRB &= ~(1<<DDB0);//pinne B0 er innput
	PCMSK0 = (1<<PCINT0)|(1<<PCINT1);//
	PCICR |= (1<<PCIE0);//

	uint16_t adc_value;
	iniADC(0);
	iniADC(1);
	
	
	
	char buffer[10];
	Timer1Servo();
	sei();//Kalle opp interuppt
	
	while (1)
	{
				
				ADCSRA |= (1 << ADSC);
				while(!(ADCSRA |= (1 << ADSC))){}
				OCR1A = (ADC*(S_MAX-S_MIN)/1024 + S_MIN);
				
				if (dht_GetTempUtil(&temperature_int, &humidity_int) != -1) {
					itoa(temperature_int,temperature_str,10);
					printString("Temperature: ");
					printString(temperature_str);
					printString(" C\r\n");
					itoa(humidity_int,humidity_str,10);
					printString("Humidity: ");
					printString(humidity_str);
					printString(" %\r\n");
				}
				adc_value = read_adc(0);
				itoa(adc_value, buffer, 10);
				printString(buffer);
				_delay_ms(1000);
				printString(" ");
	} 
}