/**
******************************************************************************
* @file    micokit_ext.c
* @author  Eshen Wang
* @version V1.0.0
* @date    8-May-2015
* @brief   micokit extension board peripherals operations..
******************************************************************************
* @attention
*
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
* TIME. AS A RESULT, MXCHIP Inc. SHALL NOT BE HELD LIABLE FOR ANY
* DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
* FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
* CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* <h2><center>&copy; COPYRIGHT 2014 MXCHIP Inc.</center></h2>
******************************************************************************
*/ 

#include "micokit_ext.h"
#include "platform_config.h"

#define micokit_ext_log(M, ...) custom_log("MICOKIT_EXT", M, ##__VA_ARGS__)
#define micokit_ext_log_trace() custom_log_trace("MICOKIT_EXT")

extern mico_semaphore_t      mfg_test_state_change_sem;
extern volatile int16_t      mfg_test_module_number;

//---------------------------- user modules functions --------------------------

// Key1 clicked callback:  previous test module in test mode
void user_key1_clicked_callback(void)
{
  if(NULL != mfg_test_state_change_sem){
    if( 0 < mfg_test_module_number){
      mfg_test_module_number = (mfg_test_module_number - 1)%(MFG_TEST_MAX_MODULE_NUM+1);
    }
    else{
      mfg_test_module_number = MFG_TEST_MAX_MODULE_NUM;
    }
    mico_rtos_set_semaphore(&mfg_test_state_change_sem);  // go back to previous module
  }
  return;
}

// Key1 long pressed callback
void user_key1_long_pressed_callback(void)
{
  return;
}

// Key2 clicked callback:  next test module in test mode
void user_key2_clicked_callback(void)
{
  if(NULL != mfg_test_state_change_sem){
    mfg_test_module_number = (mfg_test_module_number+1)%(MFG_TEST_MAX_MODULE_NUM+1);
    mico_rtos_set_semaphore(&mfg_test_state_change_sem);  // start next module
  }
  return;
}

// Key2 long pressed callback(use for enter MFG MODE when reset)
void user_key2_long_pressed_callback(void)
{
  return;
}

//------------------------------------- API ------------------------------------
OSStatus user_modules_init(void)
{
  OSStatus err = kUnknownErr;
  char oled_show_line[16] = {'\0'};   // max char each line
  
  // init DC Motor(GPIO)
  dc_motor_init();
  dc_motor_set(0);   // off
  
  // init RGB LED(P9813)
  rgb_led_init();
  rgb_led_open(0, 0, 0);  // off
  
  // init OLED
  OLED_Init();
  OLED_Clear();
  snprintf(oled_show_line, 16, "%s", (uint8_t*)DEV_KIT_MANUFACTURER);
  OLED_ShowString(0,0,(uint8_t*)oled_show_line);
  memset(oled_show_line, '\0', 16);
  snprintf(oled_show_line, 16, "%s", (uint8_t*)DEV_KIT_NAME);
  OLED_ShowString(0,3,(uint8_t*)oled_show_line);
  OLED_ShowString(0,6,"Starting...     ");
  
  // init Light sensor(ADC)
  light_sensor_init();
  
  // init infrared sensor(ADC)
  infrared_reflective_init();
  
  // init user key1 && key2
  user_key1_init();
  user_key2_init();
  
  err = temp_hum_sensor_init();
  
  return err;
}

OSStatus micokit_ext_init(void)
{
  OSStatus err = kUnknownErr;
  err = user_modules_init();
  
  return err;
}
