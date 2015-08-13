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
//    15                     - LED Backlight Anode --> Connect to 10KΩ potentiometer
//    16                     - LED Backlight Cathode --> Connect to GND

//---------------------------------------->
// Libraries
//---------------------------------------->
#include <Process.h>                 // Linux processes
#include <Bridge.h>                  // To pass information between two processors.
                                     // The Bridge library simplifies communication between the ATmega32U4 and the AR9331.
#include <LiquidCrystal.h>           // Allows the board to control LiquidCrystal displays (LCDs) 
                                     // based on the Hitachi HD44780 (or a compatible) chipset.
#include <Wire.h>                    // Allows the board to communicate with I2C/TWI devices.
#include <OneWire.h>                 // DS18B20
#include <idDHT11.h>                 // Library for reading the values from the DHT sensor

#define doAddress 97                  //default I2C address for EZO DO Circuit.
#define phAddress 99                  //default I2C address for EZO pH Circuit.

//---------------------------------------->
// I/O Pin Assignments
//---------------------------------------->
// Digital Pins
const int GrowLEDPin = 5;         // Relay Module (Auto Light Switch) LED (Red)
const int debugSwitchPin = 13;      // Pushbutton switch  

// Analog Pins
const int ledPinR = A0;             // tri-color Red pin 
const int ledPinG = A1;             // tri-color Green pin 
const int ledPinB = A2;             // tri-color Blue pin 
const int moistureSensorPin = A3;   // moisture sensor data pin  
const int flowSensorPin = A4;       // water flow sensor data pin
const int DS18S20_Pin = A5;         // DS18S20 Signal pin 
//---------------------------------------->

//---------------------------------------->
// Initialize LCD controls and data lines
//---------------------------------------->
LiquidCrystal lcd(4, 8, 9, 10, 11, 12);
//---------------------------------------->

//---------------------------------------->
// Initialize DS18S20
//---------------------------------------->
OneWire ds(DS18S20_Pin);     
//---------------------------------------->

//---------------------------------------->
// Initialize DHT11 
//---------------------------------------->
// External Interrupts: 3 (interrupt 0), 2 (interrupt 1), 0 (interrupt 2), 1 (interrupt 3) and 7 (interrupt 4). 
// These pins can be configured to trigger an interrupt on a low value, a rising or falling edge, or a change in value.
//
const int idDHT11pin = 7;           // HW interrupt pin for the DHT sensor
const int idDHT11intNumber = 4;     // External HW interrupt number for pin 7
//
// ISR callback wrapper
void dht11_wrapper();               // must be declared before the instantiating the DHT object
//
// Instantiate the sensor object
idDHT11 DHT11(idDHT11pin,idDHT11intNumber,dht11_wrapper);
//---------------------------------------->

//---------------------------------------->
// Global Variables
//---------------------------------------->

//---------------------------------------->
// define sensor's display sequence 
//---------------------------------------->
const int AMBIENT = 1;
const int WATER = 2;
const int HUMIDITY = 3;
const int FLOWRATE = 4; 
const int PH = 5;
const int DO = 6;
const int TIME = 7;
//---------------------------------------->

//---------------------------------------->
// Timer variables
//---------------------------------------->
long lcdReadingPreviousMillis = 0;        // last lcd update  
long wtcReadingPreviousMillis = 0;        // last wtc update
long updateGAEpreviousMillis = 0;         // last GAE update
long dhtSensorPreviousMillis = 0;         // last DHT sensor update
long flowRatePreviousMillis = 0;          // last flow rate update
long dO2PreviousMillis = 0;               // last moisture update
long waterPhPreviousMillis = 0;           // last pH level update
long lcdHeaderPreviousMillis = 0;         // last lcd header update
long lcdErrorPreviousMillis = 0;          // last lcd header update
//---------------------------------------->

//---------------------------------------->
// other sensors
//---------------------------------------->
float rh;                           // relative humidity
float atc;                          // temperature in Celsius
float wtc;                          // water temperature in Celsius
float fr;                           // water flow rate

