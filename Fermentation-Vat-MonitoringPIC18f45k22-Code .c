
//Preprocessor------------------------------------------------------------------------------


#include "pragmas.h"	
#include <stdlib.h>     
#include <stdio.h>	
#include <p18f45k22.h>
#include <adc.h>
#include <usart.h>      

// Constants ---------------------------------------------------------------------------------
#define ON 		1
#define OFF		0
#define TRUE	 1	
#define	FALSE 	0	
#define LOW 0
#define HIGH 1

#define BITCONVERSION 5/1024

#define TMRFLAG INTCONbits.TMR0IF 

#define OFFSETTEMP 1.4285 
#define COEFFICIENTTEMP 0.0285
#define COEFFICIENTPRESS 0.02
#define OFFSETCO	0.556
#define COEFFICIENTCO	0.00278

#define SAMPLESIZE 30
#define ADRSSIZE 4
#define AVGTIME 120//30 secs-120
#define BUFFERSIZE	20

#define TEMPCHANNEL 0
#define PRESSCHANNEL 1
#define COCHANNEL 2

#define HIGHTEMP_LED PORTDbits.RD2
#define LOWTEMP_LED PORTDbits.RD3
#define HIGHPRESS_LED PORTCbits.RC4
#define LOWPRESS_LED PORTCbits.RC5

#define MODE					PORTAbits.RA3
#define CHANNEL					PORTAbits.RA4
#define INCREMENT				PORTAbits.RA5
#define DECREMENT				PORTEbits.RE0  

#define HEATER					PORTEbits.RE1  

#define ALARM					PORTAbits.RA6

#define MOTOR1LED1					PORTDbits.RD4
#define MOTOR1LED2					PORTDbits.RD5
#define MOTOR1LED3					PORTBbits.RB1
#define MOTOR1LED4					PORTBbits.RB0

#define MOTOR2LED1					PORTBbits.RB2
#define MOTOR2LED2					PORTBbits.RB3
#define MOTOR2LED3					PORTBbits.RB4
#define MOTOR2LED4					PORTBbits.RB5


#define TEMPSTATUS_SAFE				((FVS.temperature.averageSample <= FVS.temperature.highLimit)&&(FVS.temperature.averageSample >= FVS.temperature.lowLimit))
#define PRESSSTATUS_SAFE			((FVS.pressure.averageSample <= FVS.pressure.highLimit)&&(FVS.pressure.averageSample >= FVS.pressure.lowLimit))

#define HIGHTEMP 65
#define LOWTEMP 12
#define HIGHPRESS 103
#define LOWPRESS 17
#define HIGHCO	1200
#define LOWCO 350

#define ADDRFVS "764"
#define CONTROLLER 1
#define MYADDR	764


// Global Variables --------------------------------------------------------------------------


char clearScreen[]={"\033[2J\033[H"};
char alarm[] = ("Alarm");
char safe[] = ("Safe");
char high[]=("HIGH");
char low[]=("LOW");
char tempChannel[]=("TEMPERATURE");
char pressChannel[]=("PRESSURE");
char coChannel[]=("CO2");		
char RUN[]=("RUN");
char STOP[]=("STOP");
char buffer[BUFFERSIZE];

char motorPattern[]={0x01,0x02,0x04,0x08};
char motor1Status = OFF;
char avgFlag = FALSE;
char buffCount =0;
char buffFlag= FALSE;
// Monitorng System Elements-------------------------------------------------------------------
typedef struct sensorChannel
{
	int currentSample;
	int samples[SAMPLESIZE];
	int averageSample;
	unsigned char insertAt;
	int highLimit;
	int lowLimit;
	char state;	
}sensorChannel_t; 		//eo sensorChannel

typedef struct motor
{
	int position;
	int currentPosition;
	char pattern;
	unsigned char patternCounter;

}	motor_t;

typedef struct FVS
{
	char address[ADRSSIZE];
	sensorChannel_t temperature;
	sensorChannel_t	pressure;
	sensorChannel_t co2;
	motor_t motor1;
	motor_t motor2;
	char modeState;
	char channelState;
	char indicator;
	char heater;
	
} FVS_t;			//eo FVS

FVS_t FVS;// creating a structure 

// Functions ---------------------------------------------------------------------------------



/*--- set_osc__4MHz ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		19/01/2020
Desc:		Setting the oscillator to operate at 4MHz
Input: 		none
Returns:	none

--------------------------------------------------------------------------------------------*/

