#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint8_t count, write = 0;
volatile uint8_t add = 0, addr = 0;
volatile uint8_t deploymentStatus;
volatile uint8_t voltageStatus;
volatile uint8_t currentStatus;
volatile uint8_t commData1,commData2;
volatile uint8_t operationalMode;
volatile uint8_t timestamp1,timestamp2,timestamp3,timestamp4;
int times=0;
int repeat=0;
uint8_t temp = 0x00;

void setupTimer(void){
	//enabling output compare interrupt
	TIMSK = (1<<OCIE1A);
	//putting the value to compare counter with
	OCR1A = 0x3D08;
	//selecting clock, enabling CTC mode (Clear Timer on Compare) 
	TCCR1B = (1<<CS12)|(1<<CS10)|(1<<WGM12);
	count=0;
}

void initInterrupt(void){
	//setting to generate interrupt request at rising edge
	EICRA = (1<<ISC20)|(1<<ISC21)|(1<<ISC30)|(1<<ISC31);
	//setting to generate interrupt request when logic changes
	EICRB = (1<<ISC40)|(1<<ISC50)|(1<<ISC60)|(1<<ISC70);
	//enabling external interrupts
	EIMSK = (1<<INT2)|(1<<INT3)|(1<<INT4)|(1<<INT5)|(1<<INT6)|(1<<INT7);
}

void adcInit(void){
	//enabling ADC, setting frequency prescaler to 16
	ADCSRA |= (1<<ADEN)|(1<<ADPS2);
}

void i2cInit(void){
	//setting division factor for SCL to 32
	TWBR=0x20;
	//setting prescaler to 1
	TWSR=0x00;
	add=0x00;
}

void i2cStart(void){
	//clearing interrupt flag, generate start condition, enable twi operation
	TWCR=0b10100100;
	//waiting for interrupt flag to set
	while(!(TWCR & (1<<TWINT)));
	//checking status register
	while((TWSR & 0xF8)!= 0x08);
}

void eepromWr(void){
	//loading SLA+W 
	TWDR=0b10100000;
	//clear TWINT
	TWCR=(1<<TWINT)|(1<<TWEN);
	//wait for flag to set
	while (!(TWCR & (1<<TWINT)));
	//check status register
	while( (TWSR & 0xF8) != 0x18);
}

void address(uint8_t addrs){
	/*sends to eeprom the address to write at
	*Parameters
	*addrs     address at which to write
	*/
	TWDR=addrs;
	//clear TWINT
	TWCR=(1<<TWINT)|(1<<TWEN);
	//wait for flag to set
	while (!(TWCR & (1<<TWINT)));
	//check status register
	while( (TWSR & 0xF8) != 0x28);
}

void eepromWrite(unsigned char data){
	//load data into TWDR
	TWDR=data;
	//clear TWINT
	TWCR=(1<<TWINT)|(1<<TWEN);
	//wait for flag to set
	while (!(TWCR & (1<<TWINT)));
	//check status register
	while( (TWSR & 0xF8) != 0x28);
	//increment add for next write operation
	add+=0x01;
}

unsigned char eepromRandomRead(void){
	/*returns the data stored in EEPROM at the prev sent address
	*/
	//sending repeated start condition
	TWCR=0b10100100;      
	//waiting for flag to set
	while(!(TWCR & (1<<TWINT)));
	//checking status register
	while((TWSR & 0xF8)!= 0x10);
	
	//loading SLA+R
	TWDR=0b10100001; 
	//clear TWINT
	TWCR=(1<<TWINT)|(1<<TWEN);
	//waiting for flag to set
	while (!(TWCR & (1<<TWINT)));
	//check status register
	while( (TWSR & 0xF8) != 0x40);
	
	// TWEA not set as we have to send nack
	TWCR = (1<<TWINT)|(1<<TWEN);  
	//waiting for flag to set
	while (!(TWCR & (1<<TWINT)));
	//check status register
	while(((TWSR & 0xF8)!=0x58));
	
	return(TWDR);
}


void i2cStop(void){
	//sending stop conditon
	TWCR=0b10010100;
}

