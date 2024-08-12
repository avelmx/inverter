
void sensor_acquisition_task(void *pvParameters) {

  unsigned int high_water_mark = uxTaskGetStackHighWaterMark(NULL);

  for (;;) {
    Read_Sensors();  //TAB#2 - Sensor data measurement and computation
    Device_Protection();
    relayOperation();
    //Serial.print("sensor task Core " + String(xPortGetCoreID()));
    //Serial.println(" Task stack high water mark: " + String(high_water_mark));
    vTaskDelay(5);  // 100ms delay
                    // 100ms delay
  }
}


void ADC_SetGain() {
  if (ADS1015_Mode == true) {  //FOR ADS1015 12-BIT ADC MODEL
    if (ADC_GainSelect == 0) {
      ads1.setGain(GAIN_TWOTHIRDS);
      ADC_BitReso = 3.0000;
      ads2.setGain(GAIN_TWOTHIRDS);
      ADC_BitReso = 3.0000;
      ads3.setGain(GAIN_TWOTHIRDS);
      ADC_BitReso = 3.0000;
    }  // Gain: 2/3x  Range: +/- 6.144V
    else if (ADC_GainSelect == 1) {
      ads1.setGain(GAIN_ONE);
      ADC_BitReso = 2.0000;
      ads2.setGain(GAIN_ONE);
      ADC_BitReso = 2.0000;
      ads3.setGain(GAIN_ONE);
      ADC_BitReso = 2.0000;
    }  // Gain: 1x    Range: +/- 4.096V
    else if (ADC_GainSelect == 2) {
      ads1.setGain(GAIN_TWO);
      ADC_BitReso = 1.0000;
      ads2.setGain(GAIN_TWO);
      ADC_BitReso = 1.0000;
      ads3.setGain(GAIN_TWO);
      ADC_BitReso = 1.0000;
    }       // Gain: 2x    Range: +/- 2.048V
  } else {  //FOR ADS1115 16-BIT ADC MODEL
    if (ADC_GainSelect == 0) {
      ads1.setGain(GAIN_TWOTHIRDS);
      ADC_BitReso = 0.1875;
      ads2.setGain(GAIN_TWOTHIRDS);
      ADC_BitReso = 0.1875;
      ads3.setGain(GAIN_TWOTHIRDS);
      ADC_BitReso = 0.1875;
    }  // Gain: 2/3x  Range: +/- 6.144V
    else if (ADC_GainSelect == 1) {
      ads1.setGain(GAIN_ONE);
      ADC_BitReso = 0.125;
      ads2.setGain(GAIN_ONE);
      ADC_BitReso = 0.125;
      ads3.setGain(GAIN_ONE);
      ADC_BitReso = 0.125;
    }  // Gain: 1x    Range: +/- 4.096V
    else if (ADC_GainSelect == 2) {
      ads1.setGain(GAIN_TWO);
      ADC_BitReso = 0.0625;
      ads2.setGain(GAIN_TWO);
      ADC_BitReso = 0.0625;
      ads3.setGain(GAIN_TWO);
      ADC_BitReso = 0.0625;
    }  // Gain: 2x    Range: +/- 2.048V
  }
}