//---------------------------------------->
// ezo sensor variables
//---------------------------------------->
byte code=0;                              //used to hold the I2C response code. 
char sensor_data[20];                     //we make a 20 byte character array to hold incoming data from the DO circuit. 
byte in_char=0;                           //used as a 1 byte buffer to store in bound bytes from the DO Circuit.   
float dO2_float;                          //float var used to hold the float value of the DO. 
float pH_float;                           //float var used to hold the float value of pH data. 
float ezo_float;                          //float var used to hold the float value of ezo data 
byte i=0;                                 //counter used for do_data array.
bool getSensorDataSuccess = false;        //
float bogus_dO2;                          // bogus dO value
float bogus_pH;                           // bogus pH value
//---------------------------------------->

//---------------------------------------->
// other variables
//---------------------------------------->
String error;                             // holds the ERROR message
bool alert = false;                       // initialize alert to false
int hours, minutes, seconds;              // hold the current time
bool initialCheck = true;                 // first check
bool adlawan = true;                      // between sunrise and sunset
int sensor = AMBIENT;                     // initialize sensor variable to AMBIENT
int currentSwitchState = LOW;
int previousSwitchState = LOW;
 
//---------------------------------------->
// Initialize flow rate sensor
//---------------------------------------->
// count how many pulses!
volatile uint16_t pulses = 0;
// track the state of the pulse pin
volatile uint8_t lastflowpinstate;
// you can try to keep time of how long it is between pulses
volatile uint32_t lastflowratetimer = 0;
// and use that to calculate a flow rate
volatile float flowrate;
// Interrupt is called once a millisecond, looks for any pulses from the sensor!
SIGNAL(TIMER0_COMPA_vect) {
  uint8_t x = digitalRead(flowSensorPin);
  
  if (x == lastflowpinstate) {
    lastflowratetimer++;
    return; // nothing changed!
  }
  
  if (x == HIGH) {
    //low to high transition!
    pulses++;
  }
  lastflowpinstate = x;
  flowrate = 1000.0;
  flowrate /= lastflowratetimer;  // in hertz
  lastflowratetimer = 0;
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
  }
}
//---------------------------------------->

void setup()
{
  Serial.begin(9600);    
  //Serial.println("Starting bridge...\n");
  digitalWrite(13, LOW);  
  Wire.begin();                       // Initiate the Wire library and join the I2C bus as a master or slave.
  Bridge.begin();                     // make contact with the linux processor
  digitalWrite(13, HIGH);             // Led on pin 13 turns on when the bridge is ready
  delay(2000);                 	      // wait 2 seconds  
  
  // Configures the specified pins to behave either as an input or an output.
  pinMode(ledPinR, OUTPUT);
  pinMode(ledPinG, OUTPUT);
  pinMode(ledPinB, OUTPUT); 
  pinMode(GrowLEDPin, OUTPUT);  
  pinMode(debugSwitchPin, INPUT);     
  pinMode(DS18S20_Pin, INPUT);
  pinMode(flowSensorPin, INPUT);
  
  digitalWrite(flowSensorPin, HIGH);
  lastflowpinstate = digitalRead(flowSensorPin);
  useInterrupt(true);
  
  // initialize output pins
  digitalWrite(GrowLEDPin, HIGH);        //    
  digitalWrite(ledPinR, LOW);            // 
  digitalWrite(ledPinG, HIGH);           // 
  digitalWrite(ledPinB, LOW);            // 
  
  // Setup LCD's initial display
  lcd.begin(16, 2);		      			// set up lCD's number of columns and rows
  lcd.clear();			      			// clears the LCD screen and position the cursor on the upper-left corner of the screen
  lcd.print("  OpenSFG V1.0  ");
  lcd.setCursor(0, 1);		      		// set the location at which the subsequent text written to the LCD will be displayed
  lcd.print("PROOF OF CONCEPT");
  delay(3000);  
}

// This wrapper is in charge of calling 
// mus be defined like this for the lib work
void dht11_wrapper() {
  DHT11.isrCallback();
} 

