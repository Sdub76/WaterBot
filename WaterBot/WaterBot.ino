// WaterBot
// Christmas Tree Watering Bot
//  

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/////////////////////////
// Macro Definition
/////////////////////////
// Printing with stream operator
template<class T> inline Print& operator <<(Print &obj,     T arg) { obj.print(arg);    return obj; }
template<>        inline Print& operator <<(Print &obj, float arg) { obj.print(arg, 4); return obj; }

/////////////////////////
// HARDWARE DEFINITION
/////////////////////////
// Serial Interface Pins
#define UART1_TX          1   // SER - Serial Monitor TX
#define UART1_RX          3   // SER - Serial Monitor RX
#define I2C_SDA           4   // I2C - SDA OLED Display
#define I2C_SCL           5   // I2C - SCL OLED Display
// Sensors
//#define SENSOR_EXCITATION 15  // DOUT - Level Sensor Excitation / Red LED  
#define SUPPLY_LEVEL      13  // DIN  - Supply Level (Binary Low = Wet)
#define TREE_LEVEL        14  // DIN  - Tree Level (Binary Low = Wet)
#define TREE_LEVEL_A      A0  // AIN  - Tree Level (Analog) 0-3.3V through 390/1M Voltage Divider = 0.00-0.93V
// Switched Voltage Outputs
#define STATUS_LED        16  // DOUT - Tree LED (switched 12V) Low = ON 
#define PUMP              12  // DOUT - Pump Power (switched 12V) Low = ON
// SPECIAL FUNCTION PINS
#define GPIO0             0   // BOOTLOADER - BOOT SKETCH - Red LED (Don't use for other purpose)
#define GPIO2             2   // BOOTLOADER - UART BOOTLOADER - Blue LED (Don't use for other purpose)
#define GPIO15            15  // BOOTLOADER - SDIO MODE

//VARIABLE DEFINITION
unsigned long LoopTimer = 0;  // Timer variable for major frames
int LoopDelay = 5000;         // Delay between major frames (ms)

unsigned long BlinkTimer = 0; // Timer variable for LED Blink
int LED_State  = 0;           // State variable for LED
int BlinkDelay = 0;           // Delay between LED state changes (ms)
#define LED_OFF           0   // Steady OFF
#define LED_ON           -1   // Steady ON
#define LED_SLOW        500   // Time between slow LED toggles
#define LED_FAST         50   // Time between fast LED toggles

#define RELAY_OFF         1   // SVO Relay is active LOW
#define RELAY_ON          0   // SVO Relay is active LOW

unsigned long TimePrev = 0;   // State variable for pump timer
unsigned long TimeCurr = 0;   // Current Time for pump timer
bool NewPumpSession = true;   // Flag to indicate transition to new pumping session
int PumpTime = 0;             // Accumlated watering time this pumping session (ms)
int PumpTimeout = 10*1000;    // Maximum allowable pump run time (ms)
float WaterTime = 0.0;        // Accumulated total watering time (s)
int Pump_State = 0;           // State variable for Pump

int SupplyState = 0;          // Digital Supply level sensor value
int TreeState_raw = 0;        // Digital Tree level sesor value        
int TreeLevel_raw = 0;        // Analog Tree level sensor value
int TreeState = -1;           // Post-processed tree level state
float TreeLevel = 0.0;        // Post-processed tree level value (0-100%)

#define SENSOR_WET         0  // Water level sensor is low = wet
#define SENSOR_DRY         1  // Water level sensor is high = dry
#define SENSOR_ERROR      -1  // Water level sensor is invalid

#define SENS_RAW_EMPTY   725  // Based on manual calibration (raw ADC counts)
#define SENS_RAW_FULL    320  // Based on manual calibration (raw ADC counts)
#define SENS_RAW_ERR     900  // Based on manual calibration (raw ADC counts)

int State = 0;                // Main Control State Variable
#define STATE_SLEEP      0    // Sleep
#define STATE_RUN        1    // Running/Filling
#define STATE_LOWSUPPLY  2    // Supply reservoir empty
#define STATE_SENSORERR  3    // Sensor out of range
#define STATE_FAILSAFE   4    // Failsafe state if pump runs too long

/////////////////////////
//STRUCT Initialization
/////////////////////////
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);



void setup() {

  // Initialize Outputs
  pinMode(PUMP, OUTPUT);
  digitalWrite(PUMP,RELAY_OFF);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED,RELAY_OFF);
