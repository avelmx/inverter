#include <Wire.h> 
#include <Adafruit_ADS1X15.h>
#include <Adafruit_MCP23X08.h>
#include <Adafruit_MCP23X17.h>
#include <math.h> // for sin() function
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h> 
#include <Update.h>
#include <WebSocketsServer_Generic.h>
#include <ESPAsyncWebSrv.h>
#include <Ticker.h>

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define USE_MUTEX

// Define LED channel connected to the inverter circuit
#define PWM_CHANNEL_A 1  // Channel for High-Side MOSFET AC
#define PWM_CHANNEL_B 2  // Channel for Low-Side MOSFET AC
#define PWM_CHANNEL_C 0  // Channel for BUCK
#define PWM_CHANNEL_D 3  // Channel for CoolingFan

// Define timer configuration for 100kHz PWM
#define TIMER_SOURCE_CLK TIMER_CLK_APB
#define TIMER_DIVIDER  80  // Adjust to achieve desired frequency (calculate based on clock source)

// Define constants (adjust based on your setup)
#define PWM_FREQUENCY 100000  // 100 kHz carrier frequency
#define SAMPLING_FREQUENCY 1000000  // 1 MHz sampling frequency
#define ADC_RESOLUTION 10  // Adjust if using a different ADC resolution
#define DEAD_TIME_CYCLES 0.15  // Adjust based on IRF3205 datasheet


#define SDA_PIN 41 // GPIO 41
#define SCL_PIN 40 // GPIO 40

//mcp23017 io pins
#define GPIO16 8         //inverter ir2104- 2ho, 2lo
#define GPIO17 9         //inverter ir2104- 1ho, 1lo
#define GPIO27 13         //buck flow back control
#define GPIO32 14         //buck ir2104
#define LOGSEL12_24_1 12  //12 or 24v battery
#define LOGSEL12_24_2 11  //12 or 24v battery
#define LOGSEL12_24_3 10  //12 or 24v battery
#define GPIO34 0          //ADS1015 at 0x49 RDY
#define GPIO36 1          //ADS1015 at 0x48 RDY
#define GPIO37 3          //ADS1015 at 0x4A RDY
#define AC_2_LOG 4        //AC LIVE
#define AC_1_LOG 5        //AC RETURN
#define SOL_LOG 6         //Solar panel enable
#define BAT_LOG 7         //Battery to inverter circuit
#define IN1_PIN  10  // Replace with actual pin assignment (IO10)
#define IN2_PIN  11  // Replace with actual pin assignment (IO11)
#define buck_IN  12  // Replace with actual pin assignment (IO11)
#define fanPin  13  // Replace with actual pin assignment (IO11)

//ADC objects
Adafruit_ADS1015 ads1; // Initialize with address 0x48
Adafruit_ADS1015 ads2; // Initialize with address 0x49
Adafruit_ADS1015 ads3; // Initialize with address 0x4A

//MCP23017 object
Adafruit_MCP23X17 mcp;


char 
ssid[] = "avel_home",                   //   USER PARAMETER - Enter Your WiFi SSID
pass[] = "zxcvbnmmmm",               //   USER PARAMETER - Enter Your WiFi Password
softApssid[] = "esp32";

const char* firmwareFileName = "/firmware.bin";
const char* hostname = "esp32-ota";
const char* username = "avel"; // Set your username
const char* passwordHash = "md5"; // Set your password hash (e.g., using md5)

bool loggedIn = false;

