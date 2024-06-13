
void inverter_task(void *pvParameters) {
  // Configure LED control peripheral (LEDC) for PWM generation
  ledc_timer_config_t ledc_timer_config = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .duty_resolution = ADC_RESOLUTION,  // 8 bits for 256 possible duty cycle values (0-255)
    .timer_source = TIMER_SOURCE_CLK,
    .clk_cfg = LEDC_CLK_APB_DIV,  // Use APB clock divided by 'timer_divider'
    .tick_freq = ESP_CLK_MHZ * 1000000, // Set base timer frequency (1 MHz here)
    .divider = TIMER_DIVIDER,
  };
ledc_timer_config(PWM_CHANNEL_A, &ledc_timer_config);
ledc_timer_config(PWM_CHANNEL_B, &ledc_timer_config);

  // Configure LED channel
  ledc_channel_config_t ledc_channel_config = {
    .channel = LEDC_CHANNEL,
    .duty = 0,
    .gpio_intr_type = LEDC_INTR_DISABLE,
    .duty_scale = LEDC_DUTY_SCALE_1,
    .sig_out_mode = LEDC_SIG_OUT_MODE_DISABLE,
    .idle_level = 0,
    .flags = LEDC_CHAN_DEFAULT,
  };
ledc_channel_config(PWM_CHANNEL_A, &ledc_channel_config);
ledc_channel_config(PWM_CHANNEL_B, &ledc_channel_config);

  // Initialize variables
  float sine_value = 0;  // Replace with logic to get sampled sine wave value (ADC reading)
  int duty_cycle = 0;

  // Start LED channel
  ledcAttachPin(IN1_PIN, PWM_CHANNEL_A);  
  ledcAttachPin(IN2_PIN, PWM_CHANNEL_B); 
  ledc_channel_start(PWM_CHANNEL_A);
  ledc_channel_start(PWM_CHANNEL_B);

  while (true) {
    unsigned long runningTime = millis(); // Current running time in milliseconds
    // Calculate duty cycle using LUT function
    duty_cycle =  calculateDutyCycle(runningTime, frequency);

    // Apply dead time
    if (duty_cycle >= 0) {
      ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_A, duty_cycle);
      delayMicroseconds(DEAD_TIME_CYCLES);
      ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_B, 0);  // Turn off low-side MOSFET
    } else if (duty_cycle < 0) {
      ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_B, abs(duty_cycle));  // Use absolute value for low-side
      delayMicroseconds(DEAD_TIME_CYCLES);
      ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_A, 0);  // Turn off high-side MOSFET
    } else {
      // Handle zero output condition (optional)
      ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_A, 0);
      ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_B, 0);
    }

    // Set LED channel duty cycle
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_A, duty_cycle);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_B, duty_cycle);

    // Delay for PWM frequency
    int delay_ms = 1 / (PWM_FREQUENCY / 1000); // Delay in milliseconds
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
  }
}



// Function to calculate duty cycle
float calculateDutyCycle(unsigned long runningTime, float frequency) {
    // Calculate the period of the sine wave
    float period = 1.0 / frequency;   
    // Calculate the time for one half of the sine wave (from 0 to peak)
    float halfPeriod = period / 2.0;   
    // Calculate the number of complete cycles in the running time
    int completeCycles = runningTime / period;   
    // Calculate the remaining time after complete cycles
    float remainingTime = runningTime % period;  
    // If there's no remaining time, return 0 (no duty cycle)
    if (remainingTime == 0) {
        return 0.0;
    }

    // Calculate the duty cycle as a percentage
    float period_ratio = remainingTime / period;
    float sine_value = sin(period_ratio * M_PI * 2);
     // Scale sine_value based on ADC resolution
    float scaled_value = (float)sine_value * (pow(2, ADC_RESOLUTION) - 1);
    // Offset and scaling for positive/negative voltage control
    int duty_cycle = (int)scaled_value;
    return duty_cycle;
}