void set_osc__4MHz(void)
{
	OSCCON =  0x53; 			// Sleep on sleepp cmd,  4MHz
             
	OSCCON2 = 0x04;			


	OSCTUNE = 0x80;				
						 
	while (OSCCONbits.HFIOFS != 1); 	// wait for osccillator to become stable
}
//eo: set_osc__4MHz


		// --- port configuration ---
/*--- configPort ---------------------------------------------------------------------------
Author:		ABDUL BAASIT
Date:		19/01/2020
Desc:		Set port cofigurations.
Input: 		none
Returns:	none

--------------------------------------------------------------------------------------------*/

void configPort(void)
{
	
	ANSELA 				= 0X07; //configuring the pin RA0,RA1 as analog and the other pins as digital.
	LATA   				= 0X00; //clear output 
	TRISA               = 0X3F;
	
	ANSELB				= 0x00;
	LATB				= 0x00;
	TRISB				= 0x00;

 	ANSELC 				= 0X00;  // No Analog inputs
	LATC 				= 0X00; // clear output 
	TRISC				= 0x00;
	 
	ANSELD 				= 0x00;
	LATD				= 0x00;
	TRISD				= 0x00;

	ANSELE				= 0x00;
	LATE				= 0x00;
	TRISE				= 0x01;
	


}		//eo:configPort


		// --- serial communication ---
//*--- configSerial ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		19/01/2020
Desc:		Configuring pic for serial port-1 configuration
Input: 		none
Returns:	none

--------------------------------------------------------------------------------------------*/

void configSerial(void)
{
	SPBRG 		= 25;      //  Set baud rate value at 4Mhz
	RCSTA1 		= 0X90;   //  Receiver Status Register
	TXSTA1 		= 0X26;   //   Transmitter Status Register
	BAUDCON1 	= 0X40;  //  Baud rate generator
}		//eo:configSerial



		// --- ADC configuration ---
/*--- configadc ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		19/01/2020
Modified:	None
Desc:		Set analog to digital configuration
Input: 		none
Returns:	none

--------------------------------------------------------------------------------------------*/
void configadc (void)
{
 	ADCON0=0x01;			//enable ADC
 	ADCON1=0x00;
 	ADCON2=0xAA;
}		//eo:configadc


		// --- reset timer ---
/*--- resetTmr0 ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		19/01/2020
Modified:	None
Desc:		reset the timer to false start
Input: 		none
Returns:	none

--------------------------------------------------------------------------------------------*/

void resetTmr0(void)
{
	TMR0H = 0x0B;
	TMR0L = 0xDC;
	TMRFLAG = FALSE;
} 		//eo:resetTmr0


		// --- timer configuration ---
/*--- timerConfig ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		19/01/2020
Modified:	None
Desc:		setting the timer register for 0.25 second rollover
Input: 		none
Returns:	none

--------------------------------------------------------------------------------------------*/

void timerConfig(void)
{
	resetTmr0();
	T0CON = 0x91;//93 for 1-sec, 91-.25sec

}		//eo:timerConfig


		// --- getting samples ---
/*--- getSampleTemp ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		19/01/2020
Modified:	None
Desc:		analog to digital conversion for temperature sensor(potentiometer) 
Input: 		none
Returns:	float, returns a value in floating point post conversion.

--------------------------------------------------------------------------------------------*/

float getSampleTemp(void)
{
	float valueTemp = 0;
	ADCON0 = 0x01;
	ADCON0bits.GO = TRUE;
	while(ADCON0bits.GO == TRUE);
	valueTemp = ADRES;
	valueTemp = valueTemp * BITCONVERSION;
	valueTemp = valueTemp - OFFSETTEMP;
	valueTemp = valueTemp / COEFFICIENTTEMP;	
	return valueTemp;		
}		//eo:getSampleTemp


		// --- getting samples of pressure ---
/*--- getSampleADCPressure ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		19/01/2020
Modified:	None
Desc:		analog to digital conversion of pressure sensor(potentiometer)
Input: 		none
Returns:	float, returns a value in floating point post conversion.

--------------------------------------------------------------------------------------------*/

float getSampleADCPressure(void)
{
	float value = 0;
	ADCON0 = 0x05;
	ADCON0bits.GO = TRUE;
	while(ADCON0bits.GO == TRUE);
	value = ADRES;
	value = value * BITCONVERSION;
	value = value / COEFFICIENTPRESS;	
	return value;		
}		//eo:getSampleADCPressure

/*--- getSampleADCO2 ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		19/01/2020
Modified:	None
Desc:		analog to digital conversion of CO2 sensor(potentiometer)
Input: 		none
Returns:	float, returns a value in floating point post conversion.

--------------------------------------------------------------------------------------------*/