//====================================== USER PARAMETERS ==========================================//
// The parameters below are the default parameters used when the MPPT charger settings have not    //
// been set or saved through the LCD menu interface or mobile phone WiFi app. Some parameters here //
// would allow you to override or unlock features for advanced users (settings not on the LCD menu)//
//=================================================================================================//
bool                                  
MPPT_Mode               = 1,           //   USER PARAMETER - Enable MPPT algorithm, when disabled charger uses CC-CV algorithm 
enableINVERTER          = 0,           //   USER PARAMETER - Enable INVERTER
output_Mode             = 1,           //   USER PARAMETER - 0 = PSU MODE, 1 = Charger Mode  
disableFlashAutoLoad    = 1,           //   USER PARAMETER - Forces the MPPT to not use flash saved settings, enabling this "1" defaults to programmed firmware settings.
enablePPWM              = 1,           //   USER PARAMETER - Enables Predictive PWM, this accelerates regulation speed (only applicable for battery charging application)
enableWiFi              = 1,           //   USER PARAMETER - Enable WiFi Connection
enableFan               = 1,           //   USER PARAMETER - Enable Cooling Fan
enableBluetooth         = 1,           //   USER PARAMETER - Enable Bluetooth Connection
enableLCD               = 0,           //   USER PARAMETER - Enable LCD display
enableLCDBacklight      = 0,           //   USER PARAMETER - Enable LCD display's backlight
overrideFan             = 0,           //   USER PARAMETER - Fan always on
enableActiveControl     = 0,
enableDynamicCooling    = 0;           //   USER PARAMETER - Enable for PWM cooling control 
int
serialTelemMode         = 1,           //  USER PARAMETER - Selects serial telemetry data feed (0 - Disable Serial, 1 - Display All Data, 2 - Display Essential, 3 - Number only)
pwmResolution           = 8,          //  USER PARAMETER - PWM Bit Resolution 
pwmFrequency            = 39000,       //  USER PARAMETER - PWM Switching Frequency - Hz (For Buck)
fanPwm                  = 0,
fanpwmFrequency         = 1000,
fanpwmResolution        = 8,
temperatureFan          = 60,          //  USER PARAMETER - Temperature threshold for fan to turn on
temperatureMax          = 90,          //  USER PARAMETER - Overtemperature, System Shudown When Exceeded (deg C)
telemCounterReset       = 0,           //  USER PARAMETER - Reset Telem Data Every (0 = Never, 1 = Day, 2 = Week, 3 = Month, 4 = Year) 
errorTimeLimit          = 1000,        //  USER PARAMETER - Time interval for reseting error counter (milliseconds)  
errorCountLimit         = 5,           //  USER PARAMETER - Maximum number of errors  
millisRoutineInterval   = 250,         //  USER PARAMETER - Time Interval Refresh Rate For Routine Functions (ms)
millisSerialInterval    = 1,           //  USER PARAMETER - Time Interval Refresh Rate For USB Serial Datafeed (ms)
millisLCDInterval       = 1000,        //  USER PARAMETER - Time Interval Refresh Rate For LCD Display (ms)
millisWiFiInterval      = 2000,        //  USER PARAMETER - Time Interval Refresh Rate For WiFi Telemetry (ms)
millisLCDBackLInterval  = 2000,        //  USER PARAMETER - Time Interval Refresh Rate For WiFi Telemetry (ms)
backlightSleepMode      = 0,           //  USER PARAMETER - 0 = Never, 1 = 10secs, 2 = 5mins, 3 = 1hr, 4 = 6 hrs, 5 = 12hrs, 6 = 1 day, 7 = 3 days, 8 = 1wk, 9 = 1month
baudRate                = 115200;      //  USER PARAMETER - USB Serial Baud Rate (bps)

float 
voltageBatteryMax       = 12.5000,     //   USER PARAMETER - Maximum Battery Charging Voltage (Output V)
voltageBatteryMin       = 10.200,     //   USER PARAMETER - Minimum Battery Charging Voltage (Output V)
currentCharging         = 12.0000,     //   USER PARAMETER - Maximum Charging Current (A - Output)
ki                      = 0.0,
kp                      = 0.1,
kd                      = 0.0,
scaledfactor             = 1.0,
electricalPrice         = 9.5000;      //   USER PARAMETER - Input electrical price per kWh (Dollar/kWh,Euro/kWh,Peso/kWh)


