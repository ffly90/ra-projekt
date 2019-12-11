/*
* LED running light with uart
* by Steffen Walter and Johannes Staib
*
*
*
* -- naming convention --
* Update process    ->  write new rgbw values to all LEDs   =  LEDs * LED messages
* LED message       ->  rgbw values as 32-Bits for one LED  =  11 * uart messages
* uart message      ->  bits for one uart transmit          =  3 LED-Bits
*/

#include <stdint.h>
#include <stdbool.h>
#include <sys/attribs.h>
#include <xc.h>
#include "color.h"

#define LEDS 24 // Number of LEDs


// precalculat arrays
uint8_t         dark[]={0,0,0,0};           // GRBW values for LED off
uint8_t         rainbow_color[LEDS][4];     // GRBW values for rainbow
uint16_t        uart_rainbow[LEDS][11];     // UART values for each LED
uint16_t        uart_off[11];               // UART values for LED off
uint16_t        *uart_map[LEDS*11]  __attribute__((aligned(16)));

// running light values
int8_t          dir = 1;                    // init direction
int8_t          pos=0;                      // current active LED
uint16_t        *uart_msg;                  // #UART message
#define         FIRST_UART_MSG  (uint16_t*)uart_map
#define         LAST_UART_MSG   (uint16_t*)(FIRST_UART_MSG + LEDS*11 - 1)
uint8_t         adc_offset = 0;             // adc sample offset


