void system_task(void *pvParameters) {
  ledcSetup(PWM_CHANNEL_D,fanpwmFrequency,fanpwmResolution);          //Set PWM Parameters
  ledcAttachPin(fanPin, PWM_CHANNEL_D);                        //Set pin as PWM
  ledcWrite(PWM_CHANNEL_D,fanPWM);   

  for (;;){
    System_Processes();     //TAB#4 - Routine system processes 
    webSocket.loop();
  }   
}


void System_Processes(){
  ///////////////// FAN COOLING /////////////////
  if(enableFan==true){
    if(enableDynamicCooling==false){                                   //STATIC PWM COOLING MODE (2-PIN FAN - no need for hysteresis, temp data only refreshes after 'avgCountTS' or every 500 loop cycles)                       
      if(overrideFan==true){fanStatus=true;}                           //Force on fan
      else if((temperature1>=temperatureFan || temperature2>=temperatureFan) && fanPWM < 256)
      {
        fanPWM++;
      }               //Turn on fan when set fan temp reached
      else if((temperature1<=temperatureFan || temperature2<=temperatureFan) && fanPWM > 0)
      {
        fanPWM--;
      }                //Turn off fan when set fan temp reached
                                           //Send a digital signal to the fan MOSFET
    }
    else{}                                                             //DYNAMIC PWM COOLING MODE (3-PIN FAN - coming soon)
  }
  else{}                                         //Fan Disabled
  ledcWrite(PWM_CHANNEL_D,fanPWM); 
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


void handleLogin(AsyncWebServerRequest *request) {
  if (request->hasParam("username", true) && request->hasParam("password", true)) {
    String usernameParam = request->getParam("username", true)->value();
    String passwordParam = request->getParam("password", true)->value();

    if (usernameParam == "avel" && passwordParam == "1526") {
      loggedIn = true;
      request->send(200, "text/plain", "Login successful. Redirecting to firmware update...");
    } else {
      request->send(403, "text/plain", "Login failed. Invalid credentials.");
    }
  } else {
    request->send(400, "text/plain", "Bad Request");
  }
}

void handleFirmwareUpdate(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!loggedIn) {
    request->send(403, "text/plain", "Access denied. Please log in first.");
    return;
  }

  if (!index) {
    Serial.println("Receiving firmware update...");
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  }

  if (len) {
    if (Update.write(data, len) != len) {
      Update.printError(Serial);
    }
  }

  if (final) {
    if (Update.end(true)) {
      request->send(200, "text/plain", "Update success. Rebooting...");
      ESP.restart();
    } else {
      request->send(500, "text/plain", "Update failed");
    }
  }
}

StaticJsonDocument<200> jsonMessage;

void updateParameters(){
  
// Fill the JSON object with parameter values
jsonMessage["VI"] = voltageInput;
jsonMessage["CI"] = currentInput;
jsonMessage["VO"] = voltageOutput;
jsonMessage["PI"] = powerInput;
jsonMessage["Buck"] = buckEnable;
jsonMessage["Error"] = ERR;
jsonMessage["Mppta"] = MPPT_Mode;

// Convert the JSON object to a string
String jsonString;
serializeJson(jsonMessage, jsonString);

  webSocket.broadcastTXT(jsonString);
}

void handleWebSocketMessage(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_TEXT: {
         String receivedText = "";
        for (size_t i = 0; i < length; i++) {
          receivedText += (char)payload[i];
        }

        // Handle the received text message
        if (receivedText == "restart") {
          // Restart ESP32 if the received text is "restart"
          ESP.restart();
        } 
        else if(receivedText == "mppt=1") {
          setChargingAlgorithm(true);
        }
        else if(receivedText == "mppt=0") {
          setChargingAlgorithm(false);
        }
        else if(receivedText == "charge=1"){
          setOutPutMode(true);
        }
        else if(receivedText == "charge=0"){
          setOutPutMode(false);
        }
        else{
          
        }
      break;
    }
    case WStype_CONNECTED:
      // A client has connected, send initial parameter values
      webSocket.sendTXT(num, "VI:" + String(voltageInput) + "  CI:" + String(currentInput) + "  VO:" + String(voltageOutput) + " PI" + String(powerInput) + " Buck:" + String(buckEnable) + "  Error:" + String(ERR) + " Mppta" + String(MPPT_Mode));
      // Add more parameters as needed
      break;
    case WStype_DISCONNECTED:
      // A client has disconnected
      break;
  }
}


void mDnS(){
   /*use mdns for host name resolution*/
        if (!MDNS.begin(hostname)) 
        { //http://esp32.local
          Serial.println("Error setting up MDNS responder!");
          
            delay(2000);
          
        }
        Serial.println("mDNS responder started");
        
        server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request) {
          request->send(200, "text/html", "Enter your login details:<br><form method='POST' action='/login'><input type='text' name='username' placeholder='Username'><br><input type='password' name='password' placeholder='Password'><br><input type='submit' value='Login'></form>");
        });

        server.on("/login", HTTP_POST, handleLogin);

        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
          if (loggedIn) {
            request->send(200, "text/html", "Logged in. Redirecting to the main page...<script>window.location.replace('/main');</script>");
          } else {
            request->redirect("/login");
          }
        });

        server.on("/main", HTTP_GET, [](AsyncWebServerRequest *request) {
          if (loggedIn) {
            String html = "<html><head><script>var socket = new WebSocket('ws://' + window.location.hostname + ':81/');"
                        "socket.onmessage = function(event) {document.getElementById('real-time-data').textContent = event.data;};"
                        "function sendData() {var data = document.getElementById('data-input').value;socket.send(data);}</script></head>"
                        "<body>Logged in. Welcome to the main page.<br><br>"
                        "Real-time Data: <div id='real-time-data'></div><br><br>"
                        "<input type='text' id='data-input' placeholder='Enter real-tidme data'><button onclick='sendData()'>Send Data</button><br><br>"
                        "Upload Firmware:<br>"
                        "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update Firmware'></form></body></html>";
            request->send(200, "text/html", html);
          } else {
            request->redirect("/login");
          }
        });

        server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
          if (loggedIn) {
            request->send(200, "text/plain", "Firmware update page. Please use a POST request.");
          } else {
            request->send(403, "text/plain", "Access denied. Please log in first.");
          }
        }, handleFirmwareUpdate);

        server.begin();

        webSocket.begin();
        webSocket.onEvent(handleWebSocketMessage);
        updateTimer.attach(1, updateParameters);
}