//================================== CALIBRATION PARAMETERS =======================================//
// The parameters below can be tweaked for designing your own MPPT charge controllers. Only modify //
// the values below if you know what you are doing. The values below have been pre-calibrated for  //
// MPPT charge controllers designed by TechBuilder (Angelo S. Casimiro)                            //
//=================================================================================================//
bool
ADS1015_Mode            = 1;          //  CALIB PARAMETER - Use 1 for ADS1015 ADC model use 0 for ADS1115 ADC model
int
ADC_GainSelect          = 0,          //  CALIB PARAMETER - ADC Gain Selection (0→±6.144V 3mV/bit, 1→±4.096V 2mV/bit, 2→±2.048V 1mV/bit)
avgCountVS              = 3,          //  CALIB PARAMETER - Voltage Sensor Average Sampling Count (Recommended: 3)
avgCountCS              = 4,          //  CALIB PARAMETER - Current Sensor Average Sampling Count (Recommended: 4)
avgCountTS              = 500;        //  CALIB PARAMETER - Temperature Sensor Average Sampling Count
float
inVoltageDivRatio       = 8.308,    //  CALIB PARAMETER - Input voltage divider sensor ratio (change this value to calibrate voltage sensor)
outVoltageDivRatio      = 8.308,    //  CALIB PARAMETER - Output voltage divider sensor ratio (change this value to calibrate voltage sensor)
acOutVoltageDivRatio    = 1.000,
vOutSystemMax           = 30.0000,    //  CALIB PARAMETER - 
cOutSystemMax           = 50.0000,    //  CALIB PARAMETER - 
ntcResistance           = 100000.00,   //  CALIB PARAMETER - NTC temp sensor's resistance. Change to 10000.00 if you are using a 10k NTC
voltageDropout          = 1.0000,     //  CALIB PARAMETER - Buck regulator's dropout voltage (DOV is present due to Max Duty Cycle Limit)
voltageBatteryThresh    = 7.5000,     //  CALIB PARAMETER - Power cuts-off when this voltage is reached (Output V)
currentInAbsolute       = 25.0000,    //  CALIB PARAMETER - Maximum Input Current The System Can Handle (A - Input)
currentOutAbsolute      = 25.0000,    //  CALIB PARAMETER - Maximum Output Current The System Can Handle (A - Input)
PPWM_margin             = 90.000,    //  CALIB PARAMETER - Minimum Operating Duty Cycle for Predictive PWM (%)
PWM_MaxDC               = 97.0000,    //  CALIB PARAMETER - Maximum Operating Duty Cycle (%) 90%-97% is good
efficiencyRate          = 1.0000,     //  CALIB PARAMETER - Theroretical Buck Efficiency (% decimal)
currentMidPoint         = 2.495,     //  CALIB PARAMETER - Current Sensor Midpoint (V)
currentSens             = 0.0000,     //  CALIB PARAMETER - Current Sensor Sensitivity (V/A)
currentSensV            = 0.0660,     //  CALIB PARAMETER - Current Sensor Sensitivity (mV/A)
vInSystemMin            = 10.000,     //  CALIB PARAMETER - 
acMinRmsVoltage         = 210,
acMaxRmsVoltage         = 240,
targetAcRMSVoltage      = 220,
maxAcrmsCurrent         = 6,
maxPowerVin             =18.000;      //input voltage at which solar panel power maxes out