float getSampleADCO2(void)
{
	float value = 0;
	ADCON0 = 0x09;
	ADCON0bits.GO = TRUE;
	while(ADCON0bits.GO == TRUE);
	value = ADRES;
	value = value * BITCONVERSION;
	value = value - OFFSETCO;
	value = value / COEFFICIENTCO;	
	return value;		
}		//eo:getSampleADCO2





		// --- initializing structure variables ---
/*--- structureInitialization ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		19/01/2020
Modified:	None
Desc:		initializing the stucture members
Input: 		none
Returns:	none
--------------------------------------------------------------------------------------------*/

void structureInitialization(void)
{
	int i =0;
	

	FVS.temperature.currentSample =0;
	for( i=0; i < SAMPLESIZE; i++)
	{
		FVS.temperature.samples[i] = 0;
	}	
	FVS.temperature.averageSample =0;
	FVS.temperature.insertAt =0;
	FVS.temperature.highLimit = HIGHTEMP;
	FVS.temperature.lowLimit = LOWTEMP;
	FVS.temperature.state =0;
	

	FVS.pressure.currentSample =0;
	for(i=0; i < SAMPLESIZE; i++)
	{
		FVS.pressure.samples[i] =0;
	}
	FVS.pressure.averageSample = 0;
	FVS.pressure.insertAt = 0;
	FVS.pressure.highLimit = HIGHPRESS;
	FVS.pressure.lowLimit = LOWPRESS;
	FVS.pressure.state =0;
	

	FVS.co2.currentSample =0;
	for(i=0; i < SAMPLESIZE; i++)
	{
		FVS.co2.samples[i] =0;
	}
	FVS.co2.averageSample = 0;
	FVS.co2.insertAt = 0;
	FVS.co2.highLimit = HIGHCO;
	FVS.co2.lowLimit = LOWCO;
	FVS.co2.state =0;
	
	FVS.indicator = 0;
	FVS.heater = 0;

	
	FVS.motor1.position=0;
	FVS.motor1.currentPosition = 0;
	FVS.motor1.pattern		 = 0;
	FVS.motor1.patternCounter = 0;
	
	FVS.motor2.position=0;
	FVS.motor2.currentPosition = 0;
	FVS.motor2.pattern		 = 0;
	FVS.motor2.patternCounter = 0;
	
	FVS.modeState = 0;
	FVS.channelState = 0;
	
}		//eo:structureInitialization


/*--- mode ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		24/01/2020
Modified:	None
Desc:		setting the pushbutton to change the mode
Input: 		none
Returns:	none
--------------------------------------------------------------------------------------------*/

void mode(void)
{
	if(MODE == FALSE)
	{
		FVS.modeState = (!FVS.modeState);
		
			
	}
}  // eo: mode

/*--- chanelSelect ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		24/01/2020
Modified:	None
Desc:		setting the pushbutton to toggle the channels
Input: 		none
Returns:	none
--------------------------------------------------------------------------------------------*/

void chanelSelect(void)
{
	if(CHANNEL == FALSE)
	{
		FVS.channelState++;
		if(FVS.channelState ==3)
		{
			FVS.channelState=0;
		}
		while(!CHANNEL);
	}	
}  //eo: chanelSelect

/*--- incrementOperation ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		25/01/2020
Modified:	None
Desc:		function to increase the value by one for every push
Input: 		none
Returns:	none
--------------------------------------------------------------------------------------------*/

void incrementOperation(void)
{
	if(INCREMENT == FALSE)
	{
		buffFlag=TRUE;
		buffCount=0;		
	
		if( (FVS.modeState == LOW) && (FVS.channelState == TEMPCHANNEL) && (FVS.temperature.lowLimit < FVS.temperature.highLimit) )
		{
			FVS.temperature.lowLimit += 1;	
		}
		else if( (FVS.modeState == LOW) && (FVS.channelState == PRESSCHANNEL) && (FVS.pressure.lowLimit < FVS.pressure.highLimit))
		{
			FVS.pressure.lowLimit += 1;		
		}
		else if( (FVS.modeState == LOW) && (FVS.channelState == COCHANNEL) && (FVS.co2.lowLimit < FVS.co2.highLimit))
		{
			FVS.co2.lowLimit += 1;		
		}
		
		else if(FVS.modeState == HIGH && FVS.channelState == TEMPCHANNEL)
		{
			FVS.temperature.highLimit +=1;		
		}
		
		else if( FVS.modeState == HIGH && FVS.channelState == PRESSCHANNEL)
		{
			FVS.pressure.highLimit +=1;

		}
		else
		{
			FVS.co2.highLimit +=1;
		}
		while(!INCREMENT);
	}
		
}   // eo: incrementOperation