void loop()
{   
  // 10-byte character arrays to hold the sensor data 
  char atemp_str[10];                
  char wtemp_str[10];
  char rh_str[10];
  char fr_str[10];
  char ph_str[10];
  char dO2_str[10];
  
  unsigned long currentMillis = millis();  // current time
  
  // DON'T FORGET TO REMOVE THIS!!!!!!!!!!!
  bogus_pH = 6 + (0.13 * (random(6, 9)));
  pH_float = bogus_pH;
  bogus_dO2 = 6 + (0.13 * (random(6, 9)));
  dO2_float = bogus_dO2;
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  
  switchStateChange();
  // initial time check for the auto grow light
  if(initialCheck)
    growLight();  

  if(currentMillis - dhtSensorPreviousMillis > 2000){
    DHT11Sensor();
    //atc = 30.5;
    //rh = 64.5;
    dhtSensorPreviousMillis = currentMillis;
  }
  
  if(currentMillis - wtcReadingPreviousMillis > 4000){
    wtc = getWTemp(); 
    wtcReadingPreviousMillis = currentMillis;
  }
  
  if(currentMillis - flowRatePreviousMillis > 6000){
    // modify this according to your flow rate sensor
	float liters = pulses;
    liters /= 8.1;
    liters -= 6;
    liters /= 60.0;
    fr = (flowrate / 8.1) * 60;
    flowRatePreviousMillis = currentMillis;
  } 
 /******************** uncomment these two functions if you are using your d0 and pH probles ***************
  if(currentMillis - dO2PreviousMillis > 8000){  
    //---------------------------------------->
    // Get DO data from the sensor
    //---------------------------------------->      
    getWaterSensorData(doAddress);
    if(getSensorDataSuccess){
      dO2_float = ezo_float;      
    }
    else{
      //Setup LCD's initial display
      lcd.begin(16, 2);		              // set up lCD's number of columns and rows
      lcd.clear();			      // clears the LCD screen and position the cursor on the upper-left corner of the screen
      lcd.print("ERROR: Failed to");  
      lcd.setCursor(0, 1);		      // set the location at which the subsequent text written to the LCD will be displayed
      lcd.print("get DO data.....");
      delay(2000);
    } 
    dO2PreviousMillis = currentMillis;   
  } 
  
  if(currentMillis - waterPhPreviousMillis > 10000){
    //---------------------------------------->
    // Get pH data from the sensor
    //---------------------------------------->    
    getWaterSensorData(phAddress);
    if(getSensorDataSuccess){
      pH_float = ezo_float;      
    }
    else{
      //Setup LCD's initial display
      lcd.begin(16, 2);		              // set up lCD's number of columns and rows
      lcd.clear();			      // clears the LCD screen and position the cursor on the upper-left corner of the screen
      lcd.print("ERROR: Failed to");  
      lcd.setCursor(0, 1);		      // set the location at which the subsequent text written to the LCD will be displayed
      lcd.print("get pH data.....");
      delay(2000);
    }
    waterPhPreviousMillis = currentMillis;
  } */
  
  // display sensor readings on the LCD
  lcdDisplay();    
  
  //digitalWrite(ledPinG, LOW);    
  if(currentMillis - updateGAEpreviousMillis > 60000){
    String timeStamp = getTimeStamp();  

    growLight();     

    digitalWrite(ledPinB, HIGH);
    //dtostrf (CURRENT VALUE, WIDTH, PRECISION, BUFFER)
    String atc_s = dtostrf((int)atc, 3, 1, atemp_str);
    String wtc_s = dtostrf((int)wtc, 3, 1, wtemp_str);
    String rh_s = dtostrf((int)rh, 3, 1, rh_str);
    String fr_s = dtostrf((int)fr, 3, 1, fr_str);
    String ph_s = dtostrf(pH_float, 2, 1, ph_str); 
    String dO2_s = dtostrf(dO2_float, 2, 1, dO2_str);
    //---------------------------------------->
    // Update SQL Server
    //---------------------------------------->
    String response;
    // Launch "curl" command to send HTTP request to GAE server
    Process sendRequest;		        // Create a process 
    sendRequest.begin("curl");	                // start with "curl" command
    sendRequest.addParameter("youDomain/yourSqlScript.php?ATemp=" + atc_s + "&WTemp=" + wtc_s + "&RH=" + rh_s + "&FR=" + fr_s + "&PH=" + ph_s + "&DO2=" + dO2_s + "&DateTime=" + timeStamp);
    sendRequest.run();		                // Run the process and wait for its termination
  
    // Captures the HTTP response
    while (sendRequest.available() > 0) {
      char x = sendRequest.read();
      response += x;      
    }
    // Ensure the last bit of data is sent.
    Serial.flush();   
    
    if (response == "Ok"){
        //toggle ledPinB
        for(int i=0; i<5; i++){
          digitalWrite(ledPinB, !digitalRead(ledPinB)); 
          delay(500);
        } 
    }  
    else{
      //Setup LCD's initial display
      lcd.begin(16, 2);		              // set up lCD's number of columns and rows
      lcd.clear();			      // clears the LCD screen and position the cursor on the upper-left corner of the screen
      lcd.print("ERROR: FAILED TO");  
      lcd.setCursor(0, 1);		      // set the location at which the subsequent text written to the LCD will be displayed
      lcd.print("UPDATE SQLSERVER");
      delay(2000);
    } 
    updateGAEpreviousMillis = currentMillis;
  }
}