//===================================== SYSTEM PARAMETERS =========================================//
// Do not change parameter values in this section. The values below are variables used by system   //
// processes. Changing the values can damage the MPPT hardware. Kindly leave it as is! However,    //
// you can access these variables to acquire data needed for your mods.                            //
//=================================================================================================//
bool
inverterEnable        = 0,
trigprotectionTask    = 0,
ACinitialize          = 1,           
enable12Vbus          = 1,
enable24Vbus          = 0,
enableACload          = 0,
buckEnable            = 0,           // SYSTEM PARAMETER - Buck Enable Status
fanStatus             = 0,           // SYSTEM PARAMETER - Fan activity status (1 = On, 0 = Off)
bypassEnable          = 0,           // SYSTEM PARAMETER - 
chargingPause         = 0,           // SYSTEM PARAMETER - 
lowPowerMode          = 0,           // SYSTEM PARAMETER - 
buttonRightStatus     = 0,           // SYSTEM PARAMETER -
buttonLeftStatus      = 0,           // SYSTEM PARAMETER - 
buttonBackStatus      = 0,           // SYSTEM PARAMETER - 
buttonSelectStatus    = 0,           // SYSTEM PARAMETER -
buttonRightCommand    = 0,           // SYSTEM PARAMETER - 
buttonLeftCommand     = 0,           // SYSTEM PARAMETER - 
buttonBackCommand     = 0,           // SYSTEM PARAMETER - 
buttonSelectCommand   = 0,           // SYSTEM PARAMETER -
settingMode           = 0,           // SYSTEM PARAMETER -
setMenuPage           = 0,           // SYSTEM PARAMETER -
boolTemp              = 0,           // SYSTEM PARAMETER -
flashMemLoad          = 0,           // SYSTEM PARAMETER -  
confirmationMenu      = 0,           // SYSTEM PARAMETER -      
WIFI                  = 0,           // SYSTEM PARAMETER - 
BNC                   = 0,           // SYSTEM PARAMETER -  
REC                   = 0,           // SYSTEM PARAMETER - 
FLV                   = 0,           // SYSTEM PARAMETER - 
IUV                   = 0,           // SYSTEM PARAMETER - 
IOV                   = 0,           // SYSTEM PARAMETER - 
IOC                   = 0,           // SYSTEM PARAMETER - 
OUV                   = 0,           // SYSTEM PARAMETER - 
OOV                   = 0,           // SYSTEM PARAMETER - 
OOC                   = 0,           // SYSTEM PARAMETER - 
OTE                   = 0,           // SYSTEM PARAMETER - 
ACOUV                 = 0,            //Ac out under voltage
ACOOV                 = 0,            //AC over voltage
ACOUI                 = 0,            //Ac out over current
ACOUF                 = 0,            //Ac out under frequency
ACOOF                 = 0,            //Ac out OVER frequency
SEARCH                = 0;
int
inputSource           = 0,           // SYSTEM PARAMETER - 0 = MPPT has no power source, 1 = MPPT is using solar as source, 2 = MPPTis using battery as power source
avgStoreTS            = 0,           // SYSTEM PARAMETER - Temperature Sensor uses non invasive averaging, this is used an accumulator for mean averaging
temperature1          = 0,           // SYSTEM PARAMETER -
temperature2          = 0,           // SYSTEM PARAMETER -
sampleStoreTS         = 0,           // SYSTEM PARAMETER - TS AVG nth Sample
pwmMax                = 0,           // SYSTEM PARAMETER -
pwmMaxLimited         = 0,           // SYSTEM PARAMETER -
PWM                   = 0,           // SYSTEM PARAMETER -
PPWM                  = 0,           // SYSTEM PARAMETER -
pwmChannel            = 0,           // SYSTEM PARAMETER -
batteryPercent        = 0,           // SYSTEM PARAMETER -
errorCount            = 0,           // SYSTEM PARAMETER -
menuPage              = 0,           // SYSTEM PARAMETER -
subMenuPage           = 0,           // SYSTEM PARAMETER -
ERR                   = 0,           // SYSTEM PARAMETER - 
conv1                 = 0,           // SYSTEM PARAMETER -
conv2                 = 0,           // SYSTEM PARAMETER -
noacsampledata        = 2500,
acfrequency           = 50,
fanPWM                = 0,
intTemp               = 0;           // SYSTEM PARAMETER -
float
VSI                   = 0.0000,      // SYSTEM PARAMETER - Raw input voltage sensor ADC voltage
VSO                   = 0.0000,      // SYSTEM PARAMETER - Raw output voltage sensor ADC voltage
CSI                   = 0.0000,      // SYSTEM PARAMETER - Raw current sensor ADC voltage
CSI_converted         = 0.0000,      // SYSTEM PARAMETER - Actual current sensor ADC voltage 
TS1                   = 0.0000,      // SYSTEM PARAMETER - Raw temperature sensor ADC value
TS2                   = 0.0000,      // SYSTEM PARAMETER - Raw temperature sensor ADC value
ACVSO                 = 0.0000,
ACCSO                 = 0.0000,
VS24V                 = 0.0000,
IS12V                 = 0.0000,
IS24V                 = 0.0000,
voltage12bus          = 0.0000,
voltage24bus          = 0.0000,
powerOutput12         = 0.0000,
powerOutput24         = 0.0000,
voltageOutputAc       = 0.0000,
voltageOutputAcRms    = 0.0000,
powerInput            = 0.0000,      // SYSTEM PARAMETER - Input power (solar power) in Watts
powerInputPrev        = 0.0000,      // SYSTEM PARAMETER - Previously stored input power variable for MPPT algorithm (Watts)
powerOutput           = 0.0000,      // SYSTEM PARAMETER - Output power (battery or charing power in Watts)
energySavings         = 0.0000,      // SYSTEM PARAMETER - Energy savings in fiat currency (Peso, USD, Euros or etc...)
voltageInput          = 0.0000,      // SYSTEM PARAMETER - Input voltage (solar voltage)
voltageInputPrev      = 0.0000,      // SYSTEM PARAMETER - Previously stored input voltage variable for MPPT algorithm
voltageOutput         = 0.0000,      // SYSTEM PARAMETER - Input voltage (battery voltage)
currentInput          = 0.0000,      // SYSTEM PARAMETER - Output power (battery or charing voltage)
currentOutput         = 0.0000,      // SYSTEM PARAMETER - Output current (battery or charing current in Amperes)
Rntc1                 = 0.0000,      // SYSTEM PARAMETER - Part of NTC thermistor thermal sensing code
Rntc2                 = 0.0000,      // SYSTEM PARAMETER - Part of NTC thermistor thermal sensing code
ADC_BitReso           = 0.0000,      // SYSTEM PARAMETER - System detects the approriate bit resolution factor for ADS1015/ADS1115 ADC
daysRunning           = 0.0000,      // SYSTEM PARAMETER - Stores the total number of days the MPPT device has been running since last powered
Wh                    = 0.0000,      // SYSTEM PARAMETER - Stores the accumulated energy harvested (Watt-Hours)
kWh                   = 0.0000,      // SYSTEM PARAMETER - Stores the accumulated energy harvested (Kiliowatt-Hours)
MWh                   = 0.0000,      // SYSTEM PARAMETER - Stores the accumulated energy harvested (Megawatt-Hours)
loopTime              = 0.0000,      // SYSTEM PARAMETER -
outputDeviation       = 0.0000,      // SYSTEM PARAMETER - Output Voltage Deviation (%)
buckEfficiency        = 0.0000,      // SYSTEM PARAMETER - Measure buck converter power conversion efficiency (only applicable to my dual current sensor version)
floatTemp             = 0.0000,
floatTemp2             = 0.0000,
feedbackfrequency     = 0.0000,
acrmsvoltage          = 0.0000,
acrmscurrent          = 0.0000,
prevtrigtemp1         = 0.0000,
prevtrigtemp2         = 0.0000,
prevtrigvoltOutput    = 0.0000,
prevtrigvolt24bus     = 0.0000,
prevtrigvoltInput     = 0.0000,
prevTrigbatteryPercent= 0.0000,
vOutSystemMin         = 0.0000;     //  CALIB PARAMETER - 
unsigned long 
currentErrorMillis    = 0,           //SYSTEM PARAMETER -
currentButtonMillis   = 0,           //SYSTEM PARAMETER -
currentSerialMillis   = 0,           //SYSTEM PARAMETER -
currentRoutineMillis  = 0,           //SYSTEM PARAMETER -
currentLCDMillis      = 0,           //SYSTEM PARAMETER - 
currentLCDBackLMillis = 0,           //SYSTEM PARAMETER - 
currentWiFiMillis     = 0,           //SYSTEM PARAMETER - 
currentMenuSetMillis  = 0,           //SYSTEM PARAMETER - 
prevButtonMillis      = 0,           //SYSTEM PARAMETER -
prevSerialMillis      = 0,           //SYSTEM PARAMETER -
prevRoutineMillis     = 0,           //SYSTEM PARAMETER -
prevErrorMillis       = 0,           //SYSTEM PARAMETER -
prevWiFiMillis        = 0,           //SYSTEM PARAMETER -
prevLCDMillis         = 0,           //SYSTEM PARAMETER -
prevLCDBackLMillis    = 0,           //SYSTEM PARAMETER -
timeOn                = 0,           //SYSTEM PARAMETER -
loopTimeStart         = 0,           //SYSTEM PARAMETER - Used for the loop cycle stop watch, records the loop start time
initializingTime       = 0,
loopTimeEnd           = 0,           //SYSTEM PARAMETER - Used for the loop cycle stop watch, records the loop end time
secondsElapsed        = 0;           //SYSTEM PARAMETER - 0