/*--- decrementOperation ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		25/01/2020
Modified:	None
Desc:		function to decrease the value by one for every push
Input: 		none
Returns:	none
--------------------------------------------------------------------------------------------*/

void decrementOperation(void)
{
	if(DECREMENT == FALSE )
	{		
		buffFlag=TRUE;
		buffCount=0;
		
			if( FVS.modeState == LOW && FVS.channelState == TEMPCHANNEL  )
			{
				FVS.temperature.lowLimit -=1;
				
			
			}		
			
			else if(FVS.modeState == LOW && FVS.channelState == PRESSCHANNEL)
			{
				FVS.pressure.lowLimit -= 1;
				
				
			}

			else if(FVS.modeState == LOW && FVS.channelState == COCHANNEL)
			{
				FVS.co2.lowLimit -= 1;
				
				
			}
			
			else if(FVS.modeState == HIGH && FVS.channelState == TEMPCHANNEL && (FVS.temperature.lowLimit < FVS.temperature.highLimit))
			{
				FVS.temperature.highLimit -=1;
				
				
			}
			
			else if(FVS.modeState == HIGH && FVS.channelState == PRESSCHANNEL && (FVS.pressure.lowLimit < FVS.pressure.highLimit))
			{
				FVS.pressure.highLimit -=1;
				
				
			}
			
			else if(FVS.modeState == HIGH && FVS.channelState == COCHANNEL && (FVS.co2.lowLimit < FVS.pressure.highLimit))
			{
				FVS.co2.highLimit -=1;
				
				
			}
		while(!DECREMENT);
		
	}			

} // eo: decrementOperation

/*--- motor1LEDS ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		28/01/2020
Modified:	None
Desc:		leds representing the motor operation
Input: 		none
Returns:	none
--------------------------------------------------------------------------------------------*/

void motor1LEDS(void)
{
	if(motor1Status)
	{
		if(FVS.motor1.position > FVS.motor1.currentPosition)
		{
			FVS.motor1.pattern = motorPattern[FVS.motor1.patternCounter];
			
			if(FVS.motor1.pattern == 1)
			{
				MOTOR1LED1 = ON;
				MOTOR1LED2 = OFF;
				MOTOR1LED3 = OFF;
				MOTOR1LED4 = OFF;
			}
			else if(FVS.motor1.pattern == 2)
			{
				MOTOR1LED1 = OFF;
				MOTOR1LED2 = ON;
				MOTOR1LED3 = OFF;
				MOTOR1LED4 = OFF;
			}
			else if(FVS.motor1.pattern == 4 )
			{
				MOTOR1LED1 = OFF;
				MOTOR1LED2 = OFF;
				MOTOR1LED3 = ON;
				MOTOR1LED4 = OFF;
			}
			else if(FVS.motor1.pattern == 8 )
			{
				MOTOR1LED1 = OFF;
				MOTOR1LED2 = OFF;
				MOTOR1LED3 = OFF;
				MOTOR1LED4 = ON;
			}
			else
			{
				MOTOR1LED1 = OFF;
				MOTOR1LED2 = OFF;
				MOTOR1LED3 = OFF;
				MOTOR1LED4 = OFF;
			}
			FVS.motor1.currentPosition = FVS.motor1.currentPosition +3; 
			FVS.motor1.patternCounter++;
			if(FVS.motor1.patternCounter>3)
			{
				FVS.motor1.patternCounter=0;
			}
			if(FVS.motor1.position == FVS.motor1.currentPosition)
			{
				FVS.motor1.currentPosition =0;
			}
		}
	}
	else
	{
		MOTOR1LED1 = OFF;
		MOTOR1LED2 = OFF;
		MOTOR1LED3 = OFF;
		MOTOR1LED4 = OFF;

	}
}

