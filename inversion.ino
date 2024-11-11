
void inverter_task(void *pvParameters) {
  
  while (true) {
  
    if(enableACload==true && ACinitialize == true){
      initializingTime = millis(); 
      ACinitialize = false;
    }
    if(((initializingTime - runningTime) > 10000) && inverterEnable == true && enableACload == true){takeacload = true; }
    // Calculate duty cycle using LUT function

    vTaskDelay(50);
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
    pos_dutyCycleLUT[i] = 0; 
    neg_dutyCycleLUT[i] = 0; 
    pos_dutyCycleLUT_untouched[i] = 0; 
    neg_dutyCycleLUT_untouched[i] = 0; 
  }

  for (int i = 0; i < (LUT_SIZE/2); i++) {
    // Calculate duty cycle value for a sine wave in microseconds
    pos_dutyCycleLUT[i] = (uint16_t)((sin((PI * i )/ ((LUT_SIZE/2) -1))) * 98); // Scale to 0-10000 us
    pos_dutyCycleLUT_untouched[i] = pos_dutyCycleLUT[i];
    neg_dutyCycleLUT[(LUT_SIZE/2) + i] = (uint16_t)((sin((PI * i )/ ((LUT_SIZE/2) -1))) * 98); // Scale to 0-10000 us
    neg_dutyCycleLUT_untouched[(LUT_SIZE/2) + i] = neg_dutyCycleLUT[(LUT_SIZE/2) + i];
  }
}


void setupTimer() {
    // Set timer frequency to 1Mhz
  timer = timerBegin(1000000);

  // Attach onTimer function to our timer.
  timerAttachInterrupt(timer, &onTimer);

  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter) with unlimited count = 0 (fourth parameter).
  timerAlarm(timer, 1000000/(LUT_SIZE * acfrequency), true, 0);
}