//TOLERANCE VALUES 
float
voltagetolerance      = 0.1,
currenttolerance      = 0.1,
temperaturetolereance = 0.1,
batterytolerance      = 1;

String SytemPrint;


// Define data struct for ac voltage and current
 typedef struct {
    float acvoltage;
    float accurrent;
    unsigned long acsamplingTime;
} acsampleData_t;

acsampleData_t* acdataArray = new acsampleData_t[noacsampledata];

typedef struct {
    float ac_voltage_errorCorrection;
    float associated_period_ratio;
} errorData_t;

errorData_t* errordataArray = new errorData_t[noacsampledata/acfrequency];

typedef struct {
    float ac_voltage;
    float associated_period_ratio;
} acPrintData_t;

acPrintData_t* acData = new acPrintData_t[noacsampledata/acfrequency];



// Task handle
TaskHandle_t protection_task_handle;
TaskHandle_t sensor_acquisition_task_handle;
TaskHandle_t inverter_task_handle;

// Mutex for shared resources
SemaphoreHandle_t shared_lut_mutex;
QueueHandle_t protection_queue; // Declare queue handle globally

Ticker updateTimer;

AsyncWebServer server(80);
WebSocketsServer webSocket(81);


const int baudRat = 115200;  // Adjust baud rate as needed
const char* encoding = "utf-8";