void adcConvert(uint8_t mux){
	/*Writes the adc value into EEPROM
	*parameters
	*mux     analog channel
	*/
	//setting external voltage reference, left adjust result
	ADMUX = 0x20;
	ADMUX |= mux;
	//start conversion
	ADCSRA |= (1<<ADSC);
	//wait for conversion to finish
	while(!(ADCSRA & (1<<ADIF)));
	//write result into eeprom
	eepromWrite(ADCH);
}

void uartInit (void){
	//setting baud rate to 9600
	UBRR0H=0x00;
	UBRR0L=0x33;
	//setting no. of data bits to 8
	UCSR0C=0x06;
	//enable receive complete interrupt, receiver enable, transmitter enable
	UCSR0B=0x98;
}


void spiInit(void){
	//set MISO as output
	DDRB = (1<<3);
	//enable SPI and SPI interrupt
	SPCR |=(1<<SPIE)|(1<<SPE);
}

uint8_t spiTrans(uint8_t data){
	//load data into SPDR
	SPDR = data;
	// Wait for reception complete 
	while(!(SPSR & (1<<SPIF)));
	//Return data register
	return  SPDR;
}


int main(void){
	
	DDRA=0xFF;
	DDRC=0xFF;
	deploymentStatus=0x00;
	currentStatus=0x00;
	voltageStatus=0x00;
	setupTimer();
	adcInit();
	uartInit();
	spiInit();
	i2cInit();
	initInterrupt();
	sei();
	
	while (1)
	{
		if (count==30)
		{	
			count=0;
			i2cStart();
			eepromWr();
			address(0x00);
			address(add);
			eepromWrite(timestamp1);
			eepromWrite(timestamp2);
			eepromWrite(timestamp3);
			eepromWrite(timestamp4);
			eepromWrite(operationalMode);
			eepromWrite(deploymentStatus);
			eepromWrite(voltageStatus);
			adcConvert(0x00);
			adcConvert(0x01);
			eepromWrite(currentStatus);
			adcConvert(0x02);
			adcConvert(0x03);
			adcConvert(0x04);
			adcConvert(0x05);
			adcCconvert(0x06);
			adcConvert(0x07);
			eepromWrite(comm_data1);
			eepromWrite(comm_data2);
			i2cStop();
					
		}
		
	}
}

ISR(USART0_RX_vect)
{
	if (repeat==0)
	{
		commData1=UDR0;
		repeat++;
	}
	else
	{
		commData2=UDR0;
		repeat=0;
	}
}

ISR(SPI_STC_vect)
{	
	PORTC = 0x01;
	switch(times)
	{
		case 0 :
		operationalMode=SPDR;
		times++;
		//PORTC|=0xC0;
		break;
		case 1 :
		timestamp1=SPDR;
		times++;
		//PORTC|=0x30;
		break;
		case 2 :
		timestamp2=SPDR;
		times++;
		//PORTC|=0x0C;
		break;
		case 3 :
		timestamp3=SPDR;
		times++;
		//PORTC|=0x03;
		break;
		case 4 :
		timestamp4=SPDR;
		times++;
		//PORTA|=0xF0;
		break;
		case 5 :
		i2cStart();
		eepromWr();
		address(0x00);
		address(addr);
		PORTC = 0x00;
		temp = SPDR;
		SPDR = eepromRandomRead();
		i2cStop();
		PORTC = 0xF0;
		if(addr == 0x09) times = 0;
		else times = 5;
		addr++;
		break;
		
	}
}

ISR(TIMER1_COMPA_vect)
{
	count++;
}

ISR(INT2_vect)
{
	deploymentStatus |= 0x30;
}

ISR(INT3_vect)
{
	deploymentStatus |= 0x03;
}

ISR(INT4_vect)
{
	voltageStatus |= 0x30;
}

ISR(INT5_vect)
{
	voltageStatus |= 0x03;
}

ISR(INT6_vect)
{
	currentStatus |= 0x30;
}

ISR(INT7_vect)
{
	currentStatus |= 0x03;
}