//  pinMode(SENSOR_EXCITATION, OUTPUT);

  // Initialize Inputs
  pinMode(SUPPLY_LEVEL, INPUT);
  pinMode(TREE_LEVEL, INPUT);
  pinMode(TREE_LEVEL_A, INPUT);


  // Begin Serial Debug Interface
  Serial.begin(9600);
  Serial.println("WaterBot Serial Monitor");

  // Begin OLED Display Interface
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  // Address 0x3C for 128x32
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display(); // Display splashscreen
  delay(1000);
  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);

  // Initalize Variables
  State = STATE_RUN;       // Initialize Control State to Running
  Pump_State = LOW;        // Initialize Pump to OFF
  LED_State = RELAY_OFF;         // Initialize LED to OFF
  LoopTimer = millis();    // Initialize Loop timer
  BlinkTimer = millis();   // Initialize LED timer

}

// TODO Add MQTT Reporting

void loop() {
  ReadSensors();  // Only for debug

  Pump_State = RELAY_OFF;  // Command pump off just to be sure

  // Control State Machine
  switch(State) {
    // STATE_SLEEP
    //    LED = Off
    //    Transition to STATE_RUN if Loop timer complete 
    case STATE_SLEEP :
      BlinkDelay = LED_OFF;
      if ((LoopTimer + LoopDelay) < millis()) {
        State = STATE_RUN;    // Transition to Running
        LoopTimer = millis();     // Reset timer
      }
      break;

    // STATE_RUN
    //    LED = On
    //    Read Sensors
    //    Transition to STATE_LOWSUPPLY if Supply sensor is dry
    //    Engage Pump if Tree Sensor is dry 
    //    Transition to STATE_SLEEP if Tree sensor is wet
    //    Transition to STATE_SENSORERR if Tree sensor is out of range 
    case STATE_RUN :

      ReadSensors();

      if (SupplyState == SENSOR_DRY) {
        // Supply is dry - Transition out of run state
        State = STATE_LOWSUPPLY;
        break; 
      }
      switch (TreeState) {

        // Sensor is Dry - Turn on pump and start over
        case SENSOR_DRY :
          
          BlinkDelay = LED_ON; 
          
          Pump_State = RELAY_ON;

          TimeCurr = millis(); // Check the time
          // Accumulate current pumping session time if not the first time through the loop
          if (NewPumpSession) {
            PumpTime = 0;
            NewPumpSession = false;
          } else {
            PumpTime += (TimeCurr - TimePrev);  
          }
          TimePrev = TimeCurr;

          if (PumpTime > PumpTimeout) {
            // We've been pumping too long... stop in case there's a hardware problem
            // - Could be failed tree sensor below fault threshold (spillage risk)
            // - Could be broken, loose, or leaky tube (spillage risk)
            // - Could be supply sensor failure (dry pump overheat risk)
            // - Could be disconnected pump (underwatering risk)
            State = STATE_FAILSAFE;
          }
          break;

        // Sensor is Wet - Go back to sleep for a while
        case SENSOR_WET :

          BlinkDelay = LED_OFF; 

          State = STATE_SLEEP;
          LoopTimer = millis();
          break;

        // Sensor is Broken - Transition out of run state
        case SENSOR_ERROR :
          State = STATE_SENSORERR;
          break;
      }
      break;
      
    // STATE_LOWSUPPLY
    //    LED = Slow Blink
    //    Read Sensors
    //    Transition to STATE_RUN if Supply sensor is wet
    case STATE_LOWSUPPLY :
      BlinkDelay = LED_SLOW; 
      ReadSensors();
      if (SupplyState == SENSOR_WET) {
        // Supply is repleneshed - Transition back to run state
        State = STATE_RUN;
      }
      break;
      
    // STATE_SENSORERR
    //    LED = Fast Blink
    //    Do nothing
    case STATE_SENSORERR :
      BlinkDelay = LED_FAST; 
      // Do Nothing, stay stuck in this state until reset
      break;

    // STATE_FAILSAFE
    //    LED = Fast Blink
    //    Do nothing
    case STATE_FAILSAFE :
      BlinkDelay = LED_FAST; 
      // Do Nothing, stay stuck in this state until reset
      break;
  }

  // Blink LED
  switch (BlinkDelay) {

    case LED_OFF :
      LED_State = RELAY_OFF;
      break;
    
    case LED_ON :
      LED_State = RELAY_ON;
      break;

    default :
      if ((BlinkTimer + BlinkDelay) < millis()) {
        LED_State = (LED_State == RELAY_ON) ?  RELAY_OFF : RELAY_ON;  // Toggle LED state
        BlinkTimer = millis();                                       // Reset timer
      }
  }


  //Write Outputs
  digitalWrite(STATUS_LED,LED_State);
  digitalWrite(PUMP,Pump_State);
  DebugOutputs();

}