void setup() {
  Serial.begin(baudRat); //Our lovely cereal
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("niko hapa");
  ADC_SetGain();
  // Initialize ADS1 with the default address 0x48
  if (!ads1.begin()) {
    SytemPrint = "System:Failed to initialize ADS1,";
  }

  // Initialize ADS2 with address 0x49
  if (!ads2.begin(0x49)) {
   SytemPrint += "System:Failed to initialize ADS2,";
  }

  // Initialize ADS3 with address 0x4A
  if (!ads3.begin(0x4A)) {
    SytemPrint += "System:Failed to initialize ADS3,";
  }

  //MCP23017 INITIALIZATION
  if (!mcp.begin_I2C()) {
    SytemPrint += "System:Error nitializing MCP23017,";
    while (1);
  }

  WiFi.begin(ssid, pass);
  delay(1000);
  //pin modes
  //Inputs
  mcp.pinMode(GPIO34, INPUT);  //ADS1015 at 0x49 RDY
  mcp.pinMode(GPIO36, INPUT);  //ADS1015 at 0x48 RDY
  mcp.pinMode(GPIO37, INPUT);  //ADS1015 at 0x4A RDY

  //Outputs
  mcp.pinMode(LOGSEL12_24_1, OUTPUT);
  mcp.pinMode(LOGSEL12_24_2, OUTPUT);
  mcp.pinMode(LOGSEL12_24_3, OUTPUT);
  mcp.pinMode(GPIO16, OUTPUT);
  mcp.pinMode(GPIO17, OUTPUT);
  mcp.pinMode(GPIO27, OUTPUT);
  mcp.pinMode(GPIO32, OUTPUT);
  mcp.pinMode(AC_1_LOG, OUTPUT);
  mcp.pinMode(AC_2_LOG, OUTPUT);
  mcp.pinMode(BAT_LOG, OUTPUT);
  mcp.pinMode(SOL_LOG, OUTPUT);
 

  // Create the mutex for shared resources
  #ifdef USE_MUTEX
    shared_lut_mutex = xSemaphoreCreateMutex();
  #endif
  
protection_queue = xQueueCreate(1, sizeof(uint8_t)); // Create queue with size 1 and data size
if (protection_queue == NULL) {
  // Handle queue creation error
  SytemPrint += "System:Error initializing protection trigger queue. Device might blow up,";
  while (1);
}


if (WiFi.status() == WL_CONNECTED) 
  { 
    SytemPrint += "System:ssid-" + String(ssid);
    SytemPrint += "-IP address-";
    SytemPrint += String(WiFi.localIP()) + ","; 
  }
  else
  {
    // You can remove the password parameter if you want the AP to be open.
    // a valid password must have more than 7 characters
    if (!WiFi.softAP(softApssid, pass)) {
        log_e("Soft AP creation failed.");
        while(1);
      }
      IPAddress myIP = WiFi.softAPIP();
      SytemPrint += "System:AP IP address-";
      SytemPrint += String(myIP) + ",";
       delay(2000);
  }

  mDnS();

   // Create the tasks
  xTaskCreatePinnedToCore(
    protection_task
    , "protection_task"
    , 2048
    , NULL
    , 7
    , &protection_task_handle
    , ARDUINO_RUNNING_CORE 
  );

  xTaskCreatePinnedToCore(
    sensor_acquisition_task
    , "sensor_acquisition_task"
    , 2048
    , NULL
    , 6
    , &sensor_acquisition_task_handle
    , 0 
  );

  xTaskCreate(
    buck_task
    , "buck_task"
    , 32768
    , NULL
    , 4
    , NULL
  );
/*
  xTaskCreate(
    system_task
    , "system_task"
    , 32768
    , NULL
    , 1
    , NULL
  );



  xTaskCreatePinnedToCore(
    inverter_task
    , "inverter_task"
    , 2048
    , NULL
    , 5
    , &inverter_task_handle
    , ARDUINO_RUNNING_CORE 
  );

    xTaskCreate(
    telemetry_task
    , "telemetry_task"
    , 655360
    , NULL
    , 3
    , NULL
  );
*/
  xTaskCreate(
    ac_feedback_task
    , "ac_feedback_task"
    , 8192
    , NULL
    , 3
    , NULL
  );



Serial.println(SytemPrint);

}

void loop() {
    Serial.println("niko hapa");
vTaskDelay(1000);
}





