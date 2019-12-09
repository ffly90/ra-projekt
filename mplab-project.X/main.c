/*
 First PIC32MM program
 
 This simple example program lets LED1 blink
 */

#include <stdint.h>
#include <stdbool.h>
#include <sys/attribs.h>
#include <xc.h>

#include "color.h"

typedef unsigned char u8;
#define LEDS 24 // Number of LEDs


bool output=false;               // output?
int pos, dir = 1;               // position and direction of running lights
int uart_pos = 0;
unsigned int analog_in;             // Analog In value
unsigned char dark[]={0,0,0,0};     // RGBW values for LED off
unsigned char rainbow_color[LEDS][4];// RGBW values for rainbow
unsigned int uart_rainbow[LEDS][11];// UART values for each LED
unsigned int uart_off[11];          // UART values for LED off


void char_to_bool(char in, bool* out){
    int i = 0;
    for ( i = 0; i<8;i++){
        out[7-i] = in && 1<<i;
    }
}

void setup() { 
	SYSTEM_Initialize();   //set 24 MHz clock for CPU and Peripheral Bus
                           //clock period = 41,667 ns = 0,0417 us
    
    //Zähler Konfiguration
    T1CONbits.TGATE     = 0;
    T1CONbits.TCS       = 0;
    T1CONbits.TCKPS     = 0;
    T1CONbits.TSYNC     = 0;
    PR1                 = 3225; // overflow interrupt and auto reset at 3225 -> 10ms
    T1CONbits.ON        = 1;
    
    //ADC Konfiguration
    ANSELCbits.ANSC8    = 1;
    TRISCbits.TRISC8    = 1;
    AD1CON1bits.SSRC    = 0;
    AD1CON1bits.MODE12  = 0;
    AD1CHSbits.CH0SA    = 14;
    AD1CON3bits.ADRC    = 0;
    AD1CON3bits.ADCS    = 0;
    AD1CON1bits.ON      = 1;
    //Interrupt
    IEC0bits.T1IE       = 1;
    IFS0bits.T1IF       = 0;
    IPC4bits.T1IP       = 2;
    //AD1CON1bits.SAMP = 1;
    
    // UART CONFIGURATION
    
    // Set U1MODE Register
    U1MODEbits.BRGH     = 1; // 4x baud clock enabled
    U1BRG               = 1; // Set Counter to 1
    U1MODEbits.CLKSEL   = 0b01; // The UART1 clock is the SYSCL
    U1MODEbits.PDSEL    = 0b11; // 9 -bit data, no parity
    U1MODEbits.SLPEN    = 1; // UART1 clock runs during Sleep
    U1MODEbits.STSEL    = 1; // 2 Stop bit
    U1MODEbits.ON       = 1; // UARTx is enabled; UARTx pins are controlled by UARTx, as defined by the UEN[1:0] and UTXEN control bits
    // Set U1STA Register
    U1STAbits.UTXEN     = 1; //UARTx transmitter is enabled, UxTX pin is controlled by UARTx (if ON = 1)
    U1STAbits.UTXINV    = 1; // UxTX Idle state is ?1?
    
    // OUTPUT PIN CONFIGURATION
    TRISC = 0xEFFF;
    // INPUT PIN CONFIGURATION
    TRISBbits.TRISB9 = 1;   // set input RB9 or button s1
}

void readADC() {
    AD1CON1bits.SAMP = 1;
    delay_us(100);
    while (!AD1CON1bits.DONE);
    analog_in = ADC1BUF0;
    AD1CON1bits.SAMP = 0;
}

void __ISR(_TIMER_1_VECTOR, IPL2SOFT) nextOutput(void) {
    // step direction for running lights
    pos += dir;
    if(pos>=LEDS){
        pos = 0;
    }else if(pos<=0){
        pos = LEDS;
    } 
    output = true; // if active uart values will be written to fifo buffer
    IFS0bits.T1IF = 0;
}


void __ISR(_UART1_TX_VECTOR, IPL2SOFT) bufferWrite(void) {
    // interrupt 
    // send LED color values over uart
    if(output)    {
        while(U1STAbits.UTXBF == 0){ // while free space in transmit buffer        
            if(pos  == ( uart_pos / 11 )  ){// check if current uart_pos is at the activ LED
                U1TXREG = uart_rainbow[pos][uart_pos % 11 ]; // Write the data byte to the UART.
            }else{
                U1TXREG = uart_off[uart_pos % 11 ]; // Write the data byte to the UART.
            }
            
            // increment uart_pos to get next uart_message by next loop
            uart_pos++;
            if(uart_pos >= 11*LEDS){ // end uart transmision
                uart_pos = 0;
                output = false;
                break;
            }
        }
        
    }    
    IFS1bits.U1TXIF = 0;    // reset interrupt flag for free space in transmit 
                            // buffer
}



void create_rainbow(){
    int i;
    for(i=0;i<LEDS;i++){
        HsvColor hsv = {.h = i/LEDS};
        RgbColor rgb = HsvToRgb(hsv);
        rainbow_color[i][0] = rgb.g;
        rainbow_color[i][1] = rgb.r;
        rainbow_color[i][2] = rgb.b;
        rainbow_color[i][3] = 0;
    }
}


void rgbw_to_uart(u8 in[], int out[]){// in =  u8 g, u8 r, u8 b, u8 w
    bool bits[33] ={0} ;//= {r,g,b,w}; bits in order nessesarc for LED
    int i,j;
    bool temp[8]; 
    
    for(i=0;i<4;i++){
        char_to_bool(in[i],temp);
        for(j=0;j<8;j++){
            bits[i*8+j] = temp[j];
        }
    }
    for(i=0;i<11;i+=1){
        unsigned int uart_val= 0b010001000;
        if(bits[i*3]){
            uart_val |= 0b1;
        }
        if(bits[i*3+1]){
            uart_val |= 0b10000;
        }
        if(bits[i*3+2]){
            uart_val |= 0b100000000;
        }
        if(i==10){
            uart_val &= 0b001111111;
        }
        out[i]=~uart_val;
    }
}

void loop() {
    int i, j;
    bool in_old, in_new;
    // create uart muster
    for(i=0; i<LEDS; i++){
        rgbw_to_uart(rainbow_color[i], uart_rainbow[i]);
    }
    rgbw_to_uart(dark ,uart_off);
    // main loop
    while(1) {
        // call button
        in_old = in_new;
        in_new = (!(PORTB & 1<<9));
        // change dir if positiv flag on button
        if(!in_old && in_new){
            dir = -1* dir;
        }
        
        // Read Value from Potentiometer
        readADC();
    }
}

int main(void) {
    setup();
    loop();
}