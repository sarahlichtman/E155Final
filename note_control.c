/* 	Sarah_Lichtman@hmc.edu
	Laser Harp ADC functions
	11 November 2012
*/

#include <stdio.h>
#include <math.h>
#include <P32xxxx.h>


//define constants
#define RESET 		PORTFbits.RF0
#define DAC_CLOAD	PORTFbits.RF1
#define TOLERANCE 	100 		//ADC level difference allowance
#define NUMPTS		200		// this... maybe shouldn't be a constant.
#define PI 			3.14159

#define TOTAL_WAVES	8		// How many notes we're dealing with	
#define	C3 			261.6 	// Hz
#define	D3 			293.7 	// Hz
#define	E3 			329.6 	// Hz
#define	F3 			349.2 	// Hz
#define	G3 			392.0 	// Hz
#define	A3 			440.0 	// Hz
#define	B3 			493.9 	// Hz
#define	C4 			523.2 	// Hz


// constant lengths of sine arrays
unsigned short lens[TOTAL_WAVES] = {NUMPTS, NUMPTS*(C3/D3), NUMPTS*(C3/E3), NUMPTS*(C3/F3), NUMPTS*(C3/G3), NUMPTS*(C3/A3), NUMPTS*(C3/B3), NUMPTS*(C3/C4)};
//constant sine tables
unsigned short sin_waves[TOTAL_WAVES][NUMPTS] = {{0}};

// global starting points in each sine
unsigned short starts[TOTAL_WAVES] = {0};

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
	PR1 = (unsigned short) ((20000000/NUMPTS)/C3-1); // set period register for freq.
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
	unsigned char i;
	unsigned short j;
	for (i=0; i<TOTAL_WAVES; i++) {
		for (j=0; j<lens[i]; j++) {
			sin_waves[i][j] = 2047*(sin(2*PI*j/lens[i])+1);
		}
	}
}



void set_threshholds(int thresh[TOTAL_WAVES]){
	//read the ADC to set default threshhold values
	
	//set threshhold levels to current input voltage levels
	unsigned char i = 0;
	for (i=0; i<TOTAL_WAVES; i++) {
		initadc(i+2);
		thresh[i] = readadc();
	}

}

_Bool read_lasers(int thresh[TOTAL_WAVES], _Bool play[TOTAL_WAVES]){
	//sample all 8 lasers
	//will associate "high" input as "add this frequency to output signal"
	//return TRUE if changed from previous, FALSE if the same

	unsigned char i = 0;
	int temp[TOTAL_WAVES]; //sample the 8 analog inputs
	for (i=0;i<TOTAL_WAVES; i++) {
		initadc(i+2);
		temp[i] = readadc();
	}

	// SEND LASERS ON TO FPGA (VIA SPI)
	short tosend = 0;
	_Bool retval = 0;
	_Bool play2[TOTAL_WAVES] = {0}; ///
	short lights = 0x0; ////
	
	for(i=0; i<TOTAL_WAVES; i++){
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
	unsigned short output[NUMPTS] = {0};
	unsigned short i = 0;
	unsigned char j = 0; 
	unsigned char count = 0;
	for(i=0; i<TOTAL_WAVES; i++){
		count = play[i] ? count + 1 : count;
	}
	count = count ? count : 1;	
	
	for(i=0; i<NUMPTS; i++){
		for (j=0; j<TOTAL_WAVES; j++) {
			output[i] = play[j] ? output[i] + sin_waves[j][(starts[j] + i)%lens[j]] : output[i];
		}

		while(IFS0bits.T1IF == 0){}; // wait until time to change
		IFS0bits.T1IF = 0; // clear timer overflow flag
		spi_sendWave((unsigned short) (output[i]/count));//write output[i] to DAC
		
	}
	for(i=0; i<TOTAL_WAVES; i++){
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

	_Bool play[TOTAL_WAVES] = {0};
	int thresh[TOTAL_WAVES];
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