#include <msp430.h>

void DAC_cipher(int amplitude, int latch_port);

#define SAMPLE_MAX 500

int sample_count;
int offset = 1150;  //934
int amplitude = 1150;
int m;

int DAC_flag;

int main(void)
{
	int period = 20; // in ms

	WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
	BCSCTL1 = CALBC1_16MHZ;
	DCOCTL = CALDCO_16MHZ;
	_BIS_SR(GIE);

	P1DIR |= BIT0;                            // P1.0 output
	P2DIR |= BIT0;	// Will use BIT4 to activate /CE on the DAC
	P1SEL	= BIT7 + BIT5;	// + BIT4;	// These two lines dedicate P1.7 and P1.5
	P1SEL2 = BIT7 + BIT5; // + BIT4;	// for UCB0SIMO and UCB0CLK respectively

	UCB0CTL0 |= UCCKPL + UCMSB + UCMST + /* UCMODE_2 */ + UCSYNC;
	UCB0CTL1 |= UCSSEL_2;	// UCB0 will use SMCLK as the basis for

	UCB0BR0 |= 0x10;	// (low divider byte)
	UCB0BR1 |= 0x00;	// (high divider byte)
	UCB0CTL1 &= ~UCSWRST;	// **Initialize USCI state machine**

	CCTL0 = CCIE;                             // CCR0 interrupt enabled
	CCR0 = ((period*2000)/SAMPLE_MAX)-2;				  //convert period to us and divide by 1500 to get samples;  500-8 for 40ms period
	TACTL = TASSEL_2 + MC_1 + TAIE + ID_3;                  // SMCLK, upmode

	sample_count = 0;
	DAC_flag = 0;
	while(1)
	{
		//When DAC_flag is raised, change the DAC value
		if(DAC_flag)
		{
			//For less than half the period, the amplitude will be high
			if(sample_count < SAMPLE_MAX/2)
			{
				DAC_cipher(amplitude+offset,BIT0);
			}
			//For the other half of the period, the amplitude will be low
			else if(sample_count >= SAMPLE_MAX/2)
			{
				DAC_cipher(offset-amplitude,BIT0);
			}

			sample_count++;

			if(sample_count == SAMPLE_MAX)
			{
				sample_count = 0;
			}
			DAC_flag = 0;
		}
	}
}

// Timer A0 interrupt service routine
// DAC_flag is raised to change the DAC
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
	//int timerVal = TAR;
	TACTL &= ~TAIFG;
	DAC_flag = 1;
}

void DAC_cipher(int amplitude, int latch_port)
{
	int DAC_code;

	DAC_code = (0x3000)|(amplitude & 0xFFF); //Gain to 1 and DAC on

	P2OUT &= ~latch_port; //Lower CS pin low
	//send first 8 bits of code
	UCB0TXBUF = (DAC_code >> 8);

	//wait for code to be sent
	while(!(UCB0TXIFG & IFG2));

	//send last 8 bits of code
	UCB0TXBUF = (char)(DAC_code & 0xFF);

	//wait for code to be sent
	while(!(UCB0TXIFG & IFG2));

	//wait until latch
	_delay_cycles(100); //170
	//raise CS pin
	P2OUT |= latch_port;

	return;
}
