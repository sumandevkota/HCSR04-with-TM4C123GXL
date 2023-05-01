// Lab 8 Interfacing Unltasonic Sensor
//
/*
Trigger output from PE1 goes to the Trig pin on sensor
Echo from the sensor goes to PB6 of launchpad
*/
//


// ***** 1. Pre-processor Directives Section *****
#include <stdio.h>   // standard C library 
#include <math.h>
#include <stdint.h>
#include "uart.h"    // functions to implment input/output
#include "TExaS.h"   // Lab grader functions
#include "PLL.h"     // Set up core clock
#include "tm4c123gh6pm.h"

// ***** 2. Global Declarations Section *****
unsigned long volatile delay;

uint32_t Distance = 0;   // in cm
uint32_t period;	       // in us

// Define output pin
#define PE1                     (*((volatile unsigned long *)0x40024008))

// FUNCTION PROTOTYPES: Each subroutine defined
void EnableInterrupts(void);      // Enable interrupts
uint32_t Timer0A_periodCapture(void);  // Captures Echo
void Calculate_Distance(void);    // Calcualtes distance in cm
void Timer0Capture_Init(void);    // echo timer
void Init_PortE(void);            // init PortB
void Trigger_Pulse(void);         // send trigger pulse
void timer1A_delayus(int ttime);  // timer for trigger


// ***** 3. Subroutines Section *****
int main (void) {
	
  //TExaS_Init(UART_PIN_PA0,UART_PIN_PA1); // this initializes the TExaS grader lab 8
  UART_Init();                           // initialize UART for printing
	PLL_Init();                            // initialize PLL
	Timer0Capture_Init();                  // initialize echo timer
	Init_PortE();                          
  EnableInterrupts();                    // the grader needs interrupts
	PE1 = 0x00;                            // Start Trigger Low
	
	
  while(1) {
		
		// Send Trigger
		Trigger_Pulse();

		
		//Calculate Distance
		Calculate_Distance(); 

		
		
		// Print Distance
    printf("\nDistance = %d cm\n",  Distance);
  }
}


// Send Trigger Pulse
void Trigger_Pulse(void)
{
	timer1A_delayus(60000);   // Wait 60 ms
	PE1 = 0x02;               // Start Trigger High
	timer1A_delayus(10);      // Wait 10 us
	PE1 = 0x00;               // Set Trigger Low
}


void Calculate_Distance(void)
{	

	// Find Period of Time
  period = Timer0A_periodCapture();

	// Calculate Distance in cm
	Distance = (period*0.034)/200;
}



// Measure the time difference between the rising edge and falling edge of the ECHO pin
uint32_t Timer0A_periodCapture(void)
{
	
int risingEdge, fallingEdge;

TIMER0_ICR_R = 4;                 // clear timer0A capture flag 
	
while((TIMER0_RIS_R & 4) == 0)
;                                 // wait till captured 

risingEdge = TIMER0_TAR_R;        // save the timestamp 

TIMER0_ICR_R = 4;                 // clear timer0A capture flag 

while((TIMER0_RIS_R & 4) == 0)
;                                 // wait till captured 

fallingEdge = TIMER0_TAR_R;       // save the timestamp 

return (fallingEdge - risingEdge); // return the time difference 
}



/* millisecond delay using one-shot mode */
void timer1A_delayus(int ttime)
{ 
SYSCTL_RCGCTIMER_R |= 2;             // (1) enable clock to Timer Block 0 
TIMER1_CTL_R = 0;                    // (2) disable Timer before initialization 
TIMER1_CFG_R = 0x00;                 // (3) 32-bit option 
TIMER1_TAMR_R = 0x01;                // (4) one-shot mode and down-counter 
TIMER1_TAILR_R = 80 * ttime - 1;     // (5) Timer A interval load value register
TIMER1_ICR_R = 0x1;                  // (6) clear the TimerA timeout flag
TIMER1_CTL_R |= 0x01;                // (7) enable Timer A after initialization
while( (TIMER1_RIS_R & 0x1) == 0);   // (8) wait for TimerA timeout flag to set
}





/* Functions to initialize Timer0A for edge-time capture mode to
* measure the period of a square-wave input signal
*
* square wave signal should be fed to PB6 pin.
* Make sure it is 3.3 to 5V peak-to-peak.
* Initialize Timer0A in edge-time mode to capture rising edges.
* The input pin of Timer0A is PB6.
*
*/
void Timer0Capture_Init(void)
{
SYSCTL_RCGCTIMER_R |= 1;          // enable clock to Timer Block 0 
SYSCTL_RCGC2_R |= 2;              // enable clock to PORTB 
GPIO_PORTB_DIR_R &= ~0x40;        // make PB6 an input pin 
GPIO_PORTB_DEN_R |= 0x40;         // make PB6 as digital pin 
GPIO_PORTB_AFSEL_R |= 0x40;       // use PB6 alternate function 
GPIO_PORTB_PCTL_R &= ~0x0F000000; // configure PB6 for T0CCP0 
GPIO_PORTB_PCTL_R |= 0x07000000;
TIMER0_CTL_R &= ~1;               // disable timer0A during setup 
TIMER0_CFG_R = 4;                 // 16-bit timer mode 
TIMER0_TAMR_R = 0x17;             // up-count, edge-time, capture mode 
TIMER0_CTL_R |= 0x0C;             // capture the both edges to start
TIMER0_CTL_R |= 1;                // enable timer0A 
}


// Init PortE
void Init_PortE(void){
  SYSCTL_RCGC2_R |= 0x10;           // Port E clock
  delay = SYSCTL_RCGC2_R;           // wait 3-5 bus cycles
  GPIO_PORTE_DIR_R |= 0x02;         // PE1 output
  GPIO_PORTE_AFSEL_R &= ~0x02;      // not alternative
  GPIO_PORTE_AMSEL_R &= ~0x02;      // no analog
  GPIO_PORTE_DEN_R |= 0x02;         // enable PE1
}
