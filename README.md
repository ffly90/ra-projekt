# ra-projekt

# UART
1 Startbit normal low
9 Datebits 
2 Stopbits normal high
negative Logik
1 Sartbit high
9 Databits
2 Stopbist low

## HW
## Register
UxSTA.UTXINV = 0 -> Ruhezustand = 0



|Register|Bits|Wert|Bedeutung||
|-|-|-|-|-|
|U1MODE|ABAUD|0|Auto-Baud disabled||
||ACTIVE    |-  | UARTx Running Status bit| nicht setzen|
||BRGH      |1  | High Baud Rate Enabled| 4x baud clock enabled|
||CLKSEL    |01 | UARTx Clock Selection bits| The UARTx clock is the SYSCL|
||IREN      |0  | IrDA® Encoder and Decoder Enable bit| IrDA is disabled|
||LPBACK    |0  | UARTx Loopback Mode Select bit| Loopback mode is disabled|
||OVFDIS    |0  | Run During Overflow Condition Mode bit| When an Overflow Error (OERR) condition is detected, the shift register stops accepting new data(Legacy mode)|
||PDSEL     |11 | Parity and Data Selection bits| 9 -bit data, no parity|
||RTSMD     |0  | Mode Selection for UxRTS Pin bit| UxRTS pin is in Flow Control mode|
||RXINV     |0  | Receive Polarity Inversion bit| UxRX Idle state is ‘1’|
||SIDL      |0  | SIDL| Continues operation in Idle mode|
||SLPEN     |1  | UARTx Run During Sleep Enable bit| UARTx clock runs during Sleep|
||STSEL     |1  |  Stop Selection bit| 2 Stop bit|
||ON        |1  | UARTx Enable bit| UARTx is enabled; UARTx pins are controlled by UARTx, as defined by the UEN[1:0] and UTXEN control bits|
||UEN       |00 | UEN| UxTX and UxRX pins are enabled and used; UxCTS and UxRTS/UxBCLK pins are controlled by corresponding bits in the PORTx register|
||WAKE      |0  | Enable Wake-up on Start bit Detect During Sleep Mode bit| Wake-up is disabled|
|U1STA|ADDEN|0  |  Address Character Detect bit (bit 8 of received data = 1)| Address Detect mode is disabled|
||ADDR      |0  | Address Detect mode is disabled||
||FERR      |   | Frame Error Status Bit| ReadOnly |
||MASK      |0  | UARTx Address Match Mask bits| ADDR[x] is not used to detect the address matc|
||OERR      |   | Receive Buffer Overrun Error Status bit| Receive buffer has not overflowed|
||PERR      |   | Parity Error Status Bit| ReadOnly|
||RIDLE     |   | Receiver Idle Bit| ReadOnly|
||TRMT      |   | Transmit Shift Register is Empty Bit | ReadOnly WICHTIG FÜR UNS |
||URXDA     |   | UART Receive Buffer data available bit | ReadOnly|
||URXEN     |0  | UART Receiver Enable Bit| Receiver is disabled|
||URXISEL   |0  | UARTx Receive Interrupt Mode Selection bit | UARTx Receive Interrupt Mode Selection bit Können wir verwenden für schieben über Interrupt |
||UTXBF||UARTx Transmit Buffer Full Status bit | ReadOnly das Brauchen wir um zuwissen das wir schreiben dürfen |
||UTXBRK    |0  | Transmit Break bit| Break transmission is disabled or has completed|
||UTXEN     |1  | UARTx Transmit Enable bit | UARTx transmitter is enabled, UxTX pin is controlled by UARTx (if ON = 1)|
||UTXINV    |1  | UARTx Transmit Polarity Inversion bit| UxTX Idle state is ‘1’|
||UTXISEL   |00 | UARTx TX Interrupt Mode Selection bits| Interrupt is generated and asserted while the transmit buffer contains at least one empty space|

||||||
||||||
||||||

U1TXREG UART1 Regsiter mit 0-7 Bit
TX8 für Bit 8