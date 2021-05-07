#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>

#include "USART.h"

//Port where DHT sensor is connected
#define DHT_DDR DDRB
#define DHT_PORT PORTB
#define DHT_PIN PINB
#define DHT_INPUTPIN 0

//timeout retries
#define DHT_TIMEOUT 200

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

int8_t dht_GetTempUtil(int8_t *temperature, int8_t *humidity) {
	return dht_GetTemp(temperature, humidity);
}

int8_t temperature_int = 0;
int8_t humidity_int = 0;
char temperature_str[8];
char humidity_str[8];


int main(void) {
  initUSART();
  // ------ Event loop ------ //
  while (1) {
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
	_delay_ms(500);
  }                                                  /* End event loop */
  return 0;
}