/*--- motor2LEDS ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		28/01/2020
Modified:	None
Desc:		leds representing the motor operation
Input: 		none
Returns:	none
--------------------------------------------------------------------------------------------*/
void motor2LEDS(void)
{
	if(FVS.co2.state == TRUE)
	{
		if(FVS.motor2.position  > FVS.motor2.currentPosition)
		{
			FVS.motor2.pattern = motorPattern[FVS.motor2.patternCounter];
			
			if(FVS.motor2.pattern == 1)
			{
				MOTOR2LED1 = ON;
				MOTOR2LED2 = OFF;
				MOTOR2LED3 = OFF;
				MOTOR2LED4 = OFF;
			}
			else if(FVS.motor2.pattern == 2)
			{
				MOTOR2LED1 = OFF;
				MOTOR2LED2 = ON;
				MOTOR2LED3 = OFF;
				MOTOR2LED4 = OFF;
			}
			else if(FVS.motor2.pattern == 4 )
			{
				MOTOR2LED1 = OFF;
				MOTOR2LED2 = OFF;
				MOTOR2LED3 = ON;
				MOTOR2LED4 = OFF;
			}
			else if(FVS.motor2.pattern == 8 )
			{
				MOTOR2LED1 = OFF;
				MOTOR2LED2 = OFF;
				MOTOR2LED3 = OFF;
				MOTOR2LED4 = ON;
			}
			/*else
			{
				MOTOR2LED1 = OFF;
				MOTOR2LED2 = OFF;
				MOTOR2LED3 = OFF;
				MOTOR2LED4 = OFF;
			}*/
			FVS.motor2.currentPosition = FVS.motor2.currentPosition +3; 
			FVS.motor2.patternCounter++;
			if(FVS.motor2.patternCounter>3)
			{
				FVS.motor2.patternCounter=0;
			}
			if(FVS.motor2.position == FVS.motor2.currentPosition)
			{
				MOTOR2LED1 = OFF;
				MOTOR2LED2 = OFF;
				MOTOR2LED3 = OFF;
				MOTOR2LED4 = OFF;
			}
			
		}

		else if ((FVS.motor2.position == 0) && (FVS.motor2.currentPosition >0))
		{
			FVS.motor2.pattern = motorPattern[FVS.motor2.patternCounter];

			if(FVS.motor2.pattern == 1)
			{
				MOTOR2LED1 = ON;
				MOTOR2LED2 = OFF;
				MOTOR2LED3 = OFF;
				MOTOR2LED4 = OFF;
			}
			else if(FVS.motor2.pattern == 2)
			{
				MOTOR2LED1 = OFF;
				MOTOR2LED2 = ON;
				MOTOR2LED3 = OFF;
				MOTOR2LED4 = OFF;
			}
			else if(FVS.motor2.pattern == 4 )
			{
				MOTOR2LED1 = OFF;
				MOTOR2LED2 = OFF;
				MOTOR2LED3 = ON;
				MOTOR2LED4 = OFF;
			}
			else if(FVS.motor2.pattern == 8 )
			{
				MOTOR2LED1 = OFF;
				MOTOR2LED2 = OFF;
				MOTOR2LED3 = OFF;
				MOTOR2LED4 = ON;
			}

			FVS.motor2.currentPosition = FVS.motor2.currentPosition -3; 
			FVS.motor2.patternCounter--;
			if(FVS.motor2.patternCounter == -1)
			{
				FVS.motor2.patternCounter=3;
			}
			if(FVS.motor2.position == FVS.motor2.currentPosition)
			{
				FVS.motor2.currentPosition =0;
				MOTOR2LED1 = OFF;
				MOTOR2LED2 = OFF;
				MOTOR2LED3 = OFF;
				MOTOR2LED4 = OFF;
			}
			
		}
	}	
	
	else
	{
		MOTOR2LED1 = OFF;
		MOTOR2LED2 = OFF;
		MOTOR2LED3 = OFF;
		MOTOR2LED4 = OFF;

	}
}  // eo: motorLEDS

/*--- dampenerStatus ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		28/01/2020
Modified:	None
Desc:		based upon the values read from sensors the execution of dampner is decicded.
Input: 		none
Returns:	none
--------------------------------------------------------------------------------------------*/
void dampenerStatus(void)
{
		
		if(FVS.co2.averageSample > FVS.co2.highLimit)
				{
					FVS.co2.state = TRUE;
					FVS.motor2.position =30;
					ALARM = ON;	
					
					
				}  
				
				else if((FVS.co2.averageSample < FVS.co2.highLimit) &&(FVS.co2.averageSample > FVS.co2.lowLimit))
				{
					FVS.co2.state = TRUE;
					FVS.motor2.position =0;
					ALARM = OFF;	
				}
				
				else
				{
					FVS.co2.state = FALSE;
					ALARM = ON;
				
				}
		


}

		// --- function for temperature ---
/*--- tempStatus ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		20/01/2020
Modified:	None
Desc:		based upon the reading of temperature sensor the repesctive leds are turned on or off(representing
		motors) and status is updated whether it is safe or in alarm state.
Input: 		none
Returns:	none
--------------------------------------------------------------------------------------------*/


