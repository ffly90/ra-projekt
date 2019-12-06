/*
 First PIC32MM program
 
 This simple example program lets LED1 blink
 */

#include <stdint.h>
#include <stdbool.h>
#include <xc.h>

typedef char u8;
#define LEDS 24 // Number of LEDs
char pixels[LEDS][4]; // Color Values (r,g,b,w) for each LED


unsigned int uart_out[256];

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

void rgbw_to_uart(u8 in[], int out[]){//u8 r, u8 g, u8 b, u8 w
    bool bits[32] ;//= {r,g,b,w};
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
    int out[11];
    u8 test[] = {0,0,0,3};
    rgbw_to_uart(test, out);
	while(1) {
        int i;
        int j;
        // r=0, g=0, b=0, w=3
//        int out[]={~0b010001000,
//                   ~0b010001000,
//                   ~0b010001000,
//                   ~0b010001000,
//                   ~0b010001000,
//                   ~0b010001000,
//                   ~0b010001000,
//                   ~0b010001000,
//                   ~0b010001000,
//                   ~0b010001000,
//                   ~0b000011001,
//                   };
        for (j=0;j<24;j++){
            for (i = 0; i<11; i++){
                while(U1STAbits.UTXBF == 1){}
                U1TXREG = out[i];    // Write the data byte to the USART.
            }
        }
        delay_us(80);
    }
}

int main(void) {
    setup();
    loop();
}