// ----------------------------------------------------------------
void switchStateChange(){
  // reads and assign the value of debugSwitchPin
  currentSwitchState = digitalRead(debugSwitchPin);  

  //Check if the input just went from LOW to HIGH
  if((currentSwitchState == HIGH) && (previousSwitchState == LOW)){
    // sets the debug LED and the grow LED     
    digitalWrite(GrowLEDPin, !GrowLEDPin);
  } 
  previousSwitchState = currentSwitchState;     
}

// ----------------------------------------------------------------
void growLight(){  
  getHourMinSec();
  if(adlawan)
    digitalWrite(GrowLEDPin, LOW);          // Active low???
  else 
    digitalWrite(GrowLEDPin, HIGH);  
  
  initialCheck = false;   
}

// ----------------------------------------------------------------
// Update system stats LED.
// Red: Alarm Detected
// Green: All sensor readings are within the limits 
// ----------------------------------------------------------------
void updateSystemStatusLED(){
  // check for Critical conditions and ERROR
  if(!alert){
    digitalWrite(ledPinR, LOW);
    if(!digitalRead(ledPinG))
      digitalWrite(ledPinG, HIGH);  
  }
  else{
    digitalWrite(ledPinR, HIGH);  
    if(digitalRead(ledPinG))
      digitalWrite(ledPinG, LOW);
  } 
}