void tempStatus(void)
{
		if(FVS.temperature.averageSample <= 	FVS.temperature.lowLimit && avgFlag == TRUE)
			{
				LOWTEMP_LED = TRUE;
				HIGHTEMP_LED = FALSE;
				FVS.temperature.state = TRUE;
			}
			
			else if(FVS.temperature.averageSample >= FVS.temperature.highLimit && avgFlag == TRUE)
			{
				HIGHTEMP_LED = TRUE;
				LOWTEMP_LED  = FALSE;
				FVS.temperature.state = TRUE;
			}
			
			else if((FVS.temperature.averageSample >= FVS.temperature.lowLimit) && (FVS.temperature.averageSample <= FVS.temperature.highLimit) && avgFlag == TRUE)
			{
				HIGHTEMP_LED = FALSE;
				LOWTEMP_LED  = FALSE;
				FVS.temperature.state = FALSE;
				
			}
		
			else
			{
					HIGHTEMP_LED = FALSE;
				LOWTEMP_LED  = FALSE;
			}
		
		
}     //eo tempStatus



		// --- function for pressure ---
/*--- pressureStatus ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		20/01/2020
Modified:	None
Desc:		based upon the reading of pressure sensor the repesctive leds are turned on or off(representing
		motors) and status is updated whether it is safe or in alarm state.
Input: 		none
Returns:	none
--------------------------------------------------------------------------------------------*/
void pressureStatus(void)
{
		
							
			if(FVS.pressure.averageSample <= FVS.pressure.lowLimit && avgFlag == TRUE)
			{
				HIGHPRESS_LED	= FALSE;
				LOWPRESS_LED	= TRUE;
				FVS.pressure.state = TRUE;				
			}
			
			else if(FVS.pressure.averageSample >= FVS.pressure.highLimit && avgFlag == TRUE)
			{	
				HIGHPRESS_LED	= TRUE;
				LOWPRESS_LED	= FALSE;
				FVS.pressure.state = TRUE;
			}
			
			else if((FVS.pressure.averageSample >= FVS.pressure.lowLimit) && (FVS.pressure.averageSample <= FVS.pressure.highLimit && avgFlag == TRUE))
			{
				HIGHPRESS_LED	= FALSE;
				LOWPRESS_LED	= FALSE;
				FVS.pressure.state =FALSE;
			}

			else
			{
				HIGHPRESS_LED	= FALSE;
				LOWPRESS_LED	= FALSE;	
			}

		
}  // eo pressureStatus



/*--- motorOperation ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		28/01/2020
Modified:	None
Desc:		Based upon the readings from sensors and upon comparison with respective limits,
		heater, motor are tunred on or off(indicated by LED's).
Input: 		none
Returns:	none
--------------------------------------------------------------------------------------------*/
void systemOperation (void)
{

	
		if(((FVS.temperature.averageSample <= FVS.temperature.lowLimit) && (FVS.pressure.averageSample <= FVS.pressure.lowLimit)) || ((FVS.temperature.averageSample <= FVS.temperature.lowLimit) && PRESSSTATUS_SAFE))
		{	
						
			motor1Status = OFF;
			HEATER = ON;
			FVS.indicator = ON;
							
		}
					
		else if((FVS.temperature.averageSample <= FVS.temperature.lowLimit) && (FVS.pressure.averageSample >= FVS.pressure.highLimit))
		{
	
			motor1Status = ON;
			HEATER = OFF;
			FVS.indicator = OFF;
						
		}
					
		else if(((FVS.temperature.averageSample >= FVS.temperature.highLimit) && (FVS.pressure.averageSample >= FVS.pressure.highLimit)) || ((FVS.temperature.averageSample >= FVS.temperature.highLimit) && PRESSSTATUS_SAFE))
		{
			HEATER = OFF;
			FVS.indicator = OFF;
			motor1Status = ON;
			FVS.motor1.position = 360;			
			
		}		

		else if((FVS.temperature.averageSample >= FVS.temperature.highLimit) && (FVS.pressure.averageSample <= FVS.pressure.lowLimit))
		{
			HEATER = OFF;
			FVS.indicator = OFF;
			motor1Status = ON;
			FVS.motor1.position = 360;
		}		
				
		else if(TEMPSTATUS_SAFE && (FVS.pressure.averageSample <= FVS.pressure.lowLimit))
		{
								
			motor1Status= OFF;
			HEATER = FVS.indicator;
												
		}
						
		else if(TEMPSTATUS_SAFE && (FVS.pressure.averageSample >= FVS.pressure.highLimit))
		{			
				HEATER = OFF;
				motor1Status = ON;	
				FVS.motor1.position = 360;
		}
							
		else
		{
			motor1Status = OFF;
			HEATER = ON;		
		}	
			
		tempStatus();
		pressureStatus();
		dampenerStatus();
} // eo: motorOperation

