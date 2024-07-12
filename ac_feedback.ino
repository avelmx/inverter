void ac_feedback_task(void *pvParameters) {
  int counter = 0;
  for (;;){
    counter = 0;
    while(noacsampledata){
      acdataArray[counter].acvoltage = (ads1.computeVolts(ads2.readADC_SingleEnded(0)) * 112.11);   // based on voltage divider ratio
      acdataArray[counter].accurrent = ads1.computeVolts(ads1.readADC_SingleEnded(0)) ;
      acdataArray[counter].acsamplingTime =  millis();
      unsigned long theTime = acdataArray[counter].acsamplingTime;

      if(xSemaphoreTake(shared_lut_mutex, 0) == pdTRUE){ // should not wait if mutex is not available. The code uses interpolation anyways to calculate error between samples; also helps to fullfill the stated no of sps
        errordataArray[counter % acfrequency].ac_voltage_errorCorrection = calculatePerfectSineVoltageValue(theTime, acfrequency) - acdataArray[counter].acvoltage;
        errordataArray[counter % acfrequency].associated_period_ratio = (theTime % (1/acfrequency)) / (1/acfrequency);
        xSemaphoreGive(shared_lut_mutex);
      }

      acData[counter % acfrequency].ac_voltage = acdataArray[counter].acvoltage;
      acData[counter % acfrequency].associated_period_ratio = (theTime % (1/acfrequency)) / (1/acfrequency);
      
      if((counter/(noacsampledata/10)) == 0){ // after 10ms acquire rms - equivalent to 5 complete cycles
        if(acrmsvoltage == 0){
          acrmsvoltage = calculate_rms_voltage(acdataArray, counter);  //get ac rms voltage
          acrmscurrent = calculate_rms_current(acdataArray, counter);
          adjustscaledfactor(acrmsvoltage);
        }
        else {
          acrmsvoltage = 0.6*acrmsvoltage + 0.4*calculate_rms_voltage(acdataArray, counter); // simple averaging
          acrmscurrent = 0.6*acrmscurrent + 0.4*calculate_rms_current(acdataArray, counter);
        }
        if(acrmsvoltage < acMinRmsVoltage ){
          ACOUV = 1;  // flag the over voltage and trigger protection task
          trigprotectionTask = true;
        }
        else if(acrmsvoltage > acMaxRmsVoltage){
          ACOOV = 1;
          trigprotectionTask = true;
        }
        if(acrmscurrent > maxAcrmsCurrent){
          ACOUI = 1;
          trigprotectionTask = true;
        }
      }
      counter++;
      if(trigprotectionTask){
        wakeprotectionTask();
      }
      Serial.println("niko ac feedback");  
      vTaskDelay(pdMS_TO_TICKS(1/noacsampledata)); //needs to take 2500sps so this delay calculated the delay between each sample. must not exceed 3300sps for ads1015 as per datasheet
    }

    feedbackfrequency = calculate_frequency(acdataArray, noacsampledata);

    if(feedbackfrequency < (acfrequency - 3)){
      ACOUF = 1;
      trigprotectionTask = true;
    }
    else if(feedbackfrequency > (acfrequency + 3)){
      ACOOF = 1;
      trigprotectionTask = true;
    }
  }
}

void adjustscaledfactor(float acrms)  {
  if((scaledfactor <= 1) && ((acrms - targetAcRMSVoltage) > 1)){
    scaledfactor = scaledfactor - 0.05;
  }
  else if((scaledfactor <= 1) && ((acrms - targetAcRMSVoltage) < -1)){
    scaledfactor = scaledfactor + 0.05;
  }
  else{
    scaledfactor = 1;
  }
}

float calculate_rms_voltage(acsampleData_t *data, int length) {
  float squared_sum = 0.0;
  for (int i = 0; i < length; i++) {
    squared_sum += pow(data[i].acvoltage, 2);
  }
  return sqrt(squared_sum / length);
}

float calculate_rms_current(acsampleData_t *data, int length) {
  float squared_sum = 0.0;
  for (int i = 0; i < length; i++) {
    squared_sum += pow(data[i].accurrent, 2);
  }
  return sqrt(squared_sum / length);
}

float calculate_frequency(acsampleData_t *data, float sampling_rate) {
  int num_peaks = 0;
  float total_time_diff = 0.0;
  float prev_peak_time = data[0].acsamplingTime;

  for (int i = 3; i < noacsampledata - 4; i++) {
    float peak_voltage = 0.0;
    for (int j = i - 2; j <= i + 2; j++) {
      peak_voltage = fmax(peak_voltage, data[j].acvoltage);
    }

    float left_avg = (data[i - 1].acvoltage + data[i - 2].acvoltage + data[i - 3].acvoltage)/3; //reduce noise
    float right_avg = (data[i + 1].acvoltage + data[i + 2].acvoltage + data[i + 3].acvoltage)/3; //reduce noise
    if (peak_voltage > left_avg && peak_voltage > right_avg) {
      // Only consider positive peaks after rectification

      if (num_peaks % 2 == 1 && prev_peak_time != data[0].acsamplingTime) {
        total_time_diff += data[i].acsamplingTime - prev_peak_time;
        prev_peak_time = data[i].acsamplingTime;
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
    unsigned long period = 1.0 / frequency;     
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


void wakeprotectionTask(){
  uint8_t flag = 1; // Set flag value
      if (xQueueSend(protection_queue, &flag, portMAX_DELAY) != pdTRUE) {
        SytemPrint += "System:Failed to woke protection task. Possibly already woken,";
      }
}
