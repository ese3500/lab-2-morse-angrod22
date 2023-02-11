/*
 * partb.c.c
 *
 * Created: 2/8/2023 1:42:55 PM
 * Author : angel
 */ 

#define F_CPU 16000000UL
#define BAUD_RATE 9600
#define BAUD_PRESCALER (((F_CPU / (BAUD_RATE * 16UL))) - 1)


#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// Q5: PART B 

void Initialize() {
	// Disable global interrupts
	cli();

	// Set PB0 (ICP1) to input
	DDRB &= ~(1<<DDB0);

	// Set PB5 to output
	DDRB |= (1<<DDB5);

	//Timer1 Setup
	//Set Timer 1 clock to be internal div by 8
	//2MHz timer clock, 1 tick = (1 / 2M) sec
	TCCR1B &= ~(1<<CS10);
	TCCR1B |= (1<<CS11);
	TCCR1B &= ~(1<<CS12);

	//Set timer 1 to normal
	TCCR1A &= ~(1<<WGM10);
	TCCR1A &= ~(1<<WGM11);
	TCCR1B &= ~(1<<WGM12);
	TCCR1B &= ~(1<<WGM13);

	//looking for rising edge
	TCCR1B |= (1<<ICES1);
	
	//clear interrupt flag (could be omitted)
	TIFR1 |= (1<<ICF1);

	//enable input capture interrupt
	TIMSK1 |= (1<<ICIE1);

	sei();
}

ISR(TIMER1_CAPT_vect){
	if (PINB & (1<<PINB0)){ //if button pressed...
		PORTB |= (1<<PORTB5); //...turn on LED
		TCCR1B &= ~(1<<ICES1); //now look for falling edge
	} else { //else if button not pressed...
		PORTB &= ~(1<<PORTB5); //...turn off LED
		TCCR1B |= (1<<ICES1); //now look for rising edge
	}
}

int main(void)
{
 //Q5: PART B
	Initialize();
	while (1) {
		
	}
}