/*--- calcCheckSum ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		19/01/2020
Desc:		calculates the checksum 
Input: 		pointer,address of the array
Returns:	char,numeric value
--------------------------------------------------------------------------------------------*/

char calcCheckSum(char *ptr)
{
	char checkSum = 0;	
	while(*ptr)
	{
		checkSum^= *ptr;
		ptr++;
	}
	return checkSum;
}//eo-calcCheckSum

/*--- average ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		19/01/2020
Desc:		calculates the average of an array 
Input: 		pointer,address of the array
Returns:	int,numeric value
--------------------------------------------------------------------------------------------*/

int average(int *ptr)
{
	int averageValue=0,i=0;
	long  sum=0;
	for(i=0;i<SAMPLESIZE;i++)
	{	
		sum += *ptr;
		ptr++;
	}
	averageValue = sum/SAMPLESIZE;	

	return averageValue;
}//eo-average

*--- average ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		19/01/2020
Desc:		collects the coverted values from sensors and stores it in the respective sensor
		arrays, post collection of 30 samples or when array is filled and average flag is
		true the average value is stored. 
Input: 		none
Returns:	none
--------------------------------------------------------------------------------------------*/

void sampleCollection(void)
{
		for(FVS.pressure.insertAt= 0;FVS.pressure.insertAt < SAMPLESIZE; FVS.pressure.insertAt++ )
		{
			FVS.pressure.currentSample = getSampleADCPressure();
			FVS.pressure.samples[FVS.pressure.insertAt] = FVS.pressure.currentSample;
			if(FVS.pressure.insertAt == SAMPLESIZE)
			{
				FVS.pressure.insertAt= FALSE;
			}
			
		}//eo for-loop

		for (FVS.temperature.insertAt = 0; FVS.temperature.insertAt < SAMPLESIZE; FVS.temperature.insertAt++)
		{ 
			FVS.temperature.currentSample = getSampleTemp();
			FVS.temperature.samples[FVS.temperature.insertAt] = FVS.temperature.currentSample;
				

			if(FVS.temperature.insertAt == SAMPLESIZE)
			{
				FVS.temperature.insertAt= FALSE;
			}
				
		}  //eo for loop

		
			for (FVS.co2.insertAt = 0; FVS.co2.insertAt < SAMPLESIZE; FVS.co2.insertAt++)
		{ 
			FVS.co2.currentSample = getSampleADCO2();
			FVS.co2.samples[FVS.co2.insertAt] = FVS.co2.currentSample;
			
				
		
				if(FVS.co2.insertAt == SAMPLESIZE)
				{
					FVS.co2.insertAt= FALSE;
				}
		}


		if(avgFlag == TRUE)
		{
			FVS.temperature.averageSample = average(FVS.temperature.samples);
			FVS.pressure.averageSample = average(FVS.pressure.samples);
			FVS.co2.averageSample = average(FVS.co2.samples);
		}

		
	
			
	
}
		// --- display statements---
/*---display ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		20/01/2020
Modified:	None
Desc:		Displays all the data collected, converted in a user-friendly manner.
Input: 		none
Returns:	none

--------------------------------------------------------------------------------------------*/

