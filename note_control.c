/* 	Sarah_Lichtman@hmc.edu
	Laser Harp ADC functions
	11 November 2012
*/

#include <stdio.h>
#include <math.h>
#include <P32xxxx.h>


//define constants
#define RESET 		PORTFbits.RF0
#define DAC_CLOAD		PORTFbits.RF1
#define TOLERANCE 	100 		//ADC level difference allowance
#define NUMPTS		200		// this... maybe shouldn't be a constant.

// constant lengths of sine arrays
int lens[8] = {NUMPTS, NUMPTS*(261.6/293.7), NUMPTS*(261.6/329.6), NUMPTS*(261.6/349.2), NUMPTS*(261.6/392.0), NUMPTS*(261.6/440.0), NUMPTS*(261.6/493.9), NUMPTS*(261.6/523.2)};
//constant sine tables
short sinC5[NUMPTS], sinD5[NUMPTS], sinE5[NUMPTS], sinF5[NUMPTS], sinG5[NUMPTS], sinA5[NUMPTS], sinB5[NUMPTS], sinC6[NUMPTS];

// global starting points in each sine
int starts[8] = {0};

void init_PIC(void){
	//setup appropriate output pin as output, and serial connection
	// necessary to drive the DAC
	
	// set up "reset" input pin
	TRISF = 0x1; // set up input pin for reset signal
	
	// set up LEDs for testing
	TRISE = 0x0;
	TRISD = 0x0; //PORTD as outputs
	
	T1CONbits.ON = 0;
	T1CON1bits.TCS = 0; // internal clock
	T1CON1bits.TCKPS = 0; // 1:1 prescalar
	TMR1 = 0; // reset timer
	//set up timer for playing sine wave
	PR1 = (short) ((20000000/NUMPTS)/261.6-1); // set period register for freq.
	// T1CONbits.ON = 1; // turn Timer1 on


}

void initadc(int channel) {	
	AD1CHSbits.CH0SA = channel;
	AD1PCFGCLR = 1 << channel;
	AD1CON1bits.ON = 1; 	
	AD1CON1bits.SAMP = 1;
	AD1CON1bits.DONE = 0;
}	
	
int readadc(void) {	
	AD1CON1bits.SAMP = 0;
	while (!AD1CON1bits.DONE);
	AD1CON1bits.SAMP = 1;
	AD1CON1bits.DONE = 0;
	return ADC1BUF0;	
	
}	

void spi_sendNotes(short send){
  //use SPI2 to send the notes to the FPGA
  SPI2BUF = send;
}

void spi_sendWave(short send){
  //use SPI4 to send the 12-bit signal to the DAC
  DAC_CLOAD = 1; 		//disable load while changing
  SPI4BUF = send;
  while(SPI4STATbits.SPIBUSY); // wait until complete
  DAC_CLOAD = 0; 		// send new point to analog output
}


void init_sines(void){
	int i;
	int pts;
	for(i=0; i<lens[0]; i++){
		sinC5[i] = 2047*(sin(2*3.14159*i/NUMPTS)+1);
	}

	for(i=0; i<lens[1]; i++){
		sinD5[i] = 2047*(sin(2*3.14159*i/lens[1])+1);
	}

	for(i=0; i<lens[2]; i++){
		sinE5[i] = 2047*(sin(2*3.14159*i/lens[2])+1);
	}		

	for(i=0; i<lens[3]; i++){
		sinF5[i] = 2047*(sin(2*3.14159*i/lens[3])+1);
	}

	for(i=0; i<lens[4]; i++){
		sinG5[i] = 2047*(sin(2*3.14159*i/lens[4])+1);
	}

	for(i=0; i<lens[5]; i++){
		sinA5[i] = 2047*(sin(2*3.14159*i/lens[5])+1);
	}

	for(i=0; i<lens[6]; i++){
		sinB5[i] = 2047*(sin(2*3.14159*i/lens[6])+1);
	}	

	for(i=0; i<lens[7]; i++){
		sinC6[i] = 2047*(sin(2*3.14159*i/lens[7])+1);
	}	
}



void set_threshholds(int thresh[8]){
	//read the ADC to set default threshhold values
	
	//set threshhold levels to current input voltage levels
	initadc(2);	
	thresh[0] = readadc();
	initadc(3);	
	thresh[1] = readadc();
	initadc(4);	
	thresh[2] = readadc();
	initadc(5);	
	thresh[3] = readadc();
	initadc(6);	
	thresh[4] = readadc();
	initadc(7);	
	thresh[5] = readadc();
	initadc(8);	
	thresh[6] = readadc();
	initadc(9);	
	thresh[7] = readadc();

}

