
void sensor_acquisition_task(void *pvParameters) {



  for (;;){
    // print out the value you read:
    
    Serial.print("ADS1 Channel 0: "); Serial.println(ads1.readADC_SingleEnded(0));
    vTaskDelay(1000);
    Serial.print("ADS2 Channel 0: "); Serial.println(ads2.readADC_SingleEnded(1));
    vTaskDelay(1000);
    Serial.print("ADS3 Channel 0: "); Serial.println(ads3.readADC_SingleEnded(1));
    vTaskDelay(1000); // 100ms delay
  }
}