void setup() { 
	SYSTEM_Initialize();   //set 24 MHz clock for CPU and Peripheral Bus
                           //clock period = 41,667 ns = 0,0417 us
    
    //Zaehler Konfiguration
    T1CONbits.TGATE     = 0;
    T1CONbits.TCS       = 0;
    T1CONbits.TCKPS     = 0b11;
    T1CONbits.TSYNC     = 0;
    PR1                 = 938; // overflow interrupt and auto reset at 3225 -> 10ms
    //Timer Interrupt
    IEC0bits.T1IE       = 1;
    IPC4bits.T1IP       = 4;
    
    
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
    //UART Interrupt
    U1STAbits.UTXSEL = 0; // Interrupt if at least one empty space in buffer
    IPC13bits.U1TXIP = 5; // set interrupt prio to 5
    IFS1bits.U1TXIF = 0; // clear interrupt flag
    
    // OUTPUT PIN CONFIGURATION
    TRISCbits.TRISC12   = 0;    // set RC12 as output for UART1 TX
    
    // pin B9 Interrupt
    // INT2I: External 2
    TRISBbits.TRISB9    = 1;    // set input RB9 or button S1
    IPC1bits.INT2IP     = 2;    // Priority: 2
    IFS0bits.INT2IF     = 0;    // external interrupt handled by extra IRS
    CNEN0Bbits.CNIE0B9  = 0;    // deactivate rising flag interrupt
    CNEN1Bbits.CNIE1B9  = 1;    // activate falling flag interrupt
    
    INTCONbits.INT2EP   = 0;    // clear interrupt at falling edge bit
    IEC0bits.INT2IE     = 1;    // enable external interrupt 2
    
    // RC8 (AN14) is Potentiometer input
    ANSELCbits.ANSC8 = 1;   // RC8 input
    TRISCbits.TRISC8 = 1;
    AD1CHSbits.CH0SA = 14;  // AN14 input
    AD1CON1bits.ON = 1;     // ADC on
    AD1CON1bits.MODE12 = 1; // 12-bit mode
    AD1CON1bits.SSRC = 7;   // Auto-convert
    AD1CON1bits.ASAM = 1;   // Auto-sample
    AD1CON2bits.SMPI = 15;  // Interrupt every 16th sample
    AD1CON3bits.ADRC = 1;   // Select fast RC Osc
    AD1CON3bits.ADCS = 255; // Fad = 24,000,000 Hz / 510 = 47058 Hz
    AD1CON3bits.SAMC = 31;  // Sample Freq = 47058 Hz / 31 = 1518 Hz
    IEC1bits.AD1IE = 1;     // ADC interrupt enable
    IPC8bits.AD1IP = 3;     // ADC priority 4
    IPC8bits.AD1IS = 1;
}
void rgbw_to_uart(uint8_t in[], uint16_t out[]){// in =  u8 g, u8 r, u8 b, u8 w
    int i,j;
    bool temp[8];
    bool bits[32];//= {r,g,b,w}; bits in order nessesarc for LED
    
    // transfer rgbw values to one 32-Bit array 
    // r[7] -> bits[0]
    // w[0] -> bits[31]
    for(i=0;i<4;i++){
        for(j = 0; j<8; j++){ temp[7-j] = in[i] & 1<<j; }
        for(j=0;j<8;j++){ bits[i*8+j] = temp[j]; }
    }

    // create uart messages
    for(i=0;i<11;i+=1){ // loop through all uart messages for on LED message
        unsigned int uart_val= 0b010001000; // default uart message with only zeros as LED-Bits
        // set longer high time for ones
        if(bits[i*3]){  uart_val |= 0b1;}
        if(bits[i*3+1]){uart_val |= 0b10000;}
        if(bits[i*3+2]){uart_val |= 0b100000000;}
        // clear last bit
        if(i==10){uart_val &= 0b001111111;} // set thirty-third LED-Bit to nothing (no high puls)  
        out[i]=~uart_val; // convert from positiv to negativ logic
    }
}
void create_rainbow(){
    int i, j;
    // create rgbw values
    for(i=0;i<LEDS;i++){
        HsvColor hsv = {.h = 255*i/(LEDS-1), .s = 255, .v = 32};
        RgbColor rgb = HsvToRgb(hsv);
        rainbow_color[i][0] = rgb.g;
        rainbow_color[i][1] = rgb.r;
        rainbow_color[i][2] = rgb.b;
        rainbow_color[i][3] = rgb.w;
    }
    // convert rgbw values to uart messages
    for(i=0; i<LEDS; i++){
        rgbw_to_uart(rainbow_color[i], uart_rainbow[i]);
    } // rgb to uart messages
    rgbw_to_uart(dark, uart_off); // create uart message for LED off
    
    // fill uart
}
void __ISR(_EXTERNAL_2_VECTOR, IPL2SOFT) buttonInterrupt(void){
    /*
    * pin falling edge interrupt
    * - occur if button s1 is pressed
    * - changing running light direction
    */
    dir = -1* dir;          // change running light direction
    IFS0bits.INT2IF = 0;    // clear interrupt flag
}
void __ISR(_ADC_VECTOR, IPL3AUTO) ADCHandler(void){
    /*
    * adc measuring finished interrupt
    * - occurs if the autosampeling finished a conversion
    * - update the timer auto reset value
    */
    adc_offset++;
    if(adc_offset == 10){
        PR1 = 938+(938*99*ADC1BUF0/4095); // max freq for new led 100Hz , min freq 1Hz
        adc_offset = 0;
    }
    IFS1bits.AD1IF = 0;
}
void __ISR(_TIMER_1_VECTOR, IPL4SOFT) nextOutput(void) {
    /*
    * timer interrupt
    * - occurs on timer auto reset
    * - the auto reset value is depending from adc value
    */
    uint8_t i;
    // set old LED to dark in uart_map
    for(i=0;i<11;++i){
        uart_map[pos+i] = &(uart_off[i]); 
    }
    // step direction for running lights
    pos += dir;
    // check if pos is out of boundaries
    if(pos>=LEDS){  pos = 0;}
    else if(pos<0){ pos = LEDS-1;}
    
    // set new LED to color in uart_map
    for(i=0;i<11;++i){
        uart_map[pos+i] = &(uart_rainbow[pos][i]); 
    }
  
    uart_msg = FIRST_UART_MSG; // set uart_msg pointer to first uart message
    IFS0bits.T1IF = 0; // reset interrupt flag
    IEC1bits.U1TXIE = 1; // enable Interrupt
}
void __ISR(_UART1_TX_VECTOR, IPL5SRS) display(){
    /*
    * uart tx interrupt --
    * - occurs it there is empty space in uart tx fifo buffer
    * - filling new uart message in to fifo buffer
    */
    
    // transfer uart message to fifo buffer
    asm volatile( // U1TXREG = *uart_msg;
    ".set at            \n\t"
    "sh %0, U1TXREG     \n\t" // store 
    ".set noat          \n\t"
    : "+r" (*uart_msg) ::);
       
    // increment uart_pos to get next uart_message by next loop 
    asm volatile( // uart_msg++;
    ".set at            \n\t"
    "addiu %0 , %0, 2 \n\t" // increment by 2 because of 16-bit 
    ".set noat          \n\t" 
    : "+r" (uart_msg) ::);
    
    // disable Interrupt
    asm volatile( // IEC1bits.U1TXIE = (bool)(uart_msg <= LAST_UART_MSG);
    ".set at            \n\t"
    "sle $t0, %0, %1    \n\t" // compare uart_msg <= LAST_UART_MSG
    "lw  $t1, IEC1      \n\t" // load IEC1
    "ins $t1, $t0, 22, 1\n\t" // insert interrupt enable status
    "sw  $t1, IEC1      \n\t" // store IEC1
    ".set noat          \n\t"
    : "+r" (uart_msg)
    : "r"  (LAST_UART_MSG):);
       
     
    
    // clear interrupt flag
    asm volatile( // IFS1bits.U1TXIF = 0;
    ".set at                \n\t"
    "lw  $t0, IFS1          \n\t" // load IFS1 from memory
    "ins $t0, $zero, 22, 1  \n\t" // clear interrupt flag bit
    "sw  $t0, IFS1          \n\t" // write back to memory with flag cleared
    ".set noat              \n\t"
    :::);
    
}
void loop() {
    // create uart muster
    create_rainbow(); 
    // start LED running light
    IFS0bits.T1IF   = 0;    // clear timer interrupt flag
    T1CONbits.ON    = 1;    // start timer
    // main loop
    while(1) {}
}
int main(void) {
    setup();
    loop();
}