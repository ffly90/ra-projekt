/*
 First PIC32MM program
 
 This simple example program lets LED1 blink
 */

#include <stdint.h>
#include <xc.h>

void SYSTEM_Initialize(void);

void setup() { 
	SYSTEM_Initialize();   //set 24 MHz clock for CPU and Peripheral Bus
                           //clock period = 41,667 ns = 0,0417 us
}

void loop() {
	while(1) {
        while(U1STAbits.UTXBF == 1){}
       
        U1TXREG = 0x1F;    // Write the data byte to the USART.
    }
}

int main(void) {
    setup();
    loop();
}