void ac_feedback_task(void *pvParameters) {
  int counter = 0;
  for (;;){
    counter = 0;
    while(noacsampledata){
      acsampleData_t[counter].acvoltage = ads2.readADC_SingleEnded(0);
      acsampleData_t[counter].accurrent = ads1.readADC_SingleEnded(0);
      acsampleData_t[counter].acsamplingTime =  millis();
      unsigned long theTime = acsampleData_t[counter].acsamplingTime;
      errordataArray[counter % acfrequency].ac_voltage_errorCorrection = calculatePerfectSineVoltageValue(theTime, acfrequency) - acsampleData_t[counter].acvoltage;
      errordataArray[counter % acfrequency].associated_period_ratio = (theTime % (1/acfrequency)) / (1/acfrequency);
      if((counter/(noacsampledata/10)) == 0){
        acrmsvoltage = calculate_rms_voltag(acsamepleData, counter);  //get ac rms voltage
        if(acrmsvoltage < acMinRmsVoltage ){
          ACOUV = 1;
        }
        else if(acrmsvoltage > acMaxRmsVoltage){
          ACOOV = 1;
        }
      }
      counter++;
      vTaskDelay(pdMS_TO_TICKS(1/noacsampledata));
    }
    feedbackfrequency = calculate_frequency(acsamepleData, noacsampledata);
    if(feedbackfrequency < (acfrequency - 3)){
      ACOUF = 1;
    }
    else if(feedbackfrequency > (acfrequency + 3)){
      ACOOF = 1;
    }
  }
}



float calculate_rms_voltage(struct measurement_data *data, int length) {
  float squared_sum = 0.0;
  for (int i = 0; i < length; i++) {
    squared_sum += pow(data[i].voltage, 2);
  }
  return sqrt(squared_sum / length);
}

float calculate_frequency(struct measurement_data *data, int length, float sampling_rate) {
  if (length < 2) {
    return NAN; // Handle case with insufficient data
  }

  int num_peaks = 0;
  float total_time_diff = 0.0;
  float prev_peak_time = data[0].sample_time;

  for (int i = 3; i < length - 4; i++) {
    float peak_voltage = 0.0;
    for (int j = i - 2; j <= i + 2; j++) {
      peak_voltage = fmax(peak_voltage, data[j].voltage);
    }

    float left_avg = (data[i - 1].voltage + data[i - 2].voltage + data[i - 3].voltage)/3; //reduce noise
    float right_avg = (data[i + 1].voltage + data[i + 2].voltage + data[i + 3].voltage)/3; //reduce noise
    if (peak_voltage > left_avg && peak_voltage > right_avg) {
      // Only consider positive peaks after rectification

      if (num_peaks % 2 == 1 && prev_peak_time != data[0].sample_time) {
        total_time_diff += data[i].sample_time - prev_peak_time;
        prev_peak_time = data[i].sample_time;
      }
      num_peaks++;
    }
  }

  if (num_peaks < 1) {
    return NAN; // Handle case with no peaks found
  }

  float average_time_diff = total_time_diff / (num_peaks/2);
  return 1.0 / average_time_diff;
}



float calculatePerfectSineVoltageValue(unsigned long runningTime, float frequency) {
    // Calculate the period of the sine wave
    float period = 1.0 / frequency;     
    // Calculate the number of complete cycles in the running time
    float remainingTime = runningTime % period;  
    // If there's no remaining time, return 0 (no duty cycle)
    if (remainingTime == 0) {
        return 0.0;
    }

    // Calculate the duty cycle as a percentage
    float period_ratio = remainingTime / period;
    float sine_value = sin(period_ratio * M_PI * 2);
     // Scale sine_value based on ADC resolution
    float scaled_voltage = (float)sine_value * targetAcRMSVoltage * 1.414;
    return scaled_voltage;
}


float pid_control(float reference, float measured, float kp, float ki, float kd) {
  // Calculate error terms
  static float error_prev = 0.0f;  // Stores previous error for integral term
  float error = reference - measured;
  float error_integral = error_prev + error; // Simple summation for integral term

  // Update previous error for next iteration
  error_prev = error;

  // Calculate PID output
  float output = kp * error + ki * error_integral + kd * (error - error_prev);

  // Limit output (optional)
  // ... (implement logic to limit output if necessary)

  return output;
}