void ReadSensors() {
  
  // Only turn on level sensors when we're actually ready to read them
  // This prevents erosion of the moisture sensors
//  digitalWrite(SENSOR_EXCITATION,HIGH);
//  delay(500); // Wait for outputs to stabilize
  
  SupplyState = digitalRead(SUPPLY_LEVEL);
  TreeState_raw = digitalRead(TREE_LEVEL);
  TreeLevel_raw = analogRead(TREE_LEVEL_A);
  
//  digitalWrite(SENSOR_EXCITATION,LOW);

  TreeLevel = map(TreeLevel_raw,SENS_RAW_EMPTY,SENS_RAW_FULL,0,100);  // Mapping of analog values to percentage

  switch (TreeState) {
    case SENSOR_DRY :
      if (TreeState_raw == SENSOR_WET) {
        // If transitioning from "dry" to "wet", add pump on-time to total watering time
        WaterTime = WaterTime + (float)(PumpTime)/1000.0;
        // Set flag to reinitialize pump timer
        NewPumpSession = true;
        TreeState = SENSOR_WET;
      }
      // Invalidate Tree Level State if raw value > threshold
      if (TreeLevel_raw > SENS_RAW_ERR) {
        // If transitioning from "dry" to "error", add pump on-time to total watering time
        WaterTime = WaterTime + (float)PumpTime/1000.0;
        // Set flag to reinitialize pump timer
        NewPumpSession = true;
        TreeState = SENSOR_ERROR;
      }
      break;
    case SENSOR_WET :
      TreeState = SENSOR_WET;
      if (TreeState_raw == SENSOR_DRY) {
        TreeState = SENSOR_DRY;
      }
      // Invalidate Tree Level State if raw value > threshold
      if (TreeLevel_raw > SENS_RAW_ERR) {
        TreeState = SENSOR_ERROR;
      }
      break;
    case SENSOR_ERROR :
      TreeState = SENSOR_WET;
      break;
  }
  
  
}

void DebugOutputs() {

  char Status[128] = ""; // Status message for eventual display/debug
  char SubStr[32] = "";  // Substring for building output status

  //Line 1
  (SupplyState == SENSOR_DRY)? strcat(Status,"Supply = DRY\n"): strcat(Status,"Supply = WET\n");

  //Line 2
  (TreeState == SENSOR_DRY)?   sprintf(SubStr,"Tree = DRY (%3d)\n",(int)TreeLevel_raw): sprintf(SubStr,"Tree = WET (%3d)\n",(int)TreeLevel_raw);
  strcat(Status,SubStr);
  
  //Line 3
  switch(State){
    case STATE_SLEEP :
      strcat(Status,"State = SLEEP\n");
      break;
    case STATE_RUN :
      strcat(Status,"State = RUN\n");
      break;
    case STATE_LOWSUPPLY :
      strcat(Status,"State = LOW SUPPLY\n");
      break;
    case STATE_SENSORERR :
      strcat(Status,"State = SENSOR ERR\n");
      break;
    case STATE_FAILSAFE :
      strcat(Status,"State = FAIL SAFE\n");
      break;
  }
    
  //Line 4
  (Pump_State == RELAY_OFF)? sprintf(SubStr,"Pump = OFF (%3d/%3d)\n",(int)(PumpTime/1000.0),(int)WaterTime): sprintf(SubStr,"Pump = ON (%3d/%3d)\n",(int)(PumpTime/1000.0),(int)WaterTime);
  strcat(Status,SubStr);

  //Line 5
  (LED_State == RELAY_OFF)?  strcat(SubStr,"LED = OFF\n"): strcat(SubStr,"LED = ON\n");
  
  
  // Output Status Messages
  Serial << Status << '\n';
  
  display.clearDisplay();
  display.setCursor(0,0);
  display << Status;
  display.display();


}
