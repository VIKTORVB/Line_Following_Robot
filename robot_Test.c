/* ENG510
 * Line Following Robot
 * Group 172
 * Adam Horvath and Viktor Bojkov
 * PIC18F252
 * MPLAB / XC8 
 */
/////////////////////////
#include <pic18f252.h>
#include <xc.h>
#include <stdlib.h>
#include <stdio.h>
/////////////////////////
#pragma config OSC = HS // Oscillator = Set High Speed
#pragma config PWRT = OFF //Power Up Timer disabled
#pragma config WDT = OFF // Watchdog timer disabled
#pragma config LVP = OFF // Low Voltage ICSP disabled
/////////////////////////
#define _XTAL_FREQ 24000000	//Define PIC clock frequency 
/////////////////////////
#define leftPower     PORTBbits.RB0 //Define left motor I/O on PortB pin0
#define leftPhase     PORTBbits.RB1 //Define left motor phase on PortB pin1
#define rightPower    PORTBbits.RB2 //Define right motor I/O on PortB pin2
#define rightPhase    PORTBbits.RB3 //Define right motor phase on PortB pin3
#define leftLed       PORTBbits.RB7	//Define LED state indicator on PortB pin7
#define midLed        PORTBbits.RB6 //Define LED state indicator on PortB pin6
#define rightLed      PORTBbits.RB5 //Define LED state indicator on PortB pin5
/////////ADC initialising/////////
/*For correct A/D conversions, the A/D conversion clock (TAD) must be selected 
to ensure a minimum TAD time of 1.6 us. That’s why we use fosc/32, because (1/24Mhz)/32= 1.3 us
*/
void ADC_Init(){
    ADCON0 = 0x81;   //ADC on Fosc/32
    ADCON1 = 0x00;   //ADFM=0, Fosc/32, internal reference voltage
}
///////ADC channel selection///////
/*TACQ = Amplifier Settling Time + Holding Capacitor Charging Time + Temperature Coefficient
TACQ = TAMP + TC + TCOFF
Temperature coefficient is only required for temperatures > 25°C.
TACQ = 2 us + TC + [(Temp – 25°C)(0.05 µs/°C)]
TC =	 -CHOLD (RIC + RSS + RS) ln(1/2048)
-120 pF (1 kO + 7 kO + 33 kO) ln(0.0004883)		
-120 pF (41 kO) ln(0.0004883)
37.51 us		
TACQ = 2 us + 37.51 us + [(50°C – 25°C)(0.05 us/°C)]
  39.51 us + 1.25 us
  40.76 us	
*/
unsigned int ADC_Read(unsigned int channel){
    ADCON0=(ADCON0&0x81)|(channel<<3);  //clear current channel select and select new channel
    __delay_us(40);  //charging the holding capacitor
    GO_DONE=1;      //start ADC
    while(GO_DONE)continue; 
    unsigned int result = (ADRESH); //save results in ADRESH
    return result;
}
/////////Delay function///////////
void Timer2Delay(unsigned int delay){
/*Delay calculation for 10 ms
Tint = 10ms
fint = 1/Tin t = 100 Hz
PR2 = [(fosc/fint)/4]-1 => [24MHz/100Hz/4]-1 = 59999
59999/255 = 235.29 and AxB>235.29 (where A is the pre-scaler and B is the port-scaler)
Setting A 16 and B 15 gives us 240 which is bigger than 235.29
PR2=(24MHz/100Hz)/(4*16*15)-1=249
*/
    int i;	   //integer used for for loop
    T2CON=0x7B;    //Pre-scaler 1:16 and post-scaler is 1:15
    TMR2=0x00;     //Count from
    PR2=249;       //Count up to
    //Running for loop        
    for(i=0;i<delay;i++){     //Loop once to increase the delay
        T2CONbits.TMR2ON=1; //Run timer
        while(PIR1bits.TMR2IF==0);  //When flag is set
        T2CONbits.TMR2ON=0;         //Turn off timer
        PIR1bits.TMR2IF=0;         //Clear flag, loop back
    }
}
//////////////////////////////////
void main(void) {
// dump to terminal
RCSTA = 0x00;//clear both registers
TXSTA = 0x00;
SPBRG = 0x26;//9600 baud rate
RCSTA = 0b10000000;// enable serial port
TXSTA = 0b00100000;//enable transmit and set to asynchronous mode
Timer2Delay(1);

unsigned int IRSL;  //readings for left sensor
unsigned int IRSM;  //reading for mid sensor
unsigned int IRSR;  //readings for right sensor
TRISA = 0xFF;   //Port A input
TRISB = 0x00;   //Port B output
ADC_Init();     //Initialise ADC
    while(1){//infinite loop
		int i = 0;	//flag for error handling 
        
		leftLed = 0;	//set initial states of the LEDs
        midLed = 0;
        rightLed = 0;

        IRSR=ADC_Read(0);   //readings from ch0 saved at IRSR
        IRSM=ADC_Read(1);   //readings from ch1 saved at IRSM
        IRSL=ADC_Read(2);   //readings from ch2 saved at IRSL
// dump to terminal
     	TXREG = IRSR;
 /*Range calculations
Resolution is 8-bit, Vref is 5V and 0V.
Digital value = [Analogue value/(Vref+ - Vref-)] * (2^resolution)
Digital value = [ Analogue value/5V]*255
Analogue value (all the readings were taken with the use of the oscilloscope)
Middle sensor=4.2V / 0.2V 
Left sensor = 3.4V / 0.2V
Right Sensor = 3.12V / 0.2V
Digital range for mid sensor is = 214 – 10 //214 being fully on the black line
Digital range for mid sensor is = 174 – 10 //174 being fully on the black line
Digital range for mid sensor is = 160 – 10 //160 being fully on the black line
Measured range: 
Middle sensor digital range = DC - 09 // 220 – 9
Left sensor digital range = B4 - 08 // 180 - 8
Right sensor digital range = 9D – 08 // 157 - 8 
*/           
/* Delay testing, toogle pin 4 on port B
PORTBbits.RB4 = 1;
Timer2Delay(1);
PORTBbits.RB4 = 0;
*/	
        if((IRSL<30)&(IRSM>60)&(IRSR<30)){//Forward
                    midLed = 1;	//Turn on mid LED to indicate the Forward State
                	leftPower=0;
        			leftPhase=1;
        			rightPower=0;
       	    		rightPhase=1;
              Timer2Delay(10);
			}
	    if((IRSL>50)&(IRSM>50)&(IRSR>50)){//Forward crossroad
                    midLed = 1;		//Turn on all the LEDs to indicate the Forward Crossroad State
                    leftLed = 1;
                    rightLed = 1;
           	        leftPower=0;
        			leftPhase=1;
        			rightPower=0;
       	    		rightPhase=1;
              Timer2Delay(5);
			}
        if((IRSL>80)&(IRSM<50)&(IRSR<50)){//Left-sharp
                    leftLed = 1;	//Turn on left LED to indicate the Left Turn State
                	leftPower=0;
        			leftPhase=0;
        			rightPower=0;
       	    		rightPhase=1;
					int l=0;
             Timer2Delay(5);
			}
        if((IRSL>100)&(IRSM>100)&(IRSR<50)){//Left-smooth
                    leftLed = 1; 	//Turn on left LED to indicate the Left Turn State
                    midLed = 1;
                	leftPower=1;
        			leftPhase=0;
        			rightPower=0;
       	    		rightPhase=1;
	      Timer2Delay(1);
			}
	    if ((IRSL<50)&(IRSM<50)&(IRSR>80)){//Right-sharp
                    rightLed = 1; 	//Turn on left LED to indicate the Right Turn State
           	       	leftPower=0;
                    leftPhase=1;
        	        rightPower=0;
       	    	    rightPhase=0;
            Timer2Delay(5);
			}
	    if ((IRSL<50)&(IRSM>100)&(IRSR>100)){//Right-smooth
                    rightLed = 1; 	//Turn on right LED to indicate the Right Turn State
                    midLed = 1;
           	       	leftPower=0;
                    leftPhase=1;
        	        rightPower=1;
       	    	    rightPhase=0;
	     Timer2Delay(1);
			}
 	    if ((IRSL<10)&(IRSM<10)&(IRSR<10)){//Stop
           	       leftPower=1;
                   leftPhase=1;
        	       rightPower=1;
       	           rightPhase=1;
                   i=1;
	     Timer2Delay(1);
			}
       //Error handling
		 if(i==1) // when flag is set, reverse, turn slightly right
        {
                leftPower=0;
        		leftPhase=0;
        		rightPower=0;
       	    	rightPhase=0;
           Timer2Delay(5);
				leftPower=0;
                leftPhase=1;
        	    rightPower=0;
       	        rightPhase=0;
		   Timer2Delay(2);
			}
        } 
   return;
}


