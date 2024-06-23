  /*

  
  KEPT FOR FUTURE REFERENCE + CONTINUED DEV
  // Define a macro for ESP32-CAM boards
#define CAMERA_MODEL_AI_THINKER
#if defined(ESP32) && defined(CAMERA_MODEL_AI_THINKER)
#include "esp_camera.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"

#define CAMERA_MODEL_AI_THINKER
// Define the camera pins
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Time to sleep in seconds
#define TIME_TO_SLEEP 10
#endif
  
    #ifdef CAMERA_MODEL_AI_THINKER
  // Initialize the camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Camera initialization
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    DEBUG_DEV.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Camera initialized
  DEBUG_DEV.println("Camera initialized");

  // Take a picture
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    DEBUG_DEV.println("Camera capture failed");
    return;
  }
  // Use the frame buffer (fb) here...
  esp_camera_fb_return(fb);

  // Set wakeup time
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * 1000000);

  DEBUG_DEV.println("Going to sleep now");
  delay(1000);
  esp_deep_sleep_start();
  #endif
  */
  