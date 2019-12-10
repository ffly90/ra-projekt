/*
 First PIC32MM program
 
 This simple example program lets LED1 blink
 */

#include <stdint.h>
#include <stdbool.h>
#include <sys/attribs.h>
#include <xc.h>
#include "color.h"

typedef char u8;
#define LEDS 24 // Number of LEDs

unsigned char dark[]={0,0,0,0};          // RGBW values for LED off
unsigned char rainbow_color[LEDS][4];    // RGBW values for rainbow
int uart_rainbow[LEDS][11];     // UART values for each LED
int uart_off[11];               // UART values for LED off
char dir = 1; // init direction
char pos=0; // current active LED

void char_to_bool(char in, bool out[]){

}

void setup() { 
	SYSTEM_Initialize();   //set 24 MHz clock for CPU and Peripheral Bus
                           //clock period = 41,667 ns = 0,0417 us
    
    //Zähler Konfiguration
    T1CONbits.TGATE     = 0;
    T1CONbits.TCS       = 0;
    T1CONbits.TCKPS     = 0b11;
    T1CONbits.TSYNC     = 0;
    PR1                 = 3225; // overflow interrupt and auto reset at 3225 -> 10ms
    //Timer Interrupt
    IEC0bits.T1IE       = 1;
    IPC4bits.T1IP       = 3;
    
    
    // UART CONFIGURATION
    
    // Set U1MODE Register
    U1MODEbits.BRGH = 1; // 4x baud clock enabled
    U1BRG = 1; // Set Counter to 1
    U1MODEbits.CLKSEL = 0b01; // The UART1 clock is the SYSCL
    U1MODEbits.PDSEL = 0b11; // 9 -bit data, no parity
    U1MODEbits.SLPEN = 1; // UART1 clock runs during Sleep
    U1MODEbits.STSEL = 1; // 2 Stop bit
    U1MODEbits.ON = 1; // UARTx is enabled; UARTx pins are controlled by UARTx, as defined by the UEN[1:0] and UTXEN control bits
    // Set U1STA Register
    U1STAbits.UTXEN = 1; //UARTx transmitter is enabled, UxTX pin is controlled by UARTx (if ON = 1)
    U1STAbits.UTXINV = 1; // UxTX Idle state is ?1?
    
    // OUTPUT PIN CONFIGURATION
    TRISC = 0xEFFF;
    
    // pin B9 Interrupt
    // INT2I: External 2
    TRISBbits.TRISB9    = 1;    // set input RB9 or button S1
    IPC1bits.INT2IP     = 1;    // Priority: 1
    IPC1bits.INT2IS     = 0;    // Sub Priority: 0
    IFS0bits.INT2IF     = 0;    // external interrupt handled by extra IRS
    CNEN0Bbits.CNIE0B9  = 0;    // deactivate rising flag interrupt
    CNEN1Bbits.CNIE1B9  = 1;    // activate falling flag interrupt
    
    INTCONbits.INT2EP   = 0;    // clear interrupt at falling edge bit
    IEC0bits.INT2IE     = 1;    // enable external interrupt 2
    
}

void __ISR(_EXTERNAL_2_VECTOR, IPL2SOFT) buttonInterrupt(void){
    dir = -1* dir;
    IFS0bits.INT2IF = 0;
}


void create_rainbow(){
    int i;
    for(i=0;i<LEDS;i++){
        HsvColor hsv = {.h = 255*i/(LEDS-1), .s = 255, .v = 32};
        RgbColor rgb = HsvToRgb(hsv);
        rainbow_color[i][0] = rgb.g;
        rainbow_color[i][1] = rgb.r;
        rainbow_color[i][2] = rgb.b;
        rainbow_color[i][3] = rgb.w;
    }
}



void rgbw_to_uart(unsigned char in[], int out[]){// in =  u8 g, u8 r, u8 b, u8 w
    int i,j;
    bool temp[8];
    bool bits[32] ;//= {r,g,b,w}; bits in order nessesarc for LED
    
    for(i=0;i<4;i++){
        for (j = 0; j<8; j++){
            temp[7-j] = in[i] & 1<<j;
        }
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

void display(int pos, int active){
    int i;
    if(active==pos){ // if current LED to UART is current active position
        for (i = 0; i<11; i++){ // loop to send all UART values for a active LED
            while(U1STAbits.UTXBF == 1){}
            U1TXREG = uart_rainbow[active][i];    // Write the data byte to the UART.
        }   
    }else{
        for (i = 0; i<11; i++){ // loop to send all UART values for a dark LED
            while(U1STAbits.UTXBF == 1){}
            U1TXREG = uart_off[i];    // Write the data byte to the UART.
        }
    }
}

void __ISR(_TIMER_1_VECTOR, IPL3SOFT) nextOutput(void) {
    // send LED color vlaues over uart
    int j;
    for (j=0;j<LEDS;j++){
        display(pos, j);
    }

    // step direction for running lights
    pos += dir;
    if(pos>=LEDS){
        pos = 0;
    }else if(pos<0){
        pos = LEDS;
    }
    IFS0bits.T1IF = 0; // reset interrupt flag
}

void loop() {
    int i;
    // create uart muster
    create_rainbow();
    for(i=0; i<LEDS; i++){
        rgbw_to_uart(rainbow_color[i], uart_rainbow[i]);
    }
    rgbw_to_uart(dark, uart_off);
    IFS0bits.T1IF = 0;
    T1CONbits.ON = 1;
    // main loop
    while(1) {
    }
}

int main(void) {
    setup();
    loop();
}