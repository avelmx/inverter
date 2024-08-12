

void ac_feedback() {
  unsigned long
    theTime,
    time4pl = 0;
  int
    sampleindex = 0;

  float
    voltagesample = 0,
    currentsample = 0,
    product = 0,
    timeInPeriod = 0;


  if (xSemaphoreTake(i2c_bus1, 3) == pdTRUE) {
    currentsample = ads1.computeVolts(ads1.readADC_SingleEnded(0));
    voltagesample = ads2.computeVolts(ads2.readADC_SingleEnded(0)) * 112.11;  // based on voltage divider ratio
    theTime = millis();
    xSemaphoreGive(i2c_bus1);

    timeInPeriod = (remainingTime(theTime, acfrequency)) / (1 / acfrequency);
    product = timeInPeriod * noacsampledata;
    sampleindex = floor(product);

    if (product - sampleindex >= 0.5) {  // i am too lazy to do interpolation
      sampleindex = sampleindex + 1;
    }

    acdataArray[sampleindex].acvoltage = voltagesample;
    acdataArray[sampleindex].accurrent = currentsample;

    if (xSemaphoreTake(shared_lut_mutex, 0) == pdTRUE) {  // should not wait if mutex is not available. The code uses interpolation anyways to calculate error between samples; also helps to fullfill the stated no of sps
      errordataArray[sampleindex].ac_voltage_errorCorrection = calculatePerfectSineVoltageValue(theTime, acfrequency) - acdataArray[counter].acvoltage;
      errordataArray[sampleindex].associated_period_ratio = (remainingTime(theTime, acfrequency)) / (1.0 / acfrequency);
      xSemaphoreGive(shared_lut_mutex);
    }

    if ((counter / noacsampledata >= 1) && (counter % noacsampledata == 0)) {  
      if (acrmsvoltage == 0) {
        acrmsvoltage = calculate_rms_voltage(acdataArray, noacsampledata);  //get ac rms voltage
        acrmscurrent = calculate_rms_current(acdataArray, noacsampledata);
        adjustscaledfactor(acrmsvoltage);
      } else {
        acrmsvoltage = 0.6 * acrmsvoltage + 0.4 * calculate_rms_voltage(acdataArray, noacsampledata);  // simple averaging
        acrmscurrent = 0.6 * acrmscurrent + 0.4 * calculate_rms_current(acdataArray, noacsampledata);
      }

      feedbackfrequency = calculate_frequency(acdataArray, noacsampledata);

      if (acrmsvoltage < acMinRmsVoltage) {
        ACOUV = 1;  // flag the over voltage and trigger protection task
        trigprotectionTask = true;
      } else if (acrmsvoltage > acMaxRmsVoltage) {
        ACOOV = 1;
        trigprotectionTask = true;
      }
      if (acrmscurrent > maxAcrmsCurrent) {
        ACOUI = 1;
        trigprotectionTask = true;
       }
    }

    counter++;
  
  }


  //Serial.println(counter);
  //Serial.print("Stack size " + String(high_water_mark));
  //Serial.println(" niko ac feedback: Core " + String(xPortGetCoreID()));

  if (feedbackfrequency < (acfrequency - 3)) {
    ACOUF = 1;
  } else if (feedbackfrequency > (acfrequency + 3)) {
    ACOOF = 1;
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
  float prev_peak_time = data[0].timeInPeriod;

  for (int i = 3; i < noacsampledata - 4; i++) {
    float peak_voltage = 0.0;
    for (int j = i - 2; j <= i + 2; j++) {
      peak_voltage = fmax(peak_voltage, data[j].acvoltage);
    }

    float left_avg = (data[i - 1].acvoltage + data[i - 2].acvoltage + data[i - 3].acvoltage)/3; //reduce noise
    float right_avg = (data[i + 1].acvoltage + data[i + 2].acvoltage + data[i + 3].acvoltage)/3; //reduce noise
    if (peak_voltage > left_avg && peak_voltage > right_avg) {
      // Only consider positive peaks after rectification

      if (num_peaks % 2 == 1 && prev_peak_time != data[0].timeInPeriod) {
        total_time_diff += data[i].timeInPeriod - prev_peak_time;
        prev_peak_time = data[i].timeInPeriod;
      }
      num_peaks++;
    }
  }

  if (num_peaks < 1) {
  return 0; // Handle case with no peaks found
} else {
  float average_time_diff = total_time_diff / (num_peaks / 2);
  return 1.0 / average_time_diff;
}
}



float calculatePerfectSineVoltageValue(unsigned long runningTime, int frequency) {
    // Calculate the period of the sine wave
    double period = 1.0 / frequency;     
    // Calculate the number of complete cycles in the running time
    long unsigned int wholePart = (long unsigned int)((runningTime / 1000)/period);
    double remainingTime = (runningTime/1000) - (wholePart * period);  
    // If there's no remaining time, return 0 (no duty cycle)
    if (remainingTime == 0) {
        return 0.0;
    }

    // Calculate the duty cycle as a percentage
    double period_ratio = remainingTime / period;
    float sine_value = sin(period_ratio * M_PI * 2);
     // Scale sine_value based on ADC resolution
    float scaled_voltage = (float)sine_value * targetAcRMSVoltage * 1.414;
    return scaled_voltage;
}


double remainingTime(unsigned long runningTime, int frequency){
  // Calculate the period of the sine wave
    double period = 1.0 / frequency;     
    // Calculate the number of complete cycles in the running time
    long unsigned int wholePart = (long unsigned int)((runningTime / 1000)/period);
    double remainingTime = (runningTime/1000) - (wholePart * period); 
    return remainingTime;
}

void wakeprotectionTask(){
  uint8_t flag = 1; // Set flag value
      if (xQueueSend(protection_queue, &flag, portMAX_DELAY) != pdTRUE) {
        SytemPrint += "System:Failed to woke protection task. Possibly already woken,";
      }
}
