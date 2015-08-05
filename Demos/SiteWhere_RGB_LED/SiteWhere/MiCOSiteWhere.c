/**
******************************************************************************
* @file    MiCOFogCloud.c 
* @author  Eshen Wang
* @version V1.0.0
* @date    17-Mar-2015
* @brief   This file contains the implementations of cloud service interfaces 
*          for MICO.
operation
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

#include "mico.h"
#include "MiCOSiteWhere.h"

#define sitewhere_log(M, ...) custom_log("MiCO SiteWhere", M, ##__VA_ARGS__)
#define sitewhere_log_trace() custom_log_trace("MiCO SiteWhere")


/*******************************************************************************
 *                                  DEFINES
 ******************************************************************************/


/*******************************************************************************
 *                                  VARIABLES
 ******************************************************************************/


/*******************************************************************************
 *                                  FUNCTIONS
 ******************************************************************************/

//void fogNotify_WifiStatusHandler(WiFiEvent event, app_context_t * const inContext)
//{
//  sitewhere_trace();
//  (void)inContext;
//  switch (event) {
//  case NOTIFY_STATION_UP:
//    break;
//  case NOTIFY_STATION_DOWN:
//    break;
//  case NOTIFY_AP_UP:
//    break;
//  case NOTIFY_AP_DOWN:
//    break;
//  default:
//    break;
//  }
//  return;
//}

extern void sitewhere_main_thread(void *arg);
extern void mqtt_loop_thread(void *inContext);


/*******************************************************************************
 *                        FogCloud  interfaces init
 ******************************************************************************/

OSStatus MiCOStartSiteWhereService(app_context_t* const inContext)
{
  OSStatus err = kUnknownErr;
  
//  err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED, (void *)fogNotify_WifiStatusHandler, inContext);
//  require_noerr_action(err, exit, 
//                       sitewhere_log("ERROR: mico_system_notify_register (mico_notify_WIFI_STATUS_CHANGED) failed!") );
  
  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "sitewhere", 
                                sitewhere_main_thread, STACK_SIZE_FOGCLOUD_MAIN_THREAD, 
                                inContext );
  require_noerr_action( err, exit, sitewhere_log("ERROR: Unable to start sitewhere thread.") );
  
  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "MQTT Client", 
                                mqtt_loop_thread, 0x800, (void*)inContext->mico_context );
  require_noerr_action( err, exit, sitewhere_log("ERROR: Unable to start the MQTT client thread.") );
  
exit:
  return err;
}
