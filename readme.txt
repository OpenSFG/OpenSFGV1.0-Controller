// Smart and Interconnected Aquaponics System (SIAPS)
// 
// OpenSFG's SIAPS (Standalone Controller)
//  
// This work is licensed under a Creative Commons Attribution-Share Alike 3.0 License.
// You are free to share and adapt under the following terms:
// Attribution: 
//  You must give appropriate credit, provide a link to the license, and indicate if changes were made. 
//  You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.
// ShareAlike:
//    If you remix, transform, or build upon the material, you must distribute your contributions under the same license as the original.
//
//
// Released                  8/1/2015      Myron Richard Dennison
//
// ****************************************************************************
// Features:
//    1. auto grow light (using timestamp)
//    2. ambient temperature monitoring and logging (DHT11)
//    3. relative humidity sensor monitoring and logging (DHT11)
//    4. water temperature monitoring and logging (DS18B20)
//    5. water flow rate monitoring and logging
//    6. water pH level monitoring and logging
//    7. dissolved oxygen monitoring and logging
//    8. auto feeder provision
//    9. debug switch
//    10.status LED
//        R - FAIL/ALARM
//        G - Probe Status, Successful DB update indicator, etc
//        B - PASS; Everything is OK
//    11. MySQL database to store data     
//
// Arduino YUN Pin Assignments
//    Digital  
//    0                      - (RX)
//    1                      - (TX)
//    2       		     - (SDA)- \---all sensors using I2C protocol (e.g. DO, pH, etc)               
//    3                      - (SCL)  |
//    4                      - LCD RS Control Pin                  
//    5                      - Relay Module Driver (Grow Light Switch)
//    6                      - Auto Feeder Control Pin   
//    7                      - DHT11 (AT and RH sensor)
//    8                      - LCD EN Control Pin
//    9, 10, 11, 12          - LCD 4-bit Data Lines
//    13                     - Pushbutton Debug Switch
//    Analog
//    0                      - (R) STATUS LED (common Cathode)  : FAIL/ALARM
//    1                      - (G)                              : PASS
//    2                      - (B)                              : Probe Status and Successful DB update indicator
//    3                      - Moisture
//    4                      - Flow Rate data pin
//    5                      - DS18S20 Signal pin
//
// HD44780 LCD Pinout
//    1                      - GND                    
//    2                      - VCC (LCD Controller Power (+3 to +5V)
//    3                      - VLCD (LCD Display Bias (+5 to -5V)--> Connect to GND
//    4                      - RS (Register Select: H: Data L: Command)
//    5                      - R/W (H: Read L: Write)--> Connect to GND
//    6                      - Enable (Data strobe, active high) 
//    7, 8, 9, 10,           - Data: DB0(LSB)-DB3 --> 4-bit mode --\      
//    11, 12, 13, 14         - Data lines: DB4-DB7(MSB)            |--> 8-bit mode 
//    15                     - LED Backlight Anode --> Connect to 10KO potentiometer
//    16                     - LED Backlight Cathode --> Connect to GND

Schematic Diagram: http://downloads.opensfg.org/SIAPS/v1.0/OpenSFGV1.0_schem.pdf
Breadboard file: http://downloads.opensfg.org/SIAPS/v1.0/OpenSFGV1.0_bb.pdf
BOM: http://downloads.opensfg.org/SIAPS/v1.0/OpenSFGV1.0_bom.html
Fritzing file: http://downloads.opensfg.org/SIAPS/v1.0/OpenSFGV1.0.fzz