void system_task(void *pvParameters) {
  for (;;){
     System_Processes();     //TAB#4 - Routine system processes 
     Onboard_Telemetry();    //TAB#6 - Onboard telemetry (USB & Serial Telemetry)
  }   
}


void System_Processes(){
  ///////////////// FAN COOLING /////////////////
  if(enableFan==true){
    if(enableDynamicCooling==false){                                   //STATIC PWM COOLING MODE (2-PIN FAN - no need for hysteresis, temp data only refreshes after 'avgCountTS' or every 500 loop cycles)                       
      if(overrideFan==true){fanStatus=true;}                           //Force on fan
      else if(temperature>=temperatureFan){fanStatus=1;}               //Turn on fan when set fan temp reached
      else if(temperature<temperatureFan){fanStatus=0;}                //Turn off fan when set fan temp reached
      mcp.digitalWrite(FAN,fanStatus);                                     //Send a digital signal to the fan MOSFET
    }
    else{}                                                             //DYNAMIC PWM COOLING MODE (3-PIN FAN - coming soon)
  }
  else{mcp.digitalWrite(FAN,LOW);}                                         //Fan Disabled
  
  //////////// LOOP TIME STOPWATCH ////////////
  loopTimeStart = micros();                                            //Record Start Time
  loopTime = (loopTimeStart-loopTimeEnd)/1000.000;                     //Compute Loop Cycle Speed (mS)
  loopTimeEnd = micros();                                              //Record End Time

  ///////////// AUTO DATA RESET /////////////
  if(telemCounterReset==0){}                                           //Never Reset
  else if(telemCounterReset==1 && daysRunning>1)  {resetVariables();}  //Daily Reset
  else if(telemCounterReset==2 && daysRunning>7)  {resetVariables();}  //Weekly Reset
  else if(telemCounterReset==3 && daysRunning>30) {resetVariables();}  //Monthly Reset
  else if(telemCounterReset==4 && daysRunning>365){resetVariables();}  //Yearly Reset 

  ///////////// LOW POWER MODE /////////////
  if(lowPowerMode==1){}   
  else{}      
}


void Onboard_Telemetry(){
   /////////////////////// USB SERIAL DATA TELEMETRY ////////////////////////   
      // 0 - Disable Serial
      // 1 - Display All
      // 2 - Display Essential Data
      // 3 - Display Numbers Only 

      currentSerialMillis = millis();
      if(currentSerialMillis-prevSerialMillis>=millisSerialInterval){   //Run routine every millisRoutineInterval (ms)
        prevSerialMillis = currentSerialMillis;                         //Store previous time

        if(serialTelemMode==0){}
    //  else if(chargingPause==1){Serial.println("CHARGING PAUSED");}   // Charging paused message
        else if(serialTelemMode==1){                                    // 1 - Display All                           
          Serial.print(" ERR:");   Serial.print(ERR);
          Serial.print(" FLV:");   Serial.print(FLV);  
          Serial.print(" BNC:");   Serial.print(BNC);  
          Serial.print(" IUV:");   Serial.print(IUV); 
          Serial.print(" IOC:");   Serial.print(IOC); 
          Serial.print(" OOV:");   Serial.print(OOV); 
          Serial.print(" OOC:");   Serial.print(OOC);
          Serial.print(" OTE:");   Serial.print(OTE); 
          Serial.print(" REC:");   Serial.print(REC);
          Serial.print(" MPPTA:"); Serial.print(MPPT_Mode);     
          Serial.print(" CM:");    Serial.print(output_Mode);   //Charging Mode
          
          Serial.print(" "); 
          Serial.print(" BYP:");   Serial.print(bypassEnable);
          Serial.print(" EN:");    Serial.print(buckEnable);
          Serial.print(" FAN:");   Serial.print(fanStatus);    
          Serial.print(" WiFi:");  Serial.print(WIFI);      
          Serial.print(" ");  
          Serial.print(" PI:");    Serial.print(powerInput,0); 
          Serial.print(" PWM:");   Serial.print(PWM); 
          Serial.print(" PPWM:");  Serial.print(PPWM); 
          Serial.print(" VI:");    Serial.print(voltageInput,1); 
          Serial.print(" VO:");    Serial.print(voltageOutput,1); 
          Serial.print(" CI:");    Serial.print(currentInput,2); 
          Serial.print(" CO:");    Serial.print(currentOutput,2); 
          Serial.print(" Wh:");    Serial.print(Wh,2); 
          Serial.print(" Temp:");  Serial.print(temperature);  
          Serial.print(" "); 
          Serial.print(" CSMPV:"); Serial.print(currentMidPoint,3);  
          Serial.print(" CSV:");   Serial.print(CSI_converted,3);   
          Serial.print(" VO%Dev:");Serial.print(outputDeviation,1);   
          Serial.print(" SOC:");   Serial.print(batteryPercent);Serial.print("%"); 
          Serial.print(" T:");     Serial.print(secondsElapsed); 
          Serial.print(" LoopT:"); Serial.print(loopTime,3);Serial.print("ms");  
          Serial.println("");    
        }
        else if(serialTelemMode==2){ // 2 - Display Essential Data
          Serial.print(" PI:");    Serial.print(powerInput,0); 
          Serial.print(" PWM:");   Serial.print(PWM); 
          Serial.print(" PPWM:");  Serial.print(PPWM); 
          Serial.print(" VI:");    Serial.print(voltageInput,1); 
          Serial.print(" VO:");    Serial.print(voltageOutput,1); 
          Serial.print(" CI:");    Serial.print(currentInput,2); 
          Serial.print(" CO:");    Serial.print(currentOutput,2); 
          Serial.print(" Wh:");    Serial.print(Wh,2); 
          Serial.print(" Temp:");  Serial.print(temperature,1);  
          Serial.print(" EN:");    Serial.print(buckEnable);
          Serial.print(" FAN:");   Serial.print(fanStatus);   
          Serial.print(" SOC:");   Serial.print(batteryPercent);Serial.print("%"); 
          Serial.print(" T:");     Serial.print(secondsElapsed); 
          Serial.print(" LoopT:"); Serial.print(loopTime,3);Serial.print("ms");  
          Serial.println("");    
        }  
        else if(serialTelemMode==3){ // 3 - Display Numbers Only 
          Serial.print(" ");       Serial.print(powerInput,0); 
          Serial.print(" ");       Serial.print(voltageInput,1); 
          Serial.print(" ");       Serial.print(voltageOutput,1); 
          Serial.print(" ");       Serial.print(currentInput,2); 
          Serial.print(" ");       Serial.print(currentOutput,2);   
          Serial.print(" ");       Serial.print(Wh,2); 
          Serial.print(" ");       Serial.print(temperature,1);  
          Serial.print(" ");       Serial.print(buckEnable);
          Serial.print(" ");       Serial.print(fanStatus);   
          Serial.print(" ");       Serial.print(batteryPercent);
          Serial.print(" ");       Serial.print(secondsElapsed); 
          Serial.print(" ");       Serial.print(loopTime,3);
          Serial.print(" ");       Serial.println("");    
        }  

      } 

}