void error_handling_task(void *pvParameters) {
  for (;;){
    // print out the value you read:
    mcp.digitalWrite(LOGSEL12_24_3, HIGH);
    Serial.println("LOGSEL12_24_3, HIGH");
    vTaskDelay(2000); // 100ms delay
    mcp.digitalWrite(LOGSEL12_24_3, LOW);
    Serial.println("LOGSEL12_24_3, LOW");
    vTaskDelay(2000); // 100ms delay

  }
}
