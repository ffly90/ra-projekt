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

unsigned char dark[]={0,0,0,0};          // GRBW values for LED off
unsigned char rainbow_color[LEDS][4];    // GRBW values for rainbow
int uart_rainbow[LEDS][11];     // UART values for each LED
int uart_off[11];               // UART values for LED off
char dir = 1;                   // init direction
char pos=0;                     // current active LED
int uart_pos = 0;               // #UART message
unsigned char adc_offset = 0;   // adc sample offset

int buffer[LEDS][LEDS];

void setup() { 
	SYSTEM_Initialize();   //set 24 MHz clock for CPU and Peripheral Bus
                           //clock period = 41,667 ns = 0,0417 us
    
    //Zähler Konfiguration
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

void __ISR(_EXTERNAL_2_VECTOR, IPL2SOFT) buttonInterrupt(void){
    dir = -1* dir;
    IFS0bits.INT2IF = 0;
}

void __ISR(_ADC_VECTOR, IPL3AUTO) ADCHandler(void){
    adc_offset++;
    if(adc_offset == 10){
        PR1 = 938+(938*99*ADC1BUF0/4095); // max freq for new led 100Hz , min freq 1Hz
        adc_offset = 0;
    }
    IFS1bits.AD1IF = 0;
}
void create_rainbow(){
    int i, j;
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

void __ISR(_TIMER_1_VECTOR, IPL4SOFT) nextOutput(void) {
    // step direction for running lights
    pos += dir;
    if(pos>=LEDS){
        pos = 0;
    }else if(pos<0){
        pos = LEDS-1;
    }
    IFS0bits.T1IF = 0; // reset interrupt flag
    IEC1bits.U1TXIE = 1; // enable Interrupt
}


void __ISR(_UART1_TX_VECTOR, IPL5SRS) display(){
    asm volatile("nop");
//    asm volatile(
//    ".set at\n\t"
//    "div $0, %[uart_pos], %[eleven]\n\t"
//    "mflo $t0\n\t"    
//    "mfhi $t1\n\t"
//    "sll $t1, $t1, 2\n\t"
//    "beq $t0, %[pos], 1f\n\t"
//    
//    "add $t0, $t1, %[uart_off]\n\t"
//    "lw $t0, 0($t0)\n\t"
//    "sw $t0, U1TXREG\n\t"
//    "b 2f\n\t"
//    "nop\n\t"
//    
//        "1:"
//    "sll $t2, %[pos], 2\n\t"
//    "mul $t2, $t2, %[eleven]\n\t"
//    "add $t2, $t2, %[uart_rainbow]\n\t"
//    "add $t2, $t2, $t1\n\t"
//    "lw $t2, 0($t2)\n\t"
//    "sw $t2, U1TXREG\n\t"
//    "b 2f\n\t"
//    "nop\n\t"
//    "2:"
//    "nop\n\t"
//    :
//    :[uart_rainbow] "r" (uart_rainbow), [pos] "r" (pos), [eleven] "r" (11), [uart_pos] "r" (uart_pos), [uart_off] "r" (uart_off)
//    :"t0", "t1", "t2"
//    );
    
//    asm volatile
//    (
//    ".set at\n\t"
//      "divu $0, %[uart_pos], %[eleven]\n\t"
//      "mflo $t0\n\t"
//      "beq $t0, %[pos], 1f\n\t"
//      "nop\n\t"
//      "bne $t0, %[pos], 2f\n\t"
//      "nop\n\t"
//      "1:"
//      "sll $t0, %[pos], 2\n\t"
//    "mul $t0, $t0, %[eleven]\n\t"
//    "divu $0, %[uart_pos], %[eleven]\n\t"
//    "mfhi $t1\n\t"
//    "sll $t1, $t1, 2\n\t"
//    "add $t0, $t0, %[uart_rainbow]\n\t"
//    "add $t0, $t0, $t1\n\t"
//    "lw $t0, 0($t0)\n\t"
//    "sw $t0, U1TXREG\n\t"
//    "b 3f\n\t"
//    "nop\n\t"
//    "2:"
//    "divu $0, %[uart_pos], %[eleven]\n\t"
//        "mfhi $t1\n\t"
//        "sll $t1, $t1, 2\n\t"
//        "add $t1, $t1, %[uart_off]\n\t"
//        "lw $t1, 0($t1)\n\t"
//        "sw $t1, U1TXREG\n\t"
//    "3:"
//    "nop\n\t"
//    :
//    :[pos] "r" (pos), [eleven] "r" (11), [uart_pos] "r" (uart_pos), [uart_rainbow] "r" (uart_rainbow), [uart_off] "r" (uart_off)
//    :
//    );
    
    asm volatile("nop\n\t");
    asm volatile(
        ".set at\n\t"
        "sll $t0, %[pos], 2\n\t"
        "mul $t0, $t0, %[eleven]\n\t"
        "divu $0, %[uart_pos], %[eleven]\n\t"
        "mfhi $t1\n\t"
        "mflo $t3\n\t"
        "beq $t3, %[pos], if \n\t"
        
        "sll $t2, $t1, 2\n\t"
        "add $t1, $t2, %[uart_off]\n\t"
        "lw $t1, 0($t1)\n\t"
        "sw $t1, U1TXREG\n\t"
        "j endif\n\t"
        
        "if:"
        "sll $t2, $t1, 2\n\t"
        "add $t0, $t0, %[uart_rainbow]\n\t"
        "add $t0, $t0, $t2\n\t"
        "lw $t0, 0($t0)\n\t"
        "sw $t0, U1TXREG\n\t"

        "endif:"
        :
        : [uart_rainbow] "r" (uart_rainbow), [pos] "r" (pos), [eleven] "r" (11), [uart_pos] "r" (uart_pos), [uart_off] "r" (uart_off)
        : "t0", "t1", "t2", "t3"
    );

    
//        if (pos == (uart_pos / 11))
//    {
//        //U1TXREG = uart_rainbow[pos][uart_pos % 11 ];
//        
//        asm volatile(
//        ".set at\n\t"
//        "sll $t0, %[pos], 2\n\t"
//        "mul $t0, $t0, %[eleven]\n\t"
//        "divu $0, %[uart_pos], %[eleven]\n\t"
//        "mfhi $t1\n\t"
//        "sll $t2, $t1, 2\n\t"
//        "add $t0, $t0, %[uart_rainbow]\n\t"
//        "add $t0, $t0, $t2\n\t"
//        "lw $t0, 0($t0)\n\t"
//        "sw $t0, U1TXREG"
//        :
//        : [uart_rainbow] "r" (uart_rainbow), [pos] "r" (pos), [eleven] "r" (11), [uart_pos] "r" (uart_pos), [uart_off] "r" (uart_off)
//        : "t0", "t1", "t2"
//        );
//    }
//    else
//    {
//        asm volatile(
//        ".set at\n\t"
//        "divu $0, %[uart_pos], %[eleven]\n\t"
//        "mfhi $t1\n\t"
//        "sll $t2, $t1, 2\n\t"
//        "add $t1, $t2, %[uart_off]\n\t"
//        "lw $t1, 0($t1)\n\t"
//        "sw $t1, U1TXREG"
//        :
//        : [uart_off] "r" (uart_off), [eleven] "r" (11), [uart_pos] "r" (uart_pos)
//        : "t1", "t2"
//        );
//        //U1TXREG = uart_off[uart_pos % 11];
//    }
    
//    asm volatile(
//    ".set at            \n\t"
//    ".set noreorder     \n\t"
//    "li $t0, 11        \n\t" // load 11 for division
//    "divu $0, %0, $t0 \n\t" // divide uart_pos by 11
//    "mflo $t1           \n\t" // get integer from division
//    "mfhi $t2           \n\t" // get remainder from division
//    "bne %3, $t1, else \n\t" // if int quotient is not same as pos 
//    "sll $t2, $t2, 2    \n\t" // calc address offset (needs to bee executed in both cases)
//
//    "sll $t1, %3, 2    \n\t" // to get address offset 
//    "mul $t1, $t1, $t0 \n\t"
//    "addu $t1, %1, $t1 \n\t" // calc absolut address
//    "addu $t1, $t1, $t2 \n\t" // calc absolute address
//    "lw $t0, 0($t1)     \n\t" // load next uart message
//    "sw $t0, U1TXREG    \n\t" // transfer message to fifo transmit buffer 
//    "j endif            \n\t" // leave true branch
//    "nop\n\t"
//    
//    "else:                  " // begin false branch
//    "addu $t2, %2, $t2 \n\t" // calc absolute address
//    "lw $t0, 0($t2)     \n\t" // load next uart message
//    "sw $t0, U1TXREG    \n\t" // transfer message to fifo transmit buffer 
//    "endif:                  " // after if
//    : "+r" (uart_pos) 
//    : "r" (uart_rainbow), "r" (uart_off), "r" (pos)
//    : "t0", "t1", "t2"
//    );
    /* 
    %0 := uart_pos
    %1 := uart_rainbow
    %2 := uart_off
    %3 := pos
    */
    
//    if(pos  == ( uart_pos / 11 )  ){// check if current uart_pos is at the activ LED
//        U1TXREG = uart_rainbow[pos][uart_pos % 11 ]; // Write the data byte to the UART.
//    }else{
//        U1TXREG = uart_off[uart_pos % 11 ]; // Write the data byte to the UART.
//    }

    // increment uart_pos to get next uart_message by next loop
    uart_pos++;
    if(uart_pos >= 11*LEDS){ // end uart transmision
        uart_pos = 0;
        IEC1bits.U1TXIE = 0; // disable Interrupt
    }
    IFS1bits.U1TXIF = 0;
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