_Bool read_lasers(int thresh[8], _Bool play[8]){
	//sample all 8 lasers
	//will associate "high" input as "add this frequency to output signal"
	//return TRUE if changed from previous, FALSE if the same

	int temp[8]; //sample the 8 analog inputs
	initadc(2);	
	temp[0] = readadc();
	initadc(3);	
	temp[1] = readadc();
	initadc(4);	
	temp[2] = readadc();
	initadc(5);	
	temp[3] = readadc();
	initadc(6);	
	temp[4] = readadc();
	initadc(7);	
	temp[5] = readadc();
	initadc(8);	
	temp[6] = readadc();
	initadc(9);	
	temp[7] = readadc();

	// SEND LASERS ON TO FPGA (VIA SPI)
	short tosend = 0;
	_Bool retval = 0;
	int i;
	_Bool play2[8] = {0}; ///
	short lights = 0x0; ////
	
	for(i=0; i<8; i++){
		if(temp[i]<thresh[i]-TOLERANCE){ //the laser is blocked
			if(play[i]){
				retval = 0; // no change
			}else{
				play[i] = 1; // change current state
				play2[i] = 1;///
				retval = 1;
			}
		}else{
			if(play[i]){
				play[i] = 0;
				play2[i] = 0; ///
				retval = 1; // change current state
			}else{
				retval = 0; // no change
			}
		tosend += play[i] << i;  // build SPI signal	
		}
		lights += play[i] * (1<<i); ////
	}
	spi_sendNotes(tosend);
	return retval;
}

void build_sine(_Bool *play){
	//interpret inputs into frequencies
	// build combined sine signal
	// output sine signal to DAC (max 1.4MHz signal)
	// we're using a 12-bit 1-sided supply DAC
		// sin(double x) returns a double
	short output[NUMPTS] = {0};
	short i = 0; 
	char count = 0;
	for(i=0; i<8; i++){
		count = play[i] ? count + 1 : count;
	}
	count = count ? count : 1;	
	
	for(i=0; i<NUMPTS; i++){
		output[i] = play[0] ? output[i] + sinC5[(starts[0] + i)%lens[0]] : output[i];
		output[i] = play[1] ? output[i] + sinD5[(starts[1] + i)%lens[1]] : output[i];
		output[i] = play[2] ? output[i] + sinE5[(starts[2] + i)%lens[2]] : output[i];
		output[i] = play[3] ? output[i] + sinF5[(starts[3] + i)%lens[3]] : output[i];
		output[i] = play[4] ? output[i] + sinG5[(starts[4] + i)%lens[4]] : output[i];
		output[i] = play[5] ? output[i] + sinA5[(starts[5] + i)%lens[5]] : output[i];
		output[i] = play[6] ? output[i] + sinB5[(starts[6] + i)%lens[6]] : output[i];
		output[i] = play[7] ? output[i] + sinC6[(starts[7] + i)%lens[7]] : output[i];

		while(IFS0bits.T1IF == 0){}; // wait until time to change
		IFS0bits.T1IF = 0; // clear timer overflow flag
		spi_sendWave((short) (output[i]/count));//write output[i] to DAC
		
	}
	for(i=0; i<8; i++){
		starts[i] = (starts[i]+NUMPTS)%lens[i];
	}
}

void init_SPI(void) {
	TRISGbits.TRISG6 = 0; 			// set sync. clock pin as output
	TRISGbits.TRISG7 = 1;			// SDI2 as input
	TRISGbits.TRISG8 = 0;			// SD02 as output
	
	TRISBbits.TRISB14 = 0; 			// set sync. clock pin as output
	TRISFbits.TRISF5 = 0;			// SD04 as output
	DAC_CLOAD = 0;					// CLOAD as output
	
	 short junk;
	 SPI2CONbits.ON = 0; //disable SPI to reset
	 SPI4CONbits.ON = 0;
	 junk = SPI2BUF;     //read buffer to clear it
	 junk = SPI4BUF;
	 SPI2BRG = 7;         //set BAUD rate to 1.25MHz, with Pclk at 20MHz
	 SPI4BRG = 7;     	  
	 SPI2CONbits.MODE16 = 1;//set to 16-bit buffer
	 SPI4CONbits.MODE16 = 1;   // 16-bit buffer
	 SPI2CONbits.MSTEN = 1; //enable as master
	 SPI4CONbits.MSTEN = 1; 
	 SPI2CONbits.CKE = 1;  //center clock-to-data timing
	 SPI4CONbits.CKE = 1;
	 SPI2CONbits.ON = 1; //turn SPI on
	 SPI4CONbits.ON = 1; 
	 
  
}


//main
int main(void){
	init_SPI();
	init_PIC();
	init_sines();

	_Bool play[8] = {0};
	int thresh[8];
	_Bool changed = 0;

	while(1){
		if(RESET){
			set_threshholds(thresh);
		}else{
			do{
				changed = read_lasers(thresh, play);
				build_sine(play);
			}while(!RESET);
			
		}
	
		
	}

}