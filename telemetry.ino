void telemetry_task(void *pvParameters) {
 
  unsigned int high_water_mark = uxTaskGetStackHighWaterMark(NULL);
  
  for (;;){
    Onboard_Telemetry();    //TAB#6 - Onboard telemetry (USB & Serial Telemetry)
    //Serial.print("telemetry task Core " + String(xPortGetCoreID()));  
    //Serial.println(" Task stack high water mark: "+ String(high_water_mark)); 
    vTaskDelay(1000); // 100ms delay
  }
}

void Onboard_Telemetry(){
   /////////////////////// USB SERIAL DATA TELEMETRY ////////////////////////   
          feedbackloopfreq = counter;
          counter = 0;                                        
          //Serial.print(" ERR:");   Serial.print(ERR);
          String dataString = "ERR:" + String(ERR) + ",";
          //Serial.print(" FLV:");   Serial.print(FLV);
          dataString += "FLV:" + String(FLV) + ",";
         // Serial.print(" BNC:");   Serial.print(BNC);
          dataString += "BNC:" + String(BNC) + ",";  
          //Serial.print(" IUV:");   Serial.print(IUV); 
          dataString += "IUV:" + String(IUV) + ",";
         // Serial.print(" IOC:");   Serial.print(IOC); 
          dataString += "IOC:" + String(IOC) + ",";
          //Serial.print(" OOV:");   Serial.print(OOV); 
          dataString += "OOV:" + String(OOV) + ",";
          //Serial.print(" OOC:");   Serial.print(OOC);
          dataString += "OOC:" + String(OOC) + ",";
         // Serial.print(" OTE:");   Serial.print(OTE); 
          dataString += "OTE:" + String(OTE) + ",";
         // Serial.print(" REC:");   Serial.print(REC);
          dataString += "REC:" + String(REC) + ",";
         // Serial.print(" MPPTA:"); Serial.print(MPPT_Mode);   
          dataString += "MPPTA:" + String(MPPT_Mode) + ",";  
         // Serial.print(" CM:");    Serial.print(output_Mode);   //Charging Mode
          dataString += "CM:" + String(output_Mode) + ",";
         // Serial.print(" BYP:");   Serial.print(bypassEnable);
          dataString += "BYP:" + String(bypassEnable) + ",";
          //Serial.print(" EN:");    Serial.print(buckEnable);
          dataString += "BEN:" + String(buckEnable) + ",";
          //Serial.print(" FAN:");   Serial.print(fanStatus); 
          dataString += "FAN:" + String(fanStatus) + ",";   
         // Serial.print(" WiFi:");  Serial.print(WIFI); 
          dataString += "WiFi:" + String(WIFI) + ",";     
         // Serial.print(" PI:");    Serial.print(powerInput,0); 
          dataString += "PI:" + String(powerInput) + ",";
          //Serial.print(" VI:");    Serial.print(voltageInput,1);
          dataString += "BVI:" + String(voltageInput) + ","; 
          //Serial.print(" VO:");    Serial.println(voltageOutput,1);
          dataString += "BVO:" + String(voltageOutput) + ","; 
          //Serial.print(" CI:");    Serial.print(currentInput,2); 
          dataString += "BCI:" +String(currentInput) + ",";
         // Serial.print(" CO:");    Serial.print(currentOutput,2); 
          dataString += "CO:" + String(currentOutput) + ",";
         // Serial.print(" Wh:");    Serial.print(Wh,2); 
          dataString += "Wh:" + String(Wh) + ",";
         // Serial.print(" Temp1:");  Serial.print(temperature1);
          dataString += "Temp1:" + String(temperature1) + ",";
          //Serial.print(" Temp2:");  Serial.print(temperature1);  
          dataString += "Temp2:" + String(temperature2) + ",";
          //Serial.print(" SOC:");   Serial.print(batteryPercent);Serial.print("%"); 
          dataString += "SOC:" + String(batteryPercent) + ",";
          //Serial.print(" T:");     Serial.print(secondsElapsed); 
          dataString += "T:" + String(secondsElapsed) + ",";
          //Serial.print(" IE:");     Serial.print(inverterEnable); 
          dataString += "IE:" +String(inverterEnable) + ",";
          //Serial.print(" ACL:");     Serial.print(enableACload);
          dataString += "ACL:" +String( enableACload) + ",";
         // Serial.print(" ACV:");     Serial.print(acrmsvoltage);
          dataString += "ACV:" + String(acrmsvoltage) + ",";
         // Serial.print(" ACI:");     Serial.print(acrmscurrent);
          dataString += "ACI:" + String(acrmscurrent) + ",";
         // Serial.print(" ACF:");     Serial.print(feedbackfrequency);
          dataString += "ACF:" + String(feedbackfrequency) + ",";
          //samplees
          dataString += "FSPS:" + String(feedbackloopfreq) + ",";

         
          Serial.println(dataString); 
          if(SytemPrint== "")
          {

          }
          else{
            Serial.println(SytemPrint);
            SytemPrint = "";
          }         
         /*
          for(int i =0; i<= noacsampledata % acfrequency; i++) {
            Serial.println("acdata:" + String(acData[i].ac_voltage) + "_" + acData[i].associated_period_ratio + ",");
          }  
*/
}