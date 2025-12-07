// CPE 301 Final Project
// Contributers:
// Isaiah McLain
// Kaiden Farthing 
// Date: 12/12/2025 

#include <LiquidCrystal.h>

#define RDA 0x80
#define TBE 0x20   

#define START_PIN 21

// LCD pins <--> Arduino pins
const int RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5;
int right=0,up=0;
int dir1=0,dir2=0;
int score = 0;
int toggle = 0;
byte customChar[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
};

byte customChar2[8] = {
  0b00000,
  0b00000,
  0b01110,
  0b01110,
  0b01110,
  0b00000,
  0b00000,
  0b00000
};

byte customChar3[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

// UART Pointers
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

volatile unsigned char *portDDRB = (unsigned char *) 0x24;
volatile unsigned char *portB =    (unsigned char *) 0x25;

// LED Pointers
unsigned char* ddr_a = (unsigned char*) 0x21;
unsigned char* port_a = (unsigned char*) 0x22;
volatile unsigned char* pin_a = (unsigned char*) 0x20;

// Button Pointers ( Except Start )
unsigned char* ddr_c = (unsigned char*) 0x27;
unsigned char* port_c = (unsigned char*) 0x28;
volatile unsigned char* pin_c = (unsigned char*) 0x26;

// Start button needs different pin for interupt
volatile unsigned char *port_d = (unsigned char *) 0x2B;
volatile unsigned char *ddr_d = (unsigned char *) 0x2A;
volatile unsigned char *pin_d = (unsigned char *) 0x29;

// Timer Pointers
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *myTCNT1  = (unsigned  int *) 0x84;
volatile unsigned char *myTIFR1 =  (unsigned char *) 0x36;

// ADC Pointers 
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int*  my_ADC_DATA = (unsigned int*) 0x78;


byte in_char;
//This array holds the tick values
//calculate all the tick value for the given frequencies and put that in the ticks array
unsigned int ticks[14]= {18181, 16214, 15277, 13632, 12140, 11461, 10204, 18181, 16214, 15277, 13632, 12140, 11461, 10204};
//This array holds the characters to be entered, index echos the index of the ticks
//that means ticks[0] should have the tick value for the character in input[0]
unsigned char input[14]= {'a','b','c','d','e','f','g', 'A', 'B', 'C', 'D', 'E', 'F', 'G'};

//global ticks counter
unsigned int currentTicks = 65535;
unsigned char timer_running = 0;

// We have four states: 
// 0: DISABLED - Yellow LED
// 1: IDLE - Green LED
// 2: RUNNING - Blue LED
// 3: ERROR - RED LED
unsigned int program_state = 0; 

void setup() 
{           
  // Setup LEDS
  // Set PA6 (Blue), PA4 (Green), PA2 (Yellow), PA0 (Red) as outputs
  *ddr_a |= (1 << 6) | (1 << 4) | (1 << 2) | (1 << 0);

  // Set PC5, PC3, PC1 as inputs
  *ddr_c &= ~((1 << 5) | (1 << 3) | (1 << 1)); // inputs
  *port_c |=  (1 << 5) | (1 << 3) | (1 << 1);  // enable pull-ups

  // Set PD0 as input / setup for ISR
  *port_d |= 0b00000001; // set PD0 to have pull up resistor
  *ddr_d &= 0b11111110;  // set PD0 as input for button

  // Attach interrupt to start button 
  attachInterrupt(digitalPinToInterrupt(START_PIN), StartProgram, RISING);
  
  // // setup the Timer for Normal Mode, with the TOV interrupt enabled
  // setup_timer_regs();
  
  // Start the UART
  U0Init(9600);
}

void loop() 
{
  // Trigger Idle State
  if (program_state == 1) {
    *port_a &= ~(0x01 << 6); 
    *port_a |= (0x01 << 4); // Green On
    *port_a &= ~(0x01 << 2);  
    *port_a &= ~(0x01);
  }

  // Trigger Disabled State
  if (program_state == 0) {
    *port_a &= ~(0x01 << 6); 
    *port_a &= ~(0x01 << 4); 
    *port_a |= (0x01 << 2); // Yellow On
    *port_a &= ~(0x01);
  }

  // Error State
  // if (!(*pin_c & (1 << 1))) {
  //   program_state = 3;
  //   *port_a |= (0x01 << 6); // Blue On
  //   *port_a &= ~(0x01 << 4);
  //   *port_a &= ~(0x01 << 2);
  //   *port_a &= ~(0x01);
  // }

  // *port_a &= ~(0x01 << 6); 
  // *port_a |= (0x01 << 4); // Green on
  // *port_a &= ~(0x01 << 2);
  // *port_a &= ~(0x01);


  // *port_a &= ~(0x01 << 6); 
  // *port_a &= ~(0x01 << 4); 
  // *port_a |= (0x01 << 2); // Yellow on
  // *port_a &= ~(0x01);

  // *port_a &= ~(0x01 << 6); 
  // *port_a &= ~(0x01 << 4);
  // *port_a &= ~(0x01 << 2);
  // *port_a |= (0x01); // Red on
  
  // // if we recieve a character from serial
  // if (kbhit()) 
  // {
  //   // read the character
  //   in_char = getChar();
  //   // echo it back
  //   putChar(in_char);
  //   // if it's the quit character
  //   if(in_char == 'q' || in_char == 'Q')
  //   {
  //     putChar('\n');
  //     putChar('o');
  //     // set the current ticks to the max value
  //     currentTicks = 65535;
  //     // if the timer is running
  //     if(timer_running)
  //     {
  //       // stop the timer
  //       *myTCCR1B &= 0xF8;
  //       // set the flag to not running
  //       timer_running = 0;
  //       // set PB6 LOW
  //       *portB &= 0xBF;
  //     }
  //   }
  //   // otherwise we need to look for the char in the array
  //   else
  //   {
  //     // look up the character
  //     for(int i=0; i < 13; i++)
  //     {
  //       // if it's the character we received...
  //       if(in_char == input[i])
  //       {
  //         // set the ticks
  //         currentTicks = ticks[i];
  //         // if the timer is not already running, start it
  //         if(!timer_running)
  //         {
  //             // start the timer
  //             *myTCCR1B |= 0x05;
  //             // set the running flag
  //             timer_running = 1;
  //         }
  //       }
  //     }
  //   }
  // }
}

void adc_init() 
{
 // setup the A register
 // set bit 7 to 1 to enable the ADC 
 *my_ADCSRA |= 0b10000000;

 // clear bit 5 to 0 to disable the ADC trigger mode
 *my_ADCSRA &= 0b11011111;

 // clear bit 3 to 0 to disable the ADC interrupt 
 *my_ADCSRA &= 0b11110111;

 // clear bit 0-2 to 0 to set prescaler selection to slow reading
 *my_ADCSRA &= 0b11111000;

 // setup the B register
 // clear bit 3 to 0 to reset the channel and gain bits
 *my_ADCSRB &= 0b11110111;
 // clear bit 2-0 to 0 to set free running mode
 *my_ADCSRB &= 0b11111000;
 
 // setup the MUX Register
 // clear bit 7 to 0 for AVCC analog reference
 *my_ADMUX &= 0b01111111;

 // set bit 6 to 1 for AVCC analog reference
 *my_ADMUX |= 0b01000000;

 // clear bit 5 to 0 for right adjust result
 *my_ADMUX &= 0b11011111;
 // clear bit 4-0 to 0 to reset the channel and gain bits
 *my_ADMUX &= 0b11100000;
}

unsigned int adc_read(unsigned char adc_channel_num) //work with channel 0
{
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX &= 0b11100000;

  // clear the channel selection bits (MUX 5) hint: it's not in the ADMUX register
  *my_ADCSRB &= 0b11111110;

  if (adc_channel_num >= '7') {
    *my_ADCSRB |= 0b00010000;
  }
 
  // set the channel selection bits for channel 0
  *my_ADMUX += adc_channel_num;
  

  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0b01000000;

  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register and format the data based on right justification (check the lecture slide)
  
  unsigned int val = (*my_ADC_DATA & 0x03FF);
  return val;
}

// Timer setup function
void setup_timer_regs()
{
  // setup the timer control registers
  *myTCCR1A= 0x00;
  *myTCCR1B= 0X00;
  *myTCCR1C= 0x00;
  
  // reset the TOV flag
  *myTIFR1 |= 0x01;
  
  // enable the TOV interrupt
  *myTIMSK1 |= 0x01;
}


// Start Button ISR
void StartProgram()
{
  putChar('d');
  if (program_state == 0) {
    program_state = 1;
  } else {
    program_state = 0;
  }
}

/*
 * UART FUNCTIONS
 */
void U0Init(int U0baud)
{
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}
unsigned char kbhit()
{
  return *myUCSR0A & RDA;
}
unsigned char getChar()
{
  return *myUDR0;
}
void putChar(unsigned char U0pdata)
{
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}