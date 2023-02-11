/*
 * main.c.c
 *
 * Created: 2/8/2023 1:57:07 PM
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
#include "uart.h"


char String[25];

int pressedBool = 0;
int releasedBool = 1;
int spaceBool = 0;
int overflowBool = 0;
int releasedTime = 0;


void Initialize() {
	// Disable global interrupts
	cli();

	// Set PB0 (ICP1) to input
	DDRB &= ~(1<<DDB0);

	// Set PB1 and PB2 to output
	DDRB |= (1<<DDB1);
	DDRB |= (1<<DDB2);

	//Timer1 Setup
	//Set Timer 1 clock to be internal div by 1024
	//15.625kHz timer clock, 1 tick = (1 / 15.625k) sec
	TCCR1B |= (1<<CS10);
	TCCR1B &= ~(1<<CS11);
	TCCR1B |= (1<<CS12);

	//Set timer 1 to normal
	TCCR1A &= ~(1<<WGM10);
	TCCR1A &= ~(1<<WGM11);
	TCCR1B &= ~(1<<WGM12);
	TCCR1B &= ~(1<<WGM13);

	//looking for rising edge
	TCCR1B |= (1<<ICES1);
	
	//clear interrupt flags (could be omitted)
	TIFR1 |= (1<<ICF1);
	TIFR1 |= (1<<TOV1);

	//enable input capture interrupt
	TIMSK1 |= (1<<ICIE1);
	
	//overflow interrupt
	TIMSK1 |= (1<<TOIE1);

	sei();
}

ISR(TIMER1_CAPT_vect){
	if (PINB & (1<<PINB0)){ //if button pressed...
		pressedBool = 1;
		releasedBool = 0;
		releasedTime = 0;
		//turn off any on leds
		PORTB &= ~(1<<PORTB1);
		PORTB &= ~(1<<PORTB2);
		TCNT1 = 0; //clear timer
		TCCR1B &= ~(1<<ICES1); //look for falling edge
	} else { //else (if button released)...
		releasedBool = 1;
		releasedTime = TCNT1;
		TCNT1 = 0; //clear timer
		TCCR1B |= (1<<ICES1); //look for rising edge
	}
	overflowBool = 0;
}

ISR(TIMER1_OVF_vect) {
	overflowBool = 1;
}

// 0 = space, 1 = dot, 2 = dash, -1 = already space, not pressed , pressed too long/short
int dotDashSpace() {
	if((TCNT1 >= 40000) && (overflowBool == 0) && (releasedBool == 1) && (spaceBool == 0)){ // 2.56s, time not overflowed, button not pressed, previous not space
		spaceBool = 1;
		
		//turn off any on leds
		PORTB &= ~(1<<PORTB1);
		PORTB &= ~(1<<PORTB2);
		
		return 0;
	} else if ((overflowBool == 0) && (pressedBool == 1)) { //if button pressed + time not overflowed
		
		if ((1000 <= releasedTime) && (releasedTime <= 15000)) { //btwn 0.064s + 0.96s (only know total time pressed until released)
			pressedBool = 0; //using this lagging pressedBool so that can still enter here while having released time
			spaceBool = 0;
			
			if (PORTB & (1<<PORTB2)) {
				PORTB &= ~(1<<PORTB2); //turn off other led if on
			}
			PORTB |= (1<<PORTB1); //turn on led
			
			return 1;
		} else if ((15000 < releasedTime) && (releasedTime <= 65000)) { //btwn 0.96s + 4.16s
			spaceBool = 0;
			pressedBool = 0;
			
			if (PORTB & (1<<PORTB1)) {
				PORTB &= ~(1<<PORTB1); //turn off other led if on
			}
			PORTB |= (1<<PORTB2); //turn on led
			
			return 2;
		}
	}
	
	return -1;
}
 
 //keeps look until gets the next dash, dot, or space 
 int getNxt() {
	 int nxtPiece = dotDashSpace();
	 
	 while(nxtPiece < 0) {
		 nxtPiece = dotDashSpace();
	 }
	 
	 return nxtPiece;
 }


int main(void)
{
	UART_init(BAUD_PRESCALER);
	
	Initialize();
		
	while (1) {
		
		//getting letters
		int nxtPiece = getNxt();
		
		if (nxtPiece == 1) { //DOT...
			nxtPiece = getNxt();
			
			if (nxtPiece == 1) { //DOT, DOT...
				nxtPiece = getNxt();
				
				if (nxtPiece == 1) { //DOT, DOT, DOT...
					nxtPiece = getNxt();
					
					if (nxtPiece == 1) { //DOT, DOT, DOT, DOT...
						nxtPiece = getNxt();
						
						if (nxtPiece == 1) {
							sprintf(String,"5"); //dot, dot, dot, dot, dot 
							UART_putstring(String);
						} else if (nxtPiece == 2) {
							sprintf(String,"3"); //dot, dot, dot, dot, dash
							UART_putstring(String);
						} else {
							sprintf(String,"H"); //dot, dot, dot, dot
							UART_putstring(String);
						}
					} else if (nxtPiece == 2) { //DOT, DOT, DOT, DASH...
						nxtPiece = getNxt();
						
						if (nxtPiece == 1) {
							sprintf(String,"?"); //dot, dot, dot, dash, dot
							UART_putstring(String);
						} else if (nxtPiece == 2) {
							sprintf(String,"?"); //dot, dot, dot, dash, dash
							UART_putstring(String);
						} else {
							sprintf(String,"V"); //dot, dot, dot, dash
							UART_putstring(String);
						}
					} else {
						sprintf(String,"S"); //dot, dot, dot
						UART_putstring(String);
					}
				} else if (nxtPiece == 2) { //DOT, DOT, DASH...
					nxtPiece = getNxt();
					
					if (nxtPiece == 1) { //DOT, DOT, DASH, DOT...
						nxtPiece = getNxt();
						
						if (nxtPiece == 1) {
							sprintf(String,"?"); //dot, dot, dash, dot, dot
							UART_putstring(String);
						} else if (nxtPiece == 2) {
							sprintf(String,"?"); //dot, dot, dash, dot, dash
							UART_putstring(String);
						} else {
							sprintf(String,"F"); //dot, dot, dash, dot
							UART_putstring(String);
						}
					} else if (nxtPiece == 2) { //DOT, DOT, DASH, DASH...
						nxtPiece = getNxt();
						
						if (nxtPiece == 1) {
							sprintf(String,"?"); //dot, dot, dash, dash, dot
							UART_putstring(String);
						} else if (nxtPiece == 2) {
							sprintf(String,"?"); //dot, dot, dash, dash, dash
							UART_putstring(String);
						} else {
							sprintf(String,"?"); //dot, dot, dash, dash
							UART_putstring(String);
						}
					} else {
						sprintf(String,"U"); //dot, dot, dash
						UART_putstring(String);
					}
				} else {
					sprintf(String,"I"); //dot, dot
					UART_putstring(String);
				}
			} else if (nxtPiece == 2) { //DOT, DASH...
				nxtPiece = getNxt();

				if (nxtPiece == 1) { //DOT, DASH, DOT...
					nxtPiece = getNxt();
	
					if (nxtPiece == 1) { //DOT, DASH, DOT, DOT...
						nxtPiece = getNxt();
		
						if (nxtPiece == 1) {
							sprintf(String,"?"); //dot, dash, dot, dot, dot
							UART_putstring(String);
						} else if (nxtPiece == 2) {
							sprintf(String,"?"); //dot, dash, dot, dot, dash
							UART_putstring(String);
						} else {
							sprintf(String,"L"); //dot, dash, dot, dot
							UART_putstring(String);
						}
					} else if (nxtPiece == 2) { //DOT, DASH, DOT, DASH...
						nxtPiece = getNxt();
		
						if (nxtPiece == 1) {
							sprintf(String,"?"); //dot, dash, dot, dash, dot
							UART_putstring(String);
						} else if (nxtPiece == 2) {
							sprintf(String,"?"); //dot, dash, dot, dash, dash
							UART_putstring(String);
						} else {
							sprintf(String,"?"); //dot, dash, dot, dash
							UART_putstring(String);
						}
					} else {
						sprintf(String,"R"); //dot, dash, dot
						UART_putstring(String);
					}
				} else if (nxtPiece == 2) { //DOT, DASH, DASH...
					nxtPiece = getNxt();
	
					if (nxtPiece == 1) { //DOT, DASH, DASH, DOT...
						nxtPiece = getNxt();
		
						if (nxtPiece == 1) {
							sprintf(String,"?"); //dot, dash, dash, dot, dot
							UART_putstring(String);
						} else if (nxtPiece == 2) {
							sprintf(String,"?"); //dot, dash, dash, dot, dash
							UART_putstring(String);
						} else {
							sprintf(String,"P"); //dot, dash, dash, dot
							UART_putstring(String);
						}
					} else if (nxtPiece == 2) { //DOT, DASH, DASH, DASH...
						nxtPiece = getNxt();
		
						if (nxtPiece == 1) {
							sprintf(String,"?"); //dot, dash, dash, dash, dot
							UART_putstring(String);
						} else if (nxtPiece == 2) {
							sprintf(String,"1"); //dot, dash, dash, dash, dash
							UART_putstring(String);
						} else {
							sprintf(String,"J"); //dot, dash, dash, dash
							UART_putstring(String);
						}
					} else {
						sprintf(String,"W"); //dot, dash, dash
						UART_putstring(String);
					}
				} else {
					sprintf(String,"A"); //dot, dash
					UART_putstring(String);
				}
			} else { 
				sprintf(String,"E"); //dot 
				UART_putstring(String);
			}
		} else if (nxtPiece == 2) { //DASH...
			nxtPiece = getNxt();
			
			if (nxtPiece == 1) { //DASH, DOT...
				nxtPiece = getNxt();
				
				if (nxtPiece == 1) { //DASH, DOT, DOT...
					nxtPiece = getNxt();
					
					if (nxtPiece == 1) { //DASH, DOT, DOT, DOT...
						nxtPiece = getNxt();
						
						if (nxtPiece == 1) {
							sprintf(String,"6"); //dash, dot, dot, dot, dot
							UART_putstring(String);
						} else if (nxtPiece == 2) {
							sprintf(String,"?"); //dash, dot, dot, dot, dash
							UART_putstring(String);
						} else {
							sprintf(String,"B"); //dash, dot, dot, dot
							UART_putstring(String);
						}
					} else if (nxtPiece == 2) { //DASH, DOT, DOT, DASH...
						nxtPiece = getNxt();
						
						if (nxtPiece == 1) {
							sprintf(String,"?"); //dash, dot, dot, dash, dot
							UART_putstring(String);
						} else if (nxtPiece == 2) {
							sprintf(String,"?"); //dash, dot, dot, dash, dash
							UART_putstring(String);
						} else {
							sprintf(String,"X"); //dash, dot, dot, dash
							UART_putstring(String);
						}
					} else {
						sprintf(String,"D"); //dash, dot, dot
						UART_putstring(String);
					}
				} else if (nxtPiece == 2) { //DASH, DOT, DASH...
					nxtPiece = getNxt();
					
					if (nxtPiece == 1) { //DASH, DOT, DASH, DOT...
						nxtPiece = getNxt();
						
						if (nxtPiece == 1) {
							sprintf(String,"?"); //dash, dot, dash, dot, dot
							UART_putstring(String);
						} else if (nxtPiece == 2) {
							sprintf(String,"?"); //dash, dot, dash, dot, dash
							UART_putstring(String);
						} else {
							sprintf(String,"C"); //dash, dot, dash, dot
							UART_putstring(String);
						}
					} else if (nxtPiece == 2) { //DASH, DOT, DASH, DASH...
						nxtPiece = getNxt();
						
						if (nxtPiece == 1) {
							sprintf(String,"?"); //dash, dot, dash, dash, dot
							UART_putstring(String);
						} else if (nxtPiece == 2) {
							sprintf(String,"?"); //dash, dot, dash, dash, dash
							UART_putstring(String);
						} else {
							sprintf(String,"Y"); //dash, dot, dash, dash
							UART_putstring(String);
						}
					} else {
						sprintf(String,"K"); //dash, dot, dash
						UART_putstring(String);
					}
				} else {
					sprintf(String,"N"); //dash, dot
					UART_putstring(String);
				}
			} else if (nxtPiece == 2) { //DASH, DASH...
				nxtPiece = getNxt();

				if (nxtPiece == 1) { //DASH, DASH, DOT...
					nxtPiece = getNxt();
					
					if (nxtPiece == 1) { //DASH, DASH, DOT, DOT...
						nxtPiece = getNxt();
						
						if (nxtPiece == 1) {
							sprintf(String,"7"); //dash, dash, dot, dot, dot
							UART_putstring(String);
						} else if (nxtPiece == 2) {
							sprintf(String,"?"); //dash, dash, dot, dot, dash
							UART_putstring(String);
						} else {
							sprintf(String,"Z"); //dash, dash, dot, dot
							UART_putstring(String);
						}
					} else if (nxtPiece == 2) { //DASH, DASH, DOT, DASH...
						nxtPiece = getNxt();
						
						if (nxtPiece == 1) {
							sprintf(String,"?"); //dash, dash, dot, dash, dot
							UART_putstring(String);
						} else if (nxtPiece == 2) {
							sprintf(String,"?"); //dash, dash, dot, dash, dash
							UART_putstring(String);
						} else {
							sprintf(String,"Q"); //dash, dash, dot, dash
							UART_putstring(String);
						}
					} else {
						sprintf(String,"G"); //dash, dash, dot
						UART_putstring(String);
					}
				} else if (nxtPiece == 2) { //DASH, DASH, DASH...
					nxtPiece = getNxt();
					
					if (nxtPiece == 1) { //DASH, DASH, DASH, DOT...
						nxtPiece = getNxt();
						
						if (nxtPiece == 1) {
							sprintf(String,"8"); //dash, dash, dash, dot, dot
							UART_putstring(String);
						} else if (nxtPiece == 2) {
							sprintf(String,"?"); //dash, dash, dash, dot, dash
							UART_putstring(String);
						} else {
							sprintf(String,"?"); //dash, dash, dash, dot
							UART_putstring(String);
						}
					} else if (nxtPiece == 2) { //DASH, DASH, DASH, DASH...
						nxtPiece = getNxt();
						
						if (nxtPiece == 1) {
							sprintf(String,"9"); //dash, dash, dash, dash, dot
							UART_putstring(String);
						} else if (nxtPiece == 2) {
							sprintf(String,"0"); //dash, dash, dash, dash, dash
							UART_putstring(String);
						} else {
							sprintf(String,"?"); //dash, dash, dash, dash
							UART_putstring(String);
						}
					} else {
						sprintf(String,"O"); //dash, dash, dash
						UART_putstring(String);
					}
				} else {
					sprintf(String,"M"); //dash, dash
					UART_putstring(String);
				}
			} else {
				sprintf(String,"T"); //dash
				UART_putstring(String);
			}
		}
		
		//testing detecting dot, dash, space
		/*int print = getNxt();
		if (print == 0){
			sprintf(String,"space \n");
			UART_putstring(String);
		} else if (print == 1) {
			sprintf(String,". \n");
			UART_putstring(String);
		} else if (print == 2) {
			sprintf(String,"-- \n");
			UART_putstring(String);
		}*/
		
		//testing variables
		/*dotDashSpace();
		//sprintf(String,"%u \n", TCNT1);
		//UART_putstring(String);
		//sprintf(String,"releasedBool = %u \n", releasedBool);
		//UART_putstring(String);
		//sprintf(String,"pressedBool = %u \n", pressedBool);
		//UART_putstring(String);
		//sprintf(String,"spaceBool = %u \n", spaceBool);
		//UART_putstring(String);
		//sprintf(String,"overflowBool = %u \n", overflowBool);
		//UART_putstring(String);
		//sprintf(String,"releasedTime = %u \n", releasedTime);
		//UART_putstring(String);*/
		
	}
}