void display(void)
{
		
	printf("\33[1;0HSystem is Ready\n\r");
	printf("\33[2;0HFVS System Properties\n\r");
	{
		if(FVS.modeState == 0 && FVS.channelState ==0)
		{
			printf("\33[3;0HMODEPB: %4s\t\t ChannelSelected: %12s",low,tempChannel);
		}
		else if(FVS.modeState == 0 && FVS.channelState ==1)
		{
			printf("\33[3;0HMODEPB: %4s\t\t ChannelSelected: %12s",low,pressChannel);
		}
		else if(FVS.modeState == 1 && FVS.channelState ==0)
		{
			printf("\33[3;0HMODEPB: %4s\t\t ChannelSelected: %12s",high,tempChannel);
		}
		else if (FVS.modeState == 1 && FVS.channelState ==1)
		{
			printf("\33[3;0HMODEPB: %4s\t\t ChannelSelected: %12s",high,pressChannel);
		}
		else if(FVS.modeState == 1 && FVS.channelState ==2)
		{
			printf("\33[3;0HMODEPB: %4s\t\t ChannelSelected: %12s",high,coChannel);
		}

		else 
		{
			printf("\33[3;0HMODEPB: %4s\t\t ChannelSelected: %12s",low,coChannel);
		}

	}
	printf("\33[4;0HTemperature \33[4;30H  Pressure	\33[4;60H CO2\n\r");
	
	if(avgFlag == TRUE)
	{
		printf("Current:\t%i%cC    \33[5;30H  Current:\t%ikPa    \33[5;60H Current:\t%ippm   \n\r",(int)FVS.temperature.currentSample,248,(int)FVS.pressure.currentSample,(int)FVS.co2.currentSample);
	}
	else
	{
			printf("Current:\tupdating  \33[5;30H  Current:\tupdating \33[5;60H Current:\tupdating\n\r");
			
	}
	printf("High Limit:\t%i%cC \33[6;30H  High Limit:\t%ikPa \33[6;60H High Limit:\t%ippm\n\r",FVS.temperature.highLimit, 248, FVS.pressure.highLimit,FVS.co2.highLimit);

	printf("Low Limit:\t%i%cC \33[7;30H  Low Limit:\t%ikPa \33[7;60H Low Limit:\t%ippm \n\r", FVS.temperature.lowLimit, 248, FVS.pressure.lowLimit,FVS.co2.lowLimit);

	if(FVS.temperature.state == TRUE)
	{
		printf("Status:\t\t\e[31m\e[40m%5s\e[37m\e[40m\n\r",alarm);
	}
	else if(FVS.temperature.state == FALSE)
	{
		printf("Status:\t\t\e[32m\e[40m%5s\e[37m\e[40m\n\r",safe);
	}

	if(FVS.pressure.state == TRUE)
	{
		printf("\33[8;30H  Status:\t\t\e[31m\e[40m%5s\e[37m\e[40m\n\r",alarm);
	}

	else if(FVS.pressure.state == FALSE)
	{
		printf("\33[8;30H  Status:\t\t\e[32m\e[40m%5s\e[37m\e[40m\n\r", safe);

	}

	if((FVS.co2.averageSample > 	FVS.co2.highLimit) || (FVS.co2.averageSample < FVS.co2.lowLimit))
	{
		printf("\33[8;60H Status:\t\e[31m\e[40m%5s\e[37m\e[40m\n\r",alarm);
	}

	else
	{
		printf("\33[8;60H Status:\t\e[32m\e[40m%5s\e[37m\e[40m\n\r", safe);

	}


	printf("\33[11;0HMotor1: Discharge \33[11;30H Motor2: Dampener\n\r");
	printf("\33[12;0HPosition:%3i\33[12;30H Position:%2i\n\r",FVS.motor1.currentPosition,FVS.motor2.currentPosition);
	
		if(motor1Status)
		{
			printf("\33[13;0HControl: RUN \n\r");
		}
		else
		{
			printf("\33[13;0HControl: STOP\n\r");
		}	  
		
		if(FVS.motor2.currentPosition == 0 || FVS.motor2.currentPosition == 30)
		{
			printf("\33[13;30H Control: STOP\n\r");
		}
		else
		{
			printf("\33[13;30H Control: RUN \n\r");
		}
	printf("\33[14;0HData:0x%x\33[14;30H Data:0x%x\n\r",FVS.motor1.pattern,FVS.motor2.pattern); 

		

}		//eo:display

		// --- Function Header ---
/*--- initializeSystem ---------------------------------------------------------------------------
Author:		Abdul Baasit
Date:		19/01/2020
Modified:	None
Desc:		ntialize all the fuctions required for intial setup
Input: 		none
Returns:	none
--------------------------------------------------------------------------------------------*/

void initializeSystem(void)
{
	set_osc__4MHz();	
	configPort();		
	configSerial();         
	configadc();				
	timerConfig();
	structureInitialization();
	sprintf(FVS.address,ADDRFVS);
	printf("%s",clearScreen);
	
}		//eo:initializeSystem



/*--- MAIN FUNCTION -------------------------------------------------------------------------
-------------------------------------------------------------------------------------------*/

void main()
{

	char oneSec=0;
	char avgCount =0;
	initializeSystem();
	
	
	while(TRUE)
	{
		mode();
		chanelSelect();
		incrementOperation();
		decrementOperation();
		
		
		if(TMRFLAG)
		
		{
			oneSec++;
			avgCount++;
			resetTmr0();
			systemOperation();
			motor1LEDS();
			motor2LEDS();
			if(oneSec == 4)
			{
				
				sampleCollection();
				
				display();
				oneSec=0;
			}		
			
			
			if(avgCount >= AVGTIME)
			{
				avgFlag = TRUE;
			}
		}//eo if loop
		
		
	}//eo-while


}//eo main
