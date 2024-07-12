void buck_task(void *pvParameters) {

 //PWM INITIALIZATION
  ledcSetup(PWM_CHANNEL_C,pwmFrequency,pwmResolution);          //Set PWM Parameters
  ledcAttachPin(buck_IN, PWM_CHANNEL_C);                        //Set pin as PWM
  ledcWrite(PWM_CHANNEL_C,PWM);                                 //Write PWM value at startup (duty = 0)
  pwmMax = pow(2,pwmResolution)-1;                           //Get PWM Max Bit Ceiling
  pwmMaxLimited = (PWM_MaxDC*pwmMax)/100.000;                //Get maximum PWM Duty Cycle (pwm limiting protection)                          
  buck_Disable();

  for (;;){
    // print out the value you read:
    Charging_Algorithm();   //TAB#5 - Battery Charging Algorithm
    Serial.println("niko buck");  
    vTaskDelay(5); // 10ms delay
  }

}







void resetVariables(){
  secondsElapsed = 0;
  energySavings  = 0; 
  daysRunning    = 0; 
  timeOn         = 0; 
}

void backflowControl(){                                                //PV BACKFLOW CONTROL (INPUT MOSFET) 
  if(output_Mode==0){bypassEnable=1;}                                  //PSU MODE: Force backflow MOSFET on
  else{                                                                //CHARGER MODE: Force backflow MOSFET on
    if(voltageInput>voltageOutput+voltageDropout){bypassEnable=1;}     //CHARGER MODE: Normal Condition - Turn on Backflow MOSFET (on by default when not in MPPT charger mode)
    else{bypassEnable=0;}                                              //CHARGER MODE: Input Undervoltage - Turn off bypass MOSFET and prevent PV Backflow (SS)
  }
  mcp.digitalWrite(GPIO27,bypassEnable);                               //Signal backflow MOSFET GPIO pin   
}


void setOutPutMode(bool ChargeMode){
  if(!ChargeMode){
    output_Mode  = 0; //PSU mode
  }
  else{
    output_Mode  = 1; //Charging Mode
  }
}

void setChargingAlgorithm(bool MpptAlgo){
  if(!MpptAlgo){
    MPPT_Mode  = 0; //CV-CC mode
  }
  else{
    MPPT_Mode  = 1; //Mppt Algorithm
  }
}

void buck_Enable(){                                                                  //Enable MPPT Buck Converter
  buckEnable = 1;
  mcp.digitalWrite(GPIO32,HIGH);
}
void buck_Disable(){                                                                 //Disable MPPT Buck Converter
  buckEnable = 0; 
  mcp.digitalWrite(GPIO32,LOW);
  PWM = 0;
}   
void predictivePWM(){                                                                //PREDICTIVE PWM ALGORITHM 
  if(voltageInput<=0){PPWM=0;}                                                       //Prevents Indefinite Answer when voltageInput is zero
  else{PPWM =(PPWM_margin*pwmMax*voltageOutput)/(100.00*voltageInput);}              //Compute for predictive PWM Floor and store in variable
  PPWM = constrain(PPWM,0,pwmMaxLimited);
}   

void PWM_Modulation(){
  if(output_Mode==0){PWM = constrain(PWM,0,pwmMaxLimited);}                          //PSU MODE PWM = PWM OVERFLOW PROTECTION (limit floor to 0% and ceiling to maximim allowable duty cycle)
  else{
    predictivePWM();                                                                 //Runs and computes for predictive pwm floor
    PWM = constrain(PWM,PPWM,pwmMaxLimited);                                         //CHARGER MODE PWM - limit floor to PPWM and ceiling to maximim allowable duty cycle)                                       
  } 
  ledcWrite(PWM_CHANNEL_C,PWM);                                                         //Set PWM duty cycle and write to GPIO when buck is enabled
  buck_Enable();                                                                     //Turn on MPPT buck (IR2104)
}
     
void Charging_Algorithm(){
  if(ERR>0||chargingPause==1){buck_Disable();}                                       //ERROR PRESENT  - Turn off MPPT buck when error is present or when chargingPause is used for a pause override
  else{
    if(REC==1){                                                                      //IUV RECOVERY - (Only active for charging mode)
      REC=0;                                                                         //Reset IUV recovery boolean identifier 
      buck_Disable();
      SytemPrint += "System:> Solar Panel Detected > Computing For Predictive PWM ,";                                      //Display serial message                             //Display serial message 
      for(int i = 0; i<40; i++){Serial.print(".");delay(30);}                        //For loop "loading... effect                                                          //Display a line break on serial for next lines  
      predictivePWM();
      PWM = PPWM; 
    }  
    else{                                                                            //NO ERROR PRESENT  - Continue power conversion              
      /////////////////////// CC-CV BUCK PSU ALGORITHM ////////////////////////////// 
      if(MPPT_Mode==0){                                                              //CC-CV PSU MODE
        if(currentOutput>currentCharging)       {PWM--;}                             //Current Is Above → Decrease Duty Cycle
        else if(voltageOutput>voltageBatteryMax){PWM--;}                             //Voltage Is Above → Decrease Duty Cycle   
        else if(voltageOutput<voltageBatteryMax){PWM++;}                             //Increase duty cycle when output is below charging voltage (for CC-CV only mode)
        else{}                                                                       //Do nothing when set output voltage is reached 
        PWM_Modulation();                                                            //Set PWM signal to Buck PWM GPIO       
      }     
        /////////////////////// MPPT & CC-CV CHARGING ALGORITHM ///////////////////////  
      else{                                                                                                                                                         
        if(currentOutput>currentCharging){PWM--;}                                      //Current Is Above → Decrease Duty Cycle
        else if(voltageOutput>voltageBatteryMax){PWM--;}                               //Voltage Is Above → Decrease Duty Cycle   
        else{                                                                          //MPPT ALGORITHM
          if(voltageInput>maxPowerVin)     {PWM++;}  //  ↑P ↑V ; →MPP  //D--
          else {PWM--; } 

          if(SEARCH == 0){
            if(((int(voltageInput*10)/10) == maxPowerVin) && (powerInput>powerInputPrev)){
              maxPowerVin = maxPowerVin - 0.1;
              powerInputPrev = powerInput; 
            }
            else if(((int(voltageInput*10)/10) == maxPowerVin) && (powerInput<powerInputPrev)){
              SEARCH == 1;
              powerInputPrev = powerInput; 
            }
          }
          else{
            if(((int(voltageInput*10)/10) == maxPowerVin) && (powerInput>powerInputPrev)){
              maxPowerVin = maxPowerVin + 0.1;
              powerInputPrev = powerInput; 
            }
            else if(((int(voltageInput*10)/10) == maxPowerVin) && (powerInput<powerInputPrev)){
              SEARCH == 0;
              powerInputPrev = powerInput; 
            }

          }
           
                                                        //Store Previous Recorded Power
          voltageInputPrev = voltageInput;                                             //Store Previous Recorded Voltage        
        }   
        PWM_Modulation();                                                              //Set PWM signal to Buck PWM GPIO                                                                       
      }  
    }
  }
}