void Read_Sensors() {

  /////////// TEMPERATURE SENSOR /////////////
  //Serial.println("acquiring temp readings");

  if (sampleStoreTS <= avgCountTS) {
    if (xSemaphoreTake(i2c_bus1, 3) == pdTRUE) {  //TEMPERATURE SENSOR - Lite Averaging
      TS1 = TS1 + ads2.readADC_SingleEnded(1);
      TS2 = TS2 + ads2.readADC_SingleEnded(2);
      xSemaphoreGive(i2c_bus1);
      sampleStoreTS++;
    } else {
      SytemPrint += "couldnt aquire bus no temp readings";
    }
  } else {
    TS1 = TS1 / sampleStoreTS;
    TS2 = TS2 / sampleStoreTS;
    Rntc1 = 1000 * ((TS1 * 9.1) / (3.3 - 0.001 * TS1));
    Rntc2 = 1000 * ((TS2 * 9.1) / (3.3 - 0.001 * TS2));
    temperature1 = (1 / ((1 / 298.15) + ((2.303 / 4000) * (log(Rntc1 / 100000))))) - 273.15;
    temperature2 = (1 / ((1 / 298.15) + ((2.303 / 4000) * (log(Rntc2 / 100000))))) - 273.15;
    sampleStoreTS = 0;
    TS1 = 0;
    TS2 = 0;

    //Protection trigger ISR
    if ((abs(temperature1 - prevtrigtemp1) > temperaturetolereance) || (abs(temperature2 - prevtrigtemp2) > temperaturetolereance)) {
      prevtrigtemp1 = temperature1;
      prevtrigtemp2 = temperature2;
      trigprotectionTask = true;
    }
  }
  /////////// VOLTAGE & CURRENT SENSORS /////////////
  VSI = 0.0000;  //Clear Previous Input Voltage
  VSO = 0.0000;  //Clear Previous Output Voltage
  CSI = 0.0000;  //Clear Previous Current
  VS24V = 0.0000;
  ACVSO = 0.0000;
  IS12V = 0.0000;
  IS24V = 0.0000;

  //VOLTAGE SENSOR - Instantenous Averaging
  //Serial.println("acquiring voltage readings");
  for (int i = 0; i < avgCountVS; i++) {
    if (xSemaphoreTake(i2c_bus1, 3) == pdTRUE) {
      VSI = VSI + ads1.computeVolts(ads1.readADC_SingleEnded(3));
      //Serial.println(VSI);
      VSO = VSO + ads1.computeVolts(ads1.readADC_SingleEnded(1));
      VS24V = VS24V + ads3.computeVolts(ads3.readADC_SingleEnded(1));
      xSemaphoreGive(i2c_bus1);
    } else {
      SytemPrint += "couldnt aquire bus no VSI,VSO,V24 readings";
    }
  }

  voltageInput = (VSI / avgCountVS) * inVoltageDivRatio;

  voltageOutput = (VSO / avgCountVS) * outVoltageDivRatio;
  voltage24bus = (VS24V / avgCountVS) * outVoltageDivRatio;

  //Protection trigger ISR
  if ((abs(prevtrigvoltInput - voltageInput) > voltagetolerance) || (abs(prevtrigvoltOutput - voltageOutput) > voltagetolerance) || (abs(prevtrigvolt24bus - voltage24bus) > voltagetolerance)) {
    prevtrigvoltInput = voltageInput;
    prevtrigvoltOutput = voltageOutput;
    prevtrigvolt24bus = voltage24bus;
    trigprotectionTask = true;
  }


  //CURRENT SENSOR - Instantenous Averaging
  //Serial.println("acquiring current readings");
  for (int i = 0; i < avgCountCS; i++) {
    if (xSemaphoreTake(i2c_bus1, 0) == pdTRUE) {
      CSI = CSI + ads1.computeVolts(ads1.readADC_SingleEnded(2));
      IS12V = IS12V + ads3.computeVolts(ads3.readADC_SingleEnded(0));
      IS24V = IS24V + ads3.computeVolts(ads3.readADC_SingleEnded(1));
      xSemaphoreGive(i2c_bus1);
    } else {
      SytemPrint += "couldnt aquire bus no CSI,IS12V,IS24V readings";
    }
  }
  CSI_converted = (CSI / avgCountCS) * 1.3300;
  currentInput = ((CSI_converted - currentMidPoint) * -1) / currentSensV;
  if (currentInput < 0) { currentInput = 0.0000; }
  if (voltageOutput <= 0) {
    currentOutput = 0.0000;
  } else {
    currentOutput = (voltageInput * currentInput) / voltageOutput;
  }

  //POWER SOURCE DETECTION
  if (voltageInput <= 3 && voltageOutput <= 3) { inputSource = 0; }  //System is only powered by USB port
  else if (voltageInput > voltageOutput) {
    inputSource = 1;
  }                                                            //System is running on solar as power source
  else if (voltageInput < voltageOutput) { inputSource = 2; }  //System is running on batteries as power source

  //////// AUTOMATIC CURRENT SENSOR CALIBRATION ////////
  if (buckEnable == 0 && FLV == 0 && OOV == 0) {
    currentMidPoint = ((CSI / avgCountCS) * 1.3300) - 0.003;
  }

  //POWER COMPUTATION - Through computation
  powerInput = voltageInput * currentInput;
  powerOutput = voltageInput * currentInput * efficiencyRate;
  outputDeviation = (voltageOutput / voltageBatteryMax) * 100.000;

  //STATE OF CHARGE - Battery Percentage
  batteryPercent = ((voltageOutput - voltageBatteryMin) / (voltageBatteryMax - voltageBatteryMin)) * 101;
  batteryPercent = constrain(batteryPercent, 0, 100);
  //Protection trigger ISR
  if (abs(prevTrigbatteryPercent - batteryPercent) > batterytolerance) {
    prevTrigbatteryPercent = batteryPercent;
    trigprotectionTask = true;
  }



  //TIME DEPENDENT SENSOR DATA COMPUTATION
  currentRoutineMillis = millis();
  if (currentRoutineMillis - prevRoutineMillis >= millisRoutineInterval) {     //Run routine every millisRoutineInterval (ms)
    prevRoutineMillis = currentRoutineMillis;                                  //Store previous time
    Wh = Wh + (powerInput / (3600.000 * (1000.000 / millisRoutineInterval)));  //Accumulate and compute energy harvested (3600s*(1000/interval))
    kWh = Wh / 1000.000;
    MWh = Wh / 1000000.000;
    daysRunning = timeOn / (86400.000 * (1000.000 / millisRoutineInterval));  //Compute for days running (86400s*(1000/interval))
    timeOn++;                                                                 //Increment time counter
  }

  //OTHER DATA
  secondsElapsed = millis() / 1000;                    //Gets the time in seconds since the was turned on  and active
  energySavings = electricalPrice * (Wh / 1000.0000);  //Computes the solar energy saving in terms of money (electricity flag rate)
 
}
