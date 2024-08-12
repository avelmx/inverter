
void inverter_task(void *pvParameters) {
  
  while (true) {
  
    if(enableACload==true && ACinitialize == true){
      initializingTime = millis(); 
      ACinitialize = false;
    }
    if(((initializingTime - runningTime) > 10000) && inverterEnable == true && enableACload == true){takeacload = true; }
    // Calculate duty cycle using LUT function

    vTaskDelay(5);
  }
}

void inverter_Enable(){  
  if(xSemaphoreTake(i2c_bus1, 10) == pdTRUE)
      {                                                                //Enable MPPT Buck Converter
        inverterEnable = 1;
        mcp.digitalWrite(GPIO16,HIGH);
        mcp.digitalWrite(GPIO17,HIGH);
        xSemaphoreGive(i2c_bus1);
      }
    
}

void inverter_Disable(){  
  if(xSemaphoreTake(i2c_bus1, 10) == pdTRUE)
      {                                                            //Disable MPPT Buck Converter
        inverterEnable = 0; 
        mcp.digitalWrite(GPIO16,LOW);
        mcp.digitalWrite(GPIO17,LOW);
        xSemaphoreGive(i2c_bus1);
        enableACload = 0;
      }
} 



float returnerror(float associatedtime){
  if(enableActiveControl == 1)
  {
    float storederror;

      if(associatedtime <= errordataArray[0].associated_period_ratio)
      {
        float firsterrordiff = errordataArray[1].ac_voltage_errorCorrection - errordataArray[0].ac_voltage_errorCorrection;
        float timeratio = (errordataArray[(int)floor(associatedtime * noacsampledata)].associated_period_ratio -associatedtime)/(errordataArray[1].associated_period_ratio - errordataArray[0].associated_period_ratio);
        storederror = errordataArray[0].ac_voltage_errorCorrection - (firsterrordiff * timeratio);
      }
      else if(associatedtime >= errordataArray[noacsampledata - 1].associated_period_ratio){
        float lasterrordiff = errordataArray[noacsampledata - 1].ac_voltage_errorCorrection - errordataArray[noacsampledata - 2].ac_voltage_errorCorrection;
        float timeratio = (errordataArray[(int)floor(associatedtime * noacsampledata)].associated_period_ratio -associatedtime)/(errordataArray[noacsampledata - 1].associated_period_ratio - errordataArray[noacsampledata - 2].associated_period_ratio);
        storederror = errordataArray[noacsampledata - 1].ac_voltage_errorCorrection + (lasterrordiff * timeratio);
      }
      else if(errordataArray[(int)floor(associatedtime * noacsampledata)].associated_period_ratio <= associatedtime && errordataArray[(int)floor(associatedtime * noacsampledata)+1].associated_period_ratio >= associatedtime){
        float firsterrordiff = errordataArray[(int)floor(associatedtime * noacsampledata)].ac_voltage_errorCorrection - errordataArray[(int)floor(associatedtime * noacsampledata) + 1].ac_voltage_errorCorrection;
        float timeratio = (errordataArray[(int)floor(associatedtime * noacsampledata)].associated_period_ratio -associatedtime)/(errordataArray[(int)floor(associatedtime * noacsampledata)].associated_period_ratio - errordataArray[(int)floor(associatedtime * noacsampledata) + 1].associated_period_ratio);
        storederror = errordataArray[(int)floor(associatedtime * noacsampledata)].ac_voltage_errorCorrection + (firsterrordiff * timeratio);
      }

  
    float pidreturn = pidcontrol(storederror, kp, ki, kd);
    return pidreturn;
  } else{
    return 0;
  }
}


// Function to calculate duty cycle
float calculateDutyCycle(unsigned long runningTime, float frequency) {
    // Calculate the period of the sine wave
    unsigned long period = 1.0 / frequency;   
    // Calculate the time for one half of the sine wave (from 0 to peak)
    float halfPeriod = period / 2.0;   
    // Calculate the number of complete cycles in the running time
    int completeCycles = runningTime / period;   
    // Calculate the remaining time after complete cycles
    float remainingTime = runningTime % period;  
    // If there's no remaining time, return 0 (no duty cycle)
    if (remainingTime == 0) {
        return 0.0;
    }

    // Calculate the duty cycle as a percentage
    float period_ratio = remainingTime / period;
    float sine_value = sin(period_ratio * M_PI * 2);
     // Scale sine_value based on ADC resolution
    float scaled_value = sine_value;
    // Offset and scaling for positive/negative voltage control
    int duty_cycle = (int)scaled_value;
    return duty_cycle;
}


float pidcontrol(float error, float kp, float ki, float kd) {
  // Calculate error terms
  static float error_prev = 0.0f;  // Stores previous error for integral term
  float error_integral = error_prev + error; // Simple summation for integral term

  // Update previous error for next iteration
  error_prev = error;

  // Calculate PID output
  float output = kp * error + ki * error_integral + kd * (error - error_prev);

  // Limit output (optional)
  // ... (implement logic to limit output if necessary)

  return output;
}


void populateLUT() {
    for (int i = 0; i < LUT_SIZE; i++) {
        // Calculate duty cycle value for a sine wave in microseconds
        dutyCycleLUT[i] = (uint16_t)((sin(2 * PI * i / LUT_SIZE) + 1) * 5000); // Scale to 0-10000 us
    }
}


void setupTimer() {
    // Set timer frequency to 1Mhz
  timer = timerBegin(1000000);

  // Attach onTimer function to our timer.
  timerAttachInterrupt(timer, &onTimer);

  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter) with unlimited count = 0 (fourth parameter).
  timerAlarm(timer, 100, true, 0);
}

