/**
  ******************************************************************************
  * @file    micokit_ext.h
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

#ifndef __MICOKIT_EXT_H_
#define __MICOKIT_EXT_H_

#include "drivers/rgb_led.h"
#include "drivers/hsb2rgb_led.h"
#include "drivers/oled.h"
#include "drivers/DHT11.h"
#include "drivers/light_sensor.h"
#include "drivers/infrared_reflective.h"
#include "drivers/dc_motor.h"
#include "drivers/keys.h"

#define DEV_KIT_MANUFACTURER    "MXCHIP"
#define DEV_KIT_NAME            "MiCOKit3288"

typedef enum  {
  MICO_KIT_WORK_MODE = 0,
  MICO_KIT_TEST_MODE
}micokit_system_work_mode_t;


OSStatus micokit_ext_init(void);    // MicoKit extension board init

OSStatus user_modules_init(void);   // init modules on MicoKit extension board
void user_modules_tests(void);      // test modules on MicoKit extension board


#endif  // __MICOKIT_EXT_H_
