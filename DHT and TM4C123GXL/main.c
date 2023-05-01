// ***** 0. Documentation Section *****
// main.c
//     Runs on TM4C123
//     UART runs at 115,200 baud rate 
// DHT11 interfacing with TI launchpad

// ***** 1. Pre-processor Directives Section *****
#include <stdio.h>   // standard C library 
#include <stdint.h>
#include "uart.h"    // functions to implment input/output
#include "TExaS.h"   // Lab grader functions
#include "tm4c123gh6pm.h"
#include "SysTick.h"
#include "PLL.h"
#include "PLL.c"
#include "SysTick.c"

// ***** 2. Global Declarations Section *****

// FUNCTION PROTOTYPES: Each subroutine defined

void DHT_Init(void);

void EnableInterrupts(void);  // Enable interrupts

// ***** 3. Subroutines Section *****
int main (void) {
 
	unsigned long timeout1, timeout2;
	
	  enum SENSOR{DHT11, DHT22};
    enum SENSOR sensor;
    char num[10];
    long dl[5];
    long checksum;
    long RH, T, RHD, TD;
    double rh, tc, tf;
    int time;
    int byte, bit;
		
		
  TExaS_Init(UART_PIN_PA0,UART_PIN_PA1); // this initializes the TExaS grader lab 5
  UART_Init();    // initialize UART for printing
	PLL_Init1();
	SysTick_Init();
	DHT_Init();
	
  printf("\nThis program outputs Temperture and relative humidity.\n");
  EnableInterrupts();  // the grader needs interrupts
  while(1) 
	{
    
		//SysTick_Wait10ms(200);
    //trigger with high then 20mS low for DHT11 or 1 to 10mS for DHT22 then
    //switch to input making like high from pull up resistor
    //DHT responds with 80uS low followed by 80uS high
    GPIO_PORTA_DIR_R |=0x04;                                             //PORTA PA2 bi directional pin as output
    GPIO_PORTA_DATA_R &= ~0x04;                                        //Set PA2 low
    SysTick_Wait10ms(2);                                                    //wait 20 mS  (20mS low pulse) for DHT11        Both work fine with 30mS pulse
    GPIO_PORTA_DATA_R |= 0x04;       //Make PA2 high  works better going straight to input and not driving the line high
    GPIO_PORTA_DIR_R &= ~0x04;                                        //Make PA2 input (line high)
    SysTick_Wait10us(4);                                                    //wait 40 uS  for DHT to switch to output
    
		    //use counter to identify if there was a disconnect and terminate the collection process
        timeout1 = timeout2 = 50000;
        while(((GPIO_PORTA_DATA_R & 0x04)==0) && timeout1 > 0)                      //wait remainder of 80uS low from DHT
        {
            timeout1--;
        }   
        while(((GPIO_PORTA_DATA_R & 0x04)==0x04) && timeout2 > 0 && timeout1 > 0)                //wait for 80uS high from DHT
        {
            timeout2--;
        }       

        //start of data
        //each bit begins with 50uS low followed by high data bit
        //high data bit of 26 to 28uS is a 0
        //high data bit of 70uS is a 1
        //8 bit integer and 8 bit decimal relative humitity
        //8 bit integer and 8 bit decimal temperature
        //8 bit check sum = 8 bit IRH + 8 bit DRH + 8 bit IT + 8 bit DT
        dl[0]= dl[1] = dl[2] = dl[3] = 0;
				
        if(timeout1 > 0  && timeout2 > 0)
        {
            for(byte = 0; byte < 5; byte++)        //process four sets of data, the fifth check sum
            {
                for(bit = 0; bit < 8; bit++)      //process the eight bits for each data byte
                {
                //again, add a counter to identify if there is a disconnect
                    timeout1 = 10000;
                    while(((GPIO_PORTA_DATA_R & 0x04)==0)&& timeout1 > 0)              //wait 50uS for digit low
                    {
                        timeout1--;
                    }       
                    time = 0;                                         //initialize a time counter to identify if the bit is a 0 or a 1
                    while(((GPIO_PORTA_DATA_R & 0x04)==0x04) && time < 100)                //wait while DHT signal is high (count the uS of the high time)
                    {
                       SysTick_Wait1us(1);                                                    //wait 1 uS
                       time++;
                    }
                    dl[byte] = dl[byte] << 1;
                    if(time > 30)                                      //a 0 is 26-28 uS and a 1 is 70 uS
                    {
                            dl[byte] |= 0x01;
                    }
                }
            }
        }
       
        checksum = (dl[0] + dl[1] + dl[2] + dl[3]) &0xFF;        //calculate checksum
        if((dl[4] &0xFF) == checksum)                                                //check checksum
        {

            if(sensor == DHT22)
            {
                //DHT 22
                RH = dl[0] << 8 | (dl[1] & 0xFF);                                    //put two bytes together into 16 bit value
                T =  dl[2] << 8 | (dl[3] & 0xFF);
                rh = (double)RH/10;                                                  //convert to real and adjust by dividing by 10
                tc = (double)T/10;
            }
            else
            {               
                //DHT11
                tc = (double)dl[2];
                if(dl[3] & 0x80)
                {
                    tc = -1 - tc;
                }
                tc += (double)(dl[3]&0x0F) * 0.1;
                rh = (double) dl[0] + (double) dl[1] *0.1;
        }
           
//        All DHT           
            tf = tc*9/5 + 32;                                                                    //convert to F
            RH = (long)rh;                                                                        //get integer part of RH
//           T = (long)tc;                                                                            //get integer part of T
          T = (long)tf;                                                                            //get integer part of T
            RHD = (long)((rh - (double)RH)*10);                                //extract and convert decimal part to an integer
            TD = (long)((tf - (double)T)*10);
	
	}
				
	printf("\n Temperature (in F): %lf F \n", tf);
	printf("\n Relative humidity : %lu %% \n", RH);

	printf(" ~~~~~~~~~~~~~~~~~~~~~~~~~~~ \n");
	
	
	SysTick_Wait10ms(200);  
}
	
}

// Initialization for Port A 2 for DHT11

void DHT_Init(void)
{
  volatile unsigned long delay;

	SYSCTL_RCGC2_R |= 0x01;							//PORTA clock
    delay = SYSCTL_RCGC2_R;     				// 1a)wait for clock to finish   
	GPIO_PORTA_DIR_R |=0x04; 						//PORTA PA2 bi directional pin initialized as output
	GPIO_PORTA_DEN_R |=0x04; 						//PORTA PA2 as digital
	GPIO_PORTA_DATA_R |= 0x04;					//Set PA2 high
	SysTick_Wait10us(100);							//wait 
}
//

