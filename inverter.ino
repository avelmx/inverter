#include <Wire.h> 
#include <Adafruit_ADS1X15.h>
#include <Adafruit_MCP23X08.h>
#include <Adafruit_MCP23X17.h>

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define USE_MUTEX

#define SDA_PIN 41 // GPIO 41
#define SCL_PIN 40 // GPIO 40

//mcp23017 io pins
#define GPIO16 8     
#define LOGSEL12_24_3 12  

//ADC objects
Adafruit_ADS1015 ads1; // Initialize with address 0x48
Adafruit_ADS1015 ads2; // Initialize with address 0x49
Adafruit_ADS1015 ads3; // Initialize with address 0x4A

//MCP23017 object
Adafruit_MCP23X17 mcp;

// Function prototypes for the tasks
void error_handling_task(void *pvParameters);
void sensor_acquisition_task(void *pvParameters);
void buck_task(void *pvParameters);
void inverter_task(void *pvParameters);
void ac_feedback_task(void *pvParameters);

// Task handle
TaskHandle_t error_handling_task_handle;

// Mutex for shared resources
SemaphoreHandle_t shared_lut_mutex;

void setup() {
  Serial.begin(115200); //Our lovely cereal
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize ADS1 with the default address 0x48
  if (!ads1.begin()) {
    Serial.println("Failed to initialize ADS1.");
  }

  // Initialize ADS2 with address 0x49
  if (!ads2.begin(0x49)) {
    Serial.println("Failed to initialize ADS2.");
  }

  // Initialize ADS3 with address 0x4A
  if (!ads3.begin(0x4A)) {
    Serial.println("Failed to initialize ADS3.");
  }

  //MCP23017 INITIALIZATION
  if (!mcp.begin_I2C()) {
    Serial.println("Error nitializing MCP23017.");
    while (1);
  }
  //pin modes
  //Inputs
  mcp.pinMode(LOGSEL12_24_3, OUTPUT);
  //Outputs
  mcp.pinMode(GPIO16, OUTPUT);
 

  // Create the mutex for shared resources
  #ifdef USE_MUTEX
    shared_lut_mutex = xSemaphoreCreateMutex();
  #endif
  
   // Create the tasks
  xTaskCreatePinnedToCore(
    error_handling_task
    , "error_handling_task"
    , 2048
    , NULL
    , 1
    , &error_handling_task_handle
    , ARDUINO_RUNNING_CORE 
  );

  xTaskCreate(
    sensor_acquisition_task
    , "sensor_acquisition_task"
    , 2048
    , NULL
    , 5
    , NULL
  );

  xTaskCreate(
    buck_task
    , "buck_task"
    , 2048
    , NULL
    , 2
    , NULL
  );

  xTaskCreate(
    inverter_task
    , "inverter_task"
    , 2048
    , NULL
    , 2
    , NULL
  );

  xTaskCreate(
    ac_feedback_task
    , "ac_feedback_task"
    , 2048
    , NULL
    , 2
    , NULL
  );

}

void loop() {
  // put your main code here, to run repeatedly:
 
}





