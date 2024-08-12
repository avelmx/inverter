

void Device_Protection(){
  //Serial.println("yooooo"); 
  //ERROR COUNTER RESET
  currentRoutineMillis = millis();
  if(currentErrorMillis-prevErrorMillis>=errorTimeLimit){                                           //Run routine every millisErrorInterval (ms)
    prevErrorMillis = currentErrorMillis;                                                           //Store previous time
    if(errorCount<errorCountLimit){errorCount=0;}                                                   //Reset error count if it is below the limit before x milliseconds  
    else{}                                                                                          // TO ADD: sleep and charging pause if too many errors persists   
  } 
  //FAULT DETECTION     
  ERR = 0;                                                                                          //Reset local error counter
  backflowControl();                                                                                //Run backflow current protection protocol  
  if(temperature1>temperatureMax || temperature2>temperatureMax )                           {OTE=1;ERR++;errorCount++;}else{OTE=0;}  //OTE - OVERTEMPERATURE: System overheat detected
  if(currentInput>currentInAbsolute)                       {IOC=1;ERR++;errorCount++;}else{IOC=0;}  //IOC - INPUT  OVERCURRENT: Input current has reached absolute limit
  if(currentOutput>currentOutAbsolute)                     {OOC=1;ERR++;errorCount++;}else{OOC=0;}  //OOC - OUTPUT OVERCURRENT: Output current has reached absolute limit 
  if(voltageOutput>voltageBatteryMax+voltageBatteryThresh) {OOV=1;ERR++;errorCount++;}else{OOV=0;}  //OOV - OUTPUT OVERVOLTAGE: Output voltage has reached absolute limit                     
  if(voltageInput<vInSystemMin&&voltageOutput<vInSystemMin){FLV=1;ERR++;errorCount++;}else{FLV=0;}  //FLV - Fatally low system voltage (unable to resume operation)

  if(output_Mode==0)
  { 
    //Serial.println("dataString");                                                                              //PSU MODE specific protection protocol
    REC = 0; BNC = 0;                                                                               //Clear recovery and battery not connected boolean identifiers
    if(voltageInput<voltageBatteryMax+voltageDropout){IUV=1;ERR++;errorCount++;}else{IUV=0;}        //IUV - INPUT UNDERVOLTAGE: Input voltage is below battery voltage (for psu mode only)                     
  }
  else
  {
    //Serial.println("dataString2");                                                                                              //Charger MODE specific protection protocol
    backflowControl();                                                                              //Enable backflow current detection & control                           
    if(voltageOutput<vInSystemMin)                   {BNC=1;ERR++;}      else{BNC=0;}               //BNC - BATTERY NOT CONNECTED (for charger mode only, does not treat BNC as error when not under MPPT mode)
    if(voltageInput<voltageBatteryMax+voltageDropout){IUV=1;ERR++;REC=1;}else{IUV=0;}               //IUV - INPUT UNDERVOLTAGE: Input voltage is below max battery charging voltage (for charger mode only)     
  } 

}


void relayOperation()
{

  if(ACOUV || ACOOV || ACOUF || ACOOF || ACOUI) {
    if(xSemaphoreTake(i2c_bus1, 0) == pdTRUE)
    {
      //Serial.println("prote 1 err");
      mcp.digitalWrite(AC_1_LOG,LOW);     // drops AC load
      mcp.digitalWrite(AC_2_LOG,LOW);
      xSemaphoreGive(i2c_bus1);
    }
    ACinitialize          = 1;
  }

  if(enable12Vbus){
    enable24Vbus  = 0;
    if(xSemaphoreTake(i2c_bus1, 0) == pdTRUE)
    {
      //Serial.println("prote 2 err");
      mcp.digitalWrite(LOGSEL12_24_1,LOW);    
      mcp.digitalWrite(LOGSEL12_24_2,LOW);
      mcp.digitalWrite(LOGSEL12_24_3,HIGH); 
      mcp.digitalWrite(BAT_LOG,HIGH);
      xSemaphoreGive(i2c_bus1);
    }
  }
  else if(enable24Vbus){
    enable12Vbus = 0;
    if(xSemaphoreTake(i2c_bus1, 0) == pdTRUE)
    {
      //Serial.println("prote 3 err");
      mcp.digitalWrite(LOGSEL12_24_3,LOW);
      mcp.digitalWrite(LOGSEL12_24_1,HIGH);     
      mcp.digitalWrite(LOGSEL12_24_2,HIGH);
      mcp.digitalWrite(BAT_LOG,HIGH);
      xSemaphoreGive(i2c_bus1);
    }
  }

  if(takeacload == true){
    if(xSemaphoreTake(i2c_bus1, 0) == pdTRUE)
      {
        mcp.digitalWrite(AC_1_LOG,HIGH);
        mcp.digitalWrite(AC_2_LOG,HIGH);
        xSemaphoreGive(i2c_bus1);
      }
  }

  if(takeacload == false){
    if(xSemaphoreTake(i2c_bus1, 0) == pdTRUE)
      {
        mcp.digitalWrite(AC_1_LOG,LOW);
        mcp.digitalWrite(AC_2_LOG,LOW);
        xSemaphoreGive(i2c_bus1);
      }
  }
}