// ----------------------------------------------------------------
// Display
// ----------------------------------------------------------------
void lcdDisplay(){   
  unsigned long currentMillis = millis();      // current time
  
  if(currentMillis - lcdReadingPreviousMillis > 2000) {
    if(atc > 32 || wtc > 32){
      alert = true;
      updateSystemStatusLED();
      error = "OVERTEMP: " + (String)atc + "°C";     
      lcd.begin(16, 2);		              // set up lCD's number of columns and rows
      lcd.clear();			      // clears the LCD screen and position the cursor on the upper-left corner of the screen
      lcd.print("ALARM DETECTED!!");  
      lcd.setCursor(0, 1);		      // set the location at which the subsequent text written to the LCD will be displayed
      lcd.print(error);
    }
    else if((int)pH_float < 1){
      alert = true;
      updateSystemStatusLED();
      error = "LOW pH LEVEL:" + (String)pH_float;     
      lcd.begin(16, 2);		              // set up lCD's number of columns and rows
      lcd.clear();			      // clears the LCD screen and position the cursor on the upper-left corner of the screen
      lcd.print("ALARM DETECTED!!");  
      lcd.setCursor(0, 1);		      // set the location at which the subsequent text written to the LCD will be displayed
      lcd.print(error);
    }
    else if((int)dO2_float < 1){
      alert = true;
      updateSystemStatusLED();
      error = "LOW DO LEVEL:" + (String)dO2_float;     
      lcd.begin(16, 2);		              // set up lCD's number of columns and rows
      lcd.clear();			      // clears the LCD screen and position the cursor on the upper-left corner of the screen
      lcd.print("ALARM DETECTED!!");  
      lcd.setCursor(0, 1);		      // set the location at which the subsequent text written to the LCD will be displayed
      lcd.print(error);
    }
    else{      
      alert = false;
      updateSystemStatusLED();  
      
      switch (sensor){
        case AMBIENT: 
          lcd.begin(16, 2);                   // set up lCD's number of columns and rows
          lcd.clear();			      // clears the LCD screen and position the cursor on the upper-left corner of the screen
          lcd.print(">>AMBIENT TEMP<<");  
          lcd.setCursor(0, 1);		      // set the location at which the subsequent text written to the LCD will be displayed
          lcd.print("Reading: ");
          lcd.print((int)atc);
          lcd.print((char)223);
          lcd.print("C");
          sensor = WATER;
          break;
          
       case WATER:           
          lcd.begin(16, 2);                   // set up lCD's number of columns and rows
          lcd.clear();			      // clears the LCD screen and position the cursor on the upper-left corner of the screen
          lcd.print(">>>WATER TEMP<<<");  
          lcd.setCursor(0, 1);		      // set the location at which the subsequent text written to the LCD will be displayed
          lcd.print("Reading: ");
          lcd.print((int)wtc);
          lcd.print((char)223);
          lcd.print("C");
          sensor = HUMIDITY;
          break;
          
       case HUMIDITY: 
          lcd.begin(16, 2);                   // set up lCD's number of columns and rows
          lcd.clear();			      // clears the LCD screen and position the cursor on the upper-left corner of the screen
          lcd.print(">>>>HUMIDITY<<<<");  
          lcd.setCursor(0, 1);		      // set the location at which the subsequent text written to the LCD will be displayed
          lcd.print("Reading: ");
          lcd.print((int)rh);
          lcd.print("%");
          sensor = FLOWRATE;
          break; 
          
        case FLOWRATE: 
          lcd.begin(16, 2);                   // set up lCD's number of columns and rows
          lcd.clear();			      // clears the LCD screen and position the cursor on the upper-left corner of the screen
          lcd.print(">>>FLOW RATE<<<<");  
          lcd.setCursor(0, 1);		      // set the location at which the subsequent text written to the LCD will be displayed
          lcd.print("Reading: ");
          lcd.print((int)fr);
          lcd.print("L/hr");
          sensor = PH;
          break; 
          
        case PH: 
          lcd.begin(16, 2);                   // set up lCD's number of columns and rows
          lcd.clear();			      // clears the LCD screen and position the cursor on the upper-left corner of the screen
          lcd.print(">WATER pH LEVEL<");  
          lcd.setCursor(0, 1);		      // set the location at which the subsequent text written to the LCD will be displayed
          lcd.print("Reading: ");
          lcd.print(pH_float, 1);
          sensor = DO;
          break;           

        case DO: 
          lcd.begin(16, 2);                   // set up lCD's number of columns and rows
          lcd.clear();			      // clears the LCD screen and position the cursor on the upper-left corner of the screen
          lcd.print(">>DISSOLVED O2<<");  
          lcd.setCursor(0, 1);		      // set the location at which the subsequent text written to the LCD will be displayed
          lcd.print("Reading: ");
          lcd.print(dO2_float, 1);
          lcd.print("mg/L");
          sensor = TIME;
          break; 
          
       case TIME: 
          lcd.begin(16, 2);                   // set up lCD's number of columns and rows
          lcd.clear();			      // clears the LCD screen and position the cursor on the upper-left corner of the screen
          lcd.print("**    TIME    **");  
          lcd.setCursor(0, 1);		      // set the location at which the subsequent text written to the LCD will be displayed
          lcd.print("** ");
          if(hours < 10){
            lcd.print("0");
            lcd.print(hours);
          }
          else
            lcd.print(hours);
          lcd.print(" : ");
          if(minutes < 10){
            lcd.print("0");
            lcd.print(minutes);
          }
          else
            lcd.print(minutes);
          if(hours <12)
            lcd.print(" AM");
          else
            lcd.print(" PM");
          lcd.print(" **");
          delay(5000);
          sensor = AMBIENT;
          break;   
      }
    }
    // save the last time temp was updated 
    lcdReadingPreviousMillis = currentMillis;
  }   
}

// Get the current system date and time for the timestamp
String getTimeStamp() {
  String result;
  Process time;
  time.begin("date");
  time.addParameter("+%D-%T");  
  time.run(); 

  while(time.available()>0) {
    char c = time.read();
    if(c != '\n')
      result += c;
  }
  return result;
}

