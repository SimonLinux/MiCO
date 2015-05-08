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

#define MICOKIT_EXT_TEST_START                0
#define MICOKIT_EXT_TEST_OLED                 1
#define MICOKIT_EXT_TEST_RGB_LED              2
#define MICOKIT_EXT_TEST_DC_MOTOR             3
#define MICOKIT_EXT_TEST_END                  4   // max module num +1

micokit_system_work_mode_t micokit_system_work_state_cur = MICO_KIT_WORK_MODE;
volatile bool system_work_state_changed = false;

static uint8_t  micokit_ext_test_module_cur = MICOKIT_EXT_TEST_START;
static uint8_t  micokit_ext_test_module_pre = MICOKIT_EXT_TEST_START;
static int rgb_led_test_color_value = 0;

static uint8_t oled_test_print_line_cnt = 0;


//---------------------------- user modules functions --------------------------

// Key1 clicked callback: enter work mode (exit from test mode)
void user_key1_clicked_callback(void)
{
  if(MICO_KIT_TEST_MODE == micokit_system_work_state_cur){
    micokit_system_work_state_cur = MICO_KIT_WORK_MODE;
    system_work_state_changed = true;
  }
  else{
  }
  
  return;
}

// Key1 long pressed callback: enter test mode
void user_key1_long_pressed_callback(void)
{
  if(MICO_KIT_WORK_MODE == micokit_system_work_state_cur){
    micokit_ext_test_module_cur = MICOKIT_EXT_TEST_START;
    micokit_ext_test_module_pre = MICOKIT_EXT_TEST_START;
    micokit_system_work_state_cur = MICO_KIT_TEST_MODE;
    system_work_state_changed = true;
  }
  else{
  }
  
  return;
}

// Key2 clicked callback:  change test module in test mode
void user_key2_clicked_callback(void)
{
  if(MICO_KIT_TEST_MODE == micokit_system_work_state_cur){
    micokit_ext_test_module_pre = micokit_ext_test_module_cur;
    micokit_ext_test_module_cur = (micokit_ext_test_module_cur+1)%(MICOKIT_EXT_TEST_END);
  }
  else{
    dc_motor_set(0);  // stop DC Motor in work mode
  }
  return;
}

// Key2 long pressed callback: reset device info from FogCloud
void user_key2_long_pressed_callback(void)
{
  if(MICO_KIT_WORK_MODE == micokit_system_work_state_cur){
    // set cloud reset flag
  }
  else{
  }
  
  return;
}

//------------------------------------- API ------------------------------------
OSStatus user_modules_init(void)
{
  OSStatus err = kUnknownErr;
  
  // init DC Motor(GPIO)
  dc_motor_init();
  dc_motor_set(0);   // off
  
  // init RGB LED(P9813)
  rgb_led_init();
  rgb_led_open(0, 0, 0);  // off
  
  // init OLED
  OLED_Init();
  OLED_Clear();
  OLED_ShowString(20,0,(uint8_t*)DEV_KIT_MANUFACTURER);
  OLED_ShowString(20,3,(uint8_t*)DEV_KIT_NAME);
  OLED_ShowString(16,6,"Starting...");
  
  // init Light sensor(ADC)
  light_sensor_init();
  
  // init infrared sensor(ADC)
  infrared_reflective_init();
  
  // init user key1 && key2
  user_key1_init();
  user_key2_init();
  
  err = kNoErr;
  
  return err;
}

void user_modules_tests(void)
{
  if(system_work_state_changed){  // work mode => test mode
    // set OLED
    OLED_Clear();
    OLED_ShowString(20,0,(uint8_t*)DEV_KIT_MANUFACTURER);
    OLED_ShowString(20,3,(uint8_t*)DEV_KIT_NAME);
    OLED_ShowString(0,6,(uint8_t*)"                ");   // clean line3
      
    // rgb led state init
    rgb_led_test_color_value = 0;
    oled_test_print_line_cnt = 0;
    hsb2rgb_led_open(0,0,0);
    
    // dc motor stop
    dc_motor_set(0);
    
    system_work_state_changed = false;
  }
  
  // stop previous DC Motor test
  if( MICOKIT_EXT_TEST_DC_MOTOR == micokit_ext_test_module_pre){
    dc_motor_set(0);
  }
  
  // stop previous RGB LED test
  if( MICOKIT_EXT_TEST_RGB_LED == micokit_ext_test_module_pre){
    hsb2rgb_led_open(0,0,0);
  }
  
  // stop previous OLED test
  if( MICOKIT_EXT_TEST_OLED == micokit_ext_test_module_pre){
    oled_test_print_line_cnt = 0;
    OLED_Clear();
    OLED_ShowString(20,0,(uint8_t*)DEV_KIT_MANUFACTURER);
    OLED_ShowString(20,3,(uint8_t*)DEV_KIT_NAME);
  }
  
  switch(micokit_ext_test_module_cur){
  case MICOKIT_EXT_TEST_START:
    {
      OLED_ShowString(0,6,(uint8_t*)"                ");   // clean line3
      OLED_ShowString(0,6,"TEST: Press Key2");
      break;
    }
  case MICOKIT_EXT_TEST_OLED:
    {
      // OLED test
      if(0 == oled_test_print_line_cnt){
        OLED_Clear();
        oled_test_print_line_cnt++;
      }
      else if(1 == oled_test_print_line_cnt){
        OLED_ShowString(20,0,(uint8_t*)DEV_KIT_MANUFACTURER);
        oled_test_print_line_cnt++;
      }
      else if(2 == oled_test_print_line_cnt){
        OLED_ShowString(20,3,(uint8_t*)DEV_KIT_NAME);
        oled_test_print_line_cnt++;
      }
      else if(3 == oled_test_print_line_cnt){
        OLED_ShowString(16,6,"TEST:[OLED]");
        oled_test_print_line_cnt = 0;
      }
      break;
    }
  case MICOKIT_EXT_TEST_RGB_LED:
    {
      OLED_ShowString(0,6,(uint8_t*)"                ");   // clean line3
      OLED_ShowString(16,6,"TEST:[RGB_LED]");
      
      // RGB_LED test, R->G->B
      hsb2rgb_led_open(rgb_led_test_color_value,100,50);
      rgb_led_test_color_value += 120;
      if(rgb_led_test_color_value >= 360){
        rgb_led_test_color_value = 0;
      }
      
      break;
    }
  case MICOKIT_EXT_TEST_DC_MOTOR:
    {
      OLED_ShowString(0,6,(uint8_t*)"                ");   // clean line3
      OLED_ShowString(16,6,"TEST:[MOTOR]");
      
      // DC Motor test
      dc_motor_set(1);
      break;
    }
  default:
    {
      break;
    }
  } 
}

OSStatus micokit_ext_init(void)
{
  OSStatus err = kUnknownErr;
  err = user_modules_init();
  
  return err;
}
