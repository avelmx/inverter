
void inverter_task(void *pvParameters) {
  
  // Initialize variables
  float sine_value = 0;  // Replace with logic to get sampled sine wave value (ADC reading)
  int duty_cycle = 0;
  wakeprotectionTask();

  // Start LED channel
  ledcSetup(PWM_CHANNEL_A,PWM_FREQUENCY,ADC_RESOLUTION);          //Set PWM Parameters
  ledcSetup(PWM_CHANNEL_B,PWM_FREQUENCY,ADC_RESOLUTION);          //Set PWM Parameters
  ledcAttachPin(IN1_PIN, PWM_CHANNEL_A);  
  ledcAttachPin(IN2_PIN, PWM_CHANNEL_B); 

  while (true) {
    unsigned long runningTime = millis(); // Current running time in milliseconds
    if(enableACload==true && ACinitialize == true){
      initializingTime = millis(); 
      ACinitialize = false;
    }
    if(((initializingTime - runningTime) > 10000) && inverterEnable == true && enableACload == true){takeACload(); }
    // Calculate duty cycle using LUT function
    duty_cycle =  calculateDutyCycle(runningTime, acfrequency);
    if(xSemaphoreTake(shared_lut_mutex, 0) == pdTRUE){
      duty_cycle = duty_cycle + returnerror((runningTime % (1/acfrequency)) / (1/acfrequency));
       xSemaphoreGive(shared_lut_mutex);
    }
    // Apply dead time
    if (duty_cycle >= 0 && inverterEnable) {
       ledcWrite( PWM_CHANNEL_A, duty_cycle);
      vTaskDelay(pdMS_TO_TICKS(DEAD_TIME_CYCLES));
       ledcWrite( PWM_CHANNEL_B, 0);  // Turn off low-side MOSFET
    } else if (duty_cycle < 0 && inverterEnable) {
       ledcWrite( PWM_CHANNEL_B, abs(duty_cycle));  // Use absolute value for low-side
      vTaskDelay(pdMS_TO_TICKS(DEAD_TIME_CYCLES));
       ledcWrite( PWM_CHANNEL_A, 0);  // Turn off high-side MOSFET
    } else {
      // Handle zero output condition (optional)
       ledcWrite( PWM_CHANNEL_A, 0);
       ledcWrite( PWM_CHANNEL_B, 0);
    }

    // Delay for PWM frequency
    int delay_ms = 1 / (PWM_FREQUENCY / 1000); // Delay in milliseconds
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
  }
}

void inverter_Enable(){                                                                  //Enable MPPT Buck Converter
  inverterEnable = 1;
  mcp.digitalWrite(GPIO16,HIGH);
  mcp.digitalWrite(GPIO17,HIGH);
}

void inverter_Disable(){                                                                 //Disable MPPT Buck Converter
  inverterEnable = 0; 
  mcp.digitalWrite(GPIO16,LOW);
  mcp.digitalWrite(GPIO17,LOW);
  enableACload = 0;
} 

void takeACload(){
  mcp.digitalWrite(AC_1_LOG,HIGH);
  mcp.digitalWrite(AC_2_LOG,HIGH);
}

float returnerror(float associatedtime){
  if(enableActiveControl == 1){
    float storederror;
    for(int i = 0; i<(noacsampledata/acfrequency); i++){
      if(associatedtime <= errordataArray[0].associated_period_ratio){
        float firsterrordiff = errordataArray[1].ac_voltage_errorCorrection - errordataArray[0].ac_voltage_errorCorrection;
        float timeratio = associatedtime/(errordataArray[1].associated_period_ratio - errordataArray[0].associated_period_ratio);
        storederror = errordataArray[0].ac_voltage_errorCorrection - (firsterrordiff * timeratio);
      }
      else if(associatedtime >= errordataArray[noacsampledata/acfrequency - 1].associated_period_ratio){
        float lasterrordiff = errordataArray[noacsampledata/acfrequency - 1].ac_voltage_errorCorrection - errordataArray[noacsampledata/acfrequency - 2].ac_voltage_errorCorrection;
        float timeratio = associatedtime/(errordataArray[noacsampledata/acfrequency - 1].associated_period_ratio - errordataArray[noacsampledata/acfrequency - 2].associated_period_ratio);
        storederror = errordataArray[noacsampledata/acfrequency - 1].ac_voltage_errorCorrection + (lasterrordiff * timeratio);
      }
      else if(errordataArray[i].associated_period_ratio <= associatedtime && errordataArray[i+1].associated_period_ratio >= associatedtime){
        float firsterrordiff = errordataArray[i].ac_voltage_errorCorrection - errordataArray[i + 1].ac_voltage_errorCorrection;
        float timeratio = associatedtime/(errordataArray[i].associated_period_ratio - errordataArray[i + 1].associated_period_ratio);
        storederror = errordataArray[i].ac_voltage_errorCorrection + (firsterrordiff * timeratio);
      }
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
    float scaled_value = (float)sine_value * (pow(2, ADC_RESOLUTION) - 1);
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