// Parse the current time(hours; minutes; seconds)
void getHourMinSec() {
  String result;
  Process time;
  time.begin("date");
  time.addParameter("+%T");  
  time.run(); 
  
   //if there's a result from the time process, parse it
   while (time.available() > 0) {
    // get the result of the time process (should be hh:mm:ss):
    String timeString = time.readString();

    // find the colons:
    int firstColon = timeString.indexOf(":");
    int secondColon = timeString.lastIndexOf(":");

    // get the substrings for hour, minute second:
    String hourString = timeString.substring(0, firstColon);
    String minString = timeString.substring(firstColon + 1, secondColon);
    String secString = timeString.substring(secondColon + 1);

    // convert to ints,saving the previous second:
    hours = hourString.toInt();
    minutes = minString.toInt();
    seconds = secString.toInt();
  }  
  if(hours >= 6 && hours <= 21)
    adlawan = true;
  else
    adlawan = false;
}

// ----------------------------------------------------------------
// Reading temperature or humidity takes about 250 milliseconds!
// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
void DHT11Sensor(){  
  DHT11.acquire();
  while (DHT11.acquiring())
    ;
  int result = DHT11.getStatus();
  switch (result)
  {
    case IDDHTLIB_OK: 
      //Serial.println("OK");       
      alert = false;
      atc = DHT11.getCelsius();        
      rh = DHT11.getHumidity();
      break;
    default: 
      alert = true;
      break;
  }
}


float getWTemp(){
 //returns the temperature from DS18S20 in DEG Celsius

 byte data[12];
 byte addr[8];

 if ( !ds.search(addr)) {
   //no more sensors on chain, reset search
   ds.reset_search();
 }
 
 ds.reset();
 ds.select(addr);
 ds.write(0x44,1);               // start conversion, with parasite power on at the end

 byte present = ds.reset();
 ds.select(addr);  
 ds.write(0xBE);                 // Read Scratchpad

 
 for (int i = 0; i < 9; i++) {   // we need 9 bytes
  data[i] = ds.read();
 }
 
 ds.reset_search();
 
 byte MSB = data[1];
 byte LSB = data[0];

 float tempRead = ((MSB << 8) | LSB); 		//using two's compliment
 float temperatureSum = tempRead / 16;
 
 if((temperatureSum < 0) || (temperatureSum > 50))
   temperatureSum = wtc;
 
 return temperatureSum; 
}


// Get water sensor data
// For Atlas Scientific pH and DO EZO and probes
//
void getWaterSensorData(int ezoAddress){
    Wire.beginTransmission(ezoAddress); 	//call the circuit by its ID number.  
    Wire.write('r');                 		//transmit the command that was sent through the serial port.
    Wire.endTransmission();          		//end the I2C data transmission. 
    
    
    delay(1400);                    		//wait the correct amount of time for the circuit to complete its instruction. 
    
    Wire.requestFrom(ezoAddress,20,1); 		//call the circuit and request 20 bytes (this may be more then we need).
    code=Wire.read();               		//the first byte is the response code, we read this separately.  
    
    switch (code){                  		//switch case based on what the response code is.  
      case 1:                       		//decimal 1.  
        getSensorDataSuccess = true;
        Serial.println("Success");  		//means the command was successful.
        while(Wire.available()){            //are there bytes to receive.  
           in_char = Wire.read();           //receive a byte.
           sensor_data[i]= in_char;         //load this byte into our array.
           i+=1;                            //incur the counter for the array element. 
            if(in_char==0){                 //if we see that we have been sent a null command. 
                i=0;                        //reset the counter i to 0.
                Wire.endTransmission();     //end the I2C data transmission.
                break;                      //exit the while loop.
            }
        }
        ezo_float=atof(sensor_data);
        break;                        		//exits the switch case.
    
      case 2:                        		//decimal 2. 
        getSensorDataSuccess = false;
        Serial.println("Failed");    		//means the command has failed.
        break;                         		//exits the switch case.
    
      case 254:                      		//decimal 254.
        getSensorDataSuccess = false;
        Serial.println("Pending");   		//means the command has not yet been finished calculating.
        break;                         		//exits the switch case.
     
      case 255:                      		//decimal 255.
        getSensorDataSuccess = false;
        Serial.println("No Data");   		//means there is no further data to send.
        break;                       		//exits the switch case.
    }     
}
