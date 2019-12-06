/*
 First PIC32MM program
 
 This simple example program lets LED1 blink
 */

#include <stdint.h>
#include <stdbool.h>
#include <xc.h>

typedef char u8;
#define LEDS 24 // Number of LEDs

char rainbow_color[LEDS][4]={
    {0,0,0,255},
    {0,0,255,0},
    {0,255,0,0},
    {255,0,0,0},
    {0,0,0,255},
    {0,0,255,0},
    {0,255,0,0},
    {255,0,0,0},
    {0,0,0,255},
    {0,0,255,0},
    {0,255,0,0},
    {255,0,0,0},
    {0,0,0,255},
    {0,0,255,0},
    {0,255,0,0},
    {255,0,0,0},
    {0,0,0,255},
    {0,0,255,0},
    {0,255,0,0},
    {255,0,0,0},
    {0,0,0,255},
    {0,0,255,0},
    {0,255,0,0},
    {255,0,0,0}
};
int uart_rainbow[24][11];
int uart_off[11];


void char_to_bool(char in, bool* out){
    int i = 0;
    for ( i = 0; i<8;i++){
        out[7-i] = in && 1<<i;
    }
}
unsigned int to_uart_message(bool bit1, bool bit2, bool bit3){
    unsigned int out = 0;
    if(bit1){}
    return out;
}


void pix_to_uart(void){
    
    
}

void setup() { 
	SYSTEM_Initialize();   //set 24 MHz clock for CPU and Peripheral Bus
                           //clock period = 41,667 ns = 0,0417 us
    
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
    
    // PIN CONFIGURATION
    TRISC = 0xEFFF;
    
    
    
}

void create_rainbow(){
    int i;
    char g,r,b;
    for(i=0;i<LEDS;i++){
        if(i<LEDS/3){
            g = 255*i/LEDS;
            r = 255 - 255*i/LEDS;
            b = 0;
        }
        else if(i < LEDS*2/3){        
            g = 255 - 255*i/LEDS;
            r = 0;
            b = 255 * i / LEDS;            
        }
        else{
            g = 0;
            r = 255 * i / LEDS;
            b = 255 - 255 * i / LEDS;
        }
    }
}


void rgbw_to_uart(u8 in[], int out[]){// in =  u8 g, u8 r, u8 b, u8 w
    bool bits[32] ;//= {r,g,b,w}; bits in order nessesarc for LED
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
    int i, pos=0;
    char dark[]={0,0,0,0};
    for(i=0; i<24; i++){
        rgbw_to_uart(rainbow_color[i], uart_rainbow[i]);
    }
    rgbw_to_uart(dark ,uart_off);
	while(1) {
        int i;
        int j;
        for (j=0;j<24;j++){
            if(j==pos){
                for (i = 0; i<11; i++){
                    while(U1STAbits.UTXBF == 1){}
                    U1TXREG = uart_rainbow[j][i];    // Write the data byte to the UART.
                }   
            }else{
                for (i = 0; i<11; i++){
                    while(U1STAbits.UTXBF == 1){}
                    U1TXREG = uart_off[i];    // Write the data byte to the UART.
                }
            }
        }
        delay_us(10000);
        pos++;
        if(pos==24){
            pos = 0;
        }
    }
}

int main(void) {
    setup();
    loop();
}