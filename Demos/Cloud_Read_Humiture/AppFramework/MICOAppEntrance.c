/**
  ******************************************************************************
  * @file    MICOAppEntrance.c 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   Mico application entrance, addd user application functons and threads.
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

#include "MICO.h"
#include "MICOAppDefine.h"
#include "MicoFogCloud.h"

#define app_log(M, ...) custom_log("APP", M, ##__VA_ARGS__)
#define app_log_trace() custom_log_trace("APP")


/* default user_main callback function, this must be override by user. */
WEAK OSStatus user_main( app_context_t * const app_context )
{
  //app_log("ERROR: user_main undefined!");
  return kNotHandledErr;
}

/* default user_main callback function, this may be override by user. */
WEAK void userRestoreDefault_callback(application_config_t* appConfig)
{
  //app_log("INFO: call default userRestoreDefault_callback, do nothing!");  // log in ISR may cause error
}

/* user main thread created by MICO APP thread */
void user_main_thread(void* arg)
{
  OSStatus err = kUnknownErr;
  app_context_t* app_context = (app_context_t *)arg;
  
  // loop in user mian function && must not return
  err = user_main(app_context);
  
  // never get here only if user work error.
  app_log("ERROR: user_main thread exit err = %d, system will reboot...", err);
  err = mico_system_power_perform(app_context->mico_context, eState_Software_Reset);
  UNUSED_PARAMETER(err);
  
  mico_rtos_delete_thread(NULL);
}

OSStatus startUserMainThread(app_context_t *app_context)
{
  app_log_trace();
  OSStatus err = kNoErr;
  require_action(app_context, exit, err = kParamErr);
  
  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "user_main", 
                                user_main_thread, STACK_SIZE_USER_MAIN_THREAD, 
                                app_context );
exit:
  return err;
}

/* MICO system callback: Restore default configuration provided by application */
void appRestoreDefault_callback(void * const user_config_data, uint32_t size)
{
  UNUSED_PARAMETER(size);
  application_config_t* appConfig = user_config_data;
  
  appConfig->configDataVer = CONFIGURATION_VERSION;
  appConfig->bonjourServicePort = BONJOUR_SERVICE_PORT;
  
  // restore fogcloud config
  MicoFogCloudRestoreDefault(appConfig);
  // restore user config
  userRestoreDefault_callback(appConfig);
}

int application_start(void)
{
  app_log_trace();
  OSStatus err = kNoErr;
  app_context_t* app_context;
  mico_Context_t* mico_context;
  LinkStatusTypeDef wifi_link_status;

  /* Create application context */
  app_context = ( app_context_t *)calloc(1, sizeof(app_context_t) );
  require_action( app_context, exit, err = kNoMemoryErr );

  /* Create mico system context and read application's config data from flash */
  mico_context = mico_system_context_init( sizeof( application_config_t) );
  app_context->appConfig = mico_system_context_get_user_data( mico_context );
  app_context->mico_context = mico_context;

  /* mico system initialize */
  err = mico_system_init( mico_context );
  require_noerr( err, exit );

  /* Bonjour for service searching */
  MICOStartBonjourService( Station, app_context );
  
  // close system led now, wiil be turned on after Wi-Fi connected.
  MicoSysLed(false);
  
  // check wifi link status
  do{
    err = micoWlanGetLinkStatus(&wifi_link_status);
    if(kNoErr != err){
      mico_thread_sleep(3);
    }
  }while(kNoErr != err);
  
  if(1 ==  wifi_link_status.is_connected){
    mico_context->appStatus.isWifiConnected = true;
  }
  else{
    mico_context->appStatus.isWifiConnected = false;
  }
  
  /* start cloud service */
#if (MICO_CLOUD_TYPE == CLOUD_FOGCLOUD)
  app_log("MICO CloudService: FogCloud.");
  err = MicoStartFogCloudService( app_context );
  require_noerr_action( err, exit, app_log("ERROR: Unable to start FogCloud service.") );
#elif (MICO_CLOUD_TYPE == CLOUD_ALINK)
  app_log("MICO CloudService: Alink.");
#elif (MICO_CLOUD_TYPE == CLOUD_DISABLED)
  app_log("MICO CloudService: disabled.");
#else
  #error "MICO cloud service type is not defined"?
#endif
  
  /* start user thread */
  err = startUserMainThread( app_context );
  require_noerr_action( err, exit, app_log("ERROR: start user_main thread failed!") );

exit:
  mico_rtos_delete_thread(NULL);
  return err;
}
