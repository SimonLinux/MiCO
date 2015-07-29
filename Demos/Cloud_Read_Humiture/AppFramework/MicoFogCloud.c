/**
******************************************************************************
* @file    MicoFogCloud.c 
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

#include "MicoFogCloud.h"
#include "fogcloud.h"
#include "FogCloudUtils.h"

#define fogcloud_log(M, ...) custom_log("MiCOFogCloud", M, ##__VA_ARGS__)
#define fogcloud_log_trace() custom_log_trace("MicoFogCloud")


/*******************************************************************************
 *                                  DEFINES
 ******************************************************************************/


/*******************************************************************************
 *                                  VARIABLES
 ******************************************************************************/
static volatile bool device_need_delete = false;   // flag to delete device from cloud.

static mico_queue_t msg_recv_queue = NULL;   // fogcloud msg recv queue
static volatile uint32_t total_recv_buf_len = 0;
static mico_mutex_t msg_recv_queue_mutex = NULL;


/*******************************************************************************
 *                                  FUNCTIONS
 ******************************************************************************/
extern OSStatus MicoStartFogCloudConfigServer ( app_context_t * const app_conext );
extern void set_RF_LED_cloud_connected     ( app_context_t * const app_conext );
extern void set_RF_LED_cloud_disconnected  ( app_context_t * const app_conext );

void MicoFogCloudNeedResetDevice(void)
{
  device_need_delete = true;
  return;
}

void fogNotify_WifiStatusHandler(WiFiEvent event, app_context_t * const app_conext)
{
  fogcloud_log_trace();
  (void)app_conext;
  switch (event) {
  case NOTIFY_STATION_UP:
    app_conext->appStatus.isWifiConnected = true;
    break;
  case NOTIFY_STATION_DOWN:
    app_conext->appStatus.isWifiConnected = false;
    break;
  case NOTIFY_AP_UP:
    break;
  case NOTIFY_AP_DOWN:
    break;
  default:
    break;
  }
  return;
}

#ifndef DISABLE_FOGCLOUD_OTA_CHECK
void fogcloud_ota_thread(void *arg)
{
  OSStatus err = kUnknownErr;
  MVDOTARequestData_t devOTARequestData;
  app_context_t *app_conext = (app_context_t *)arg;
  
  fogcloud_log("OTA: check new firmware ...");
  memset((void*)&devOTARequestData, 0, sizeof(devOTARequestData));
  strncpy(devOTARequestData.loginId,
          app_context->appConfig.fogcloudConfig.loginId,
          MAX_SIZE_LOGIN_ID);
  strncpy(devOTARequestData.devPasswd,
          app_context->appConfig.fogcloudConfig.devPasswd,
          MAX_SIZE_DEV_PASSWD);
  strncpy(devOTARequestData.user_token,
          app_context->appConfig.fogcloudConfig.userToken,
          MAX_SIZE_USER_TOKEN);
  err = fogCloudDevFirmwareUpdate(app_conext, devOTARequestData);
  if(kNoErr == err){
    if(app_conext->appStatus.fogcloudStatus.RecvRomFileSize > 0){
      fogcloud_log("OTA: firmware download success, system will reboot && update...");
      // set bootloader to reboot && update app firmware
      mico_rtos_lock_mutex(&app_conext->flashContentInRam_mutex);
      memset(&app_conext->mico_context->flashContentInRam.bootTable, 0, sizeof(boot_table_t));
      app_conext->mico_context->flashContentInRam.bootTable.length = app_conext->appStatus.fogcloudStatus.RecvRomFileSize;
      app_conext->mico_context->flashContentInRam.bootTable.start_address = UPDATE_START_ADDRESS;
      app_conext->mico_context->flashContentInRam.bootTable.type = 'A';
      app_conext->mico_context->flashContentInRam.bootTable.upgrade_type = 'U';
      if(app_conext->mico_context->flashContentInRam.micoSystemConfig.configured != allConfigured)
        app_conext->mico_context->flashContentInRam.micoSystemConfig.easyLinkByPass = EASYLINK_SOFT_AP_BYPASS;
      MICOUpdateConfiguration(app_conext);
      mico_rtos_unlock_mutex(&app_conext->mico_context->flashContentInRam_mutex);
      app_conext->mico_context->micoStatus.sys_state = eState_Software_Reset;
      if(app_conext->mico_context->micoStatus.sys_state_change_sem != NULL ){
        mico_rtos_set_semaphore(&app_conext->mico_context->micoStatus.sys_state_change_sem);
      }
      mico_thread_sleep(MICO_WAIT_FOREVER);
    }
    else{
      fogcloud_log("OTA: firmware is up-to-date!");
    }
  }
  else{
    fogcloud_log("OTA: firmware download failed, err=%d", err);
  }
  
  fogcloud_log("fogcloud_ota_thread exit err=%d.", err);
  mico_rtos_delete_thread(NULL);
  return;
}
#endif   // DISABLE_FOGCLOUD_OTA_CHECK

void fogcloud_main_thread(void *arg)
{
  OSStatus err = kUnknownErr;
  app_context_t *app_conext = (app_context_t *)arg;
  
  MVDResetRequestData_t devResetRequestData;
  
#ifdef ENABLE_FOGCLOUD_AUTO_ACTIVATE
  MVDActivateRequestData_t devDefaultActivateData;
#endif
  
  /* wait for station on */
  while(!app_conext->appStatus.isWifiConnected){
    mico_thread_msleep(500);
  }
  
  //--- create msg recv queue, NOTE: just push msg pionter into queue, so msg memory must be freed after used.
  if(NULL == msg_recv_queue_mutex){
    err = mico_rtos_init_mutex(&msg_recv_queue_mutex);
    require_noerr_action(err, exit,
                         fogcloud_log("ERROR: mico_rtos_init_mutex (msg_recv_queue_mutex) failed, err=%d.", err));
  }
  err = mico_rtos_init_queue(&msg_recv_queue, "fog_recv_queue", sizeof(int), FOGCLOUD_MAX_RECV_QUEUE_LENGTH);
  require_noerr_action(err, exit,
                       fogcloud_log("ERROR: mico_rtos_init_queue (msg_recv_queue) failed, err=%d", err));
  
  /* start FogCloud service */
  err = fogCloudStart(app_conext);
  require_noerr_action(err, exit, 
                       fogcloud_log("ERROR: MicoFogCloudCloudInterfaceStart failed!") );
  
  /* start configServer for fogcloud (server for activate/authorize/reset/ota cmd from user APP) */
  if(false == app_conext->appConfig.fogcloudConfig.owner_binding){
    err = MicoStartFogCloudConfigServer( app_conext);
    require_noerr_action(err, exit, 
                         fogcloud_log("ERROR: start FogCloud configServer failed!") );
  }
  
 #ifdef ENABLE_FOGCLOUD_AUTO_ACTIVATE
  /* activate when wifi on */
  while(false == app_conext->appConfig.fogcloudConfig.isActivated){
    // auto activate, using default login_id/dev_pass/user_token
    fogcloud_log("device activate start...");
    memset((void*)&devDefaultActivateData, 0, sizeof(devDefaultActivateData));
    strncpy(devDefaultActivateData.loginId,
            app_conext->appConfig.fogcloudConfig.loginId,
            MAX_SIZE_LOGIN_ID);
    strncpy(devDefaultActivateData.devPasswd,
            app_conext->appConfig.fogcloudConfig.devPasswd,
            MAX_SIZE_DEV_PASSWD);
    strncpy(devDefaultActivateData.user_token,
            app_conext->mico_context->micoStatus.mac,   // use MAC as default user_token
            MAX_SIZE_USER_TOKEN);
    err = fogCloudDevActivate(app_conext, devDefaultActivateData);
    if(kNoErr == err){
      fogcloud_log("device activate success!");
      break;
    }
    else{
      fogcloud_log("device auto activate failed, err = %d, will retry in 3s ...", err);
    }
    mico_thread_sleep(3);
  }

#endif  // ENABLE_FOGCLOUD_AUTO_ACTIVATE
  
#ifndef DISABLE_FOGCLOUD_OTA_CHECK
  /* OTA check just device activated */
  if( (!app_conext->appStatus.noOTACheckOnSystemStart) && 
     (app_conext->appConfig.fogcloudConfig.isActivated) ){
    // start ota thread
    err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "fogcloud_ota", 
                                  fogcloud_ota_thread, STACK_SIZE_FOGCLOUD_OTA_THREAD, 
                                  app_conext);
    if(kNoErr != err){
      fogcloud_log("ERROR: start FogCloud OTA thread failed, err=%d.", err);
    }
  }
  app_conext->appStatus.noOTACheckOnSystemStart = false;
#endif  // DISABLE_FOGCLOUD_OTA_CHECK
  
  while(1){
    // device info reset
    if(device_need_delete){
      fogcloud_log("delete device from cloud ...");
      memset((void*)&devResetRequestData, 0, sizeof(devResetRequestData));
      strncpy(devResetRequestData.loginId,
              app_conext->appConfig.fogcloudConfig.loginId,
              MAX_SIZE_LOGIN_ID);
      strncpy(devResetRequestData.devPasswd,
              app_conext->appConfig.fogcloudConfig.devPasswd,
              MAX_SIZE_DEV_PASSWD);
      strncpy(devResetRequestData.user_token,
              app_conext->appConfig.fogcloudConfig.userToken,
                MAX_SIZE_USER_TOKEN);
      err = fogCloudResetCloudDevInfo(app_conext, devResetRequestData);
      if(kNoErr == err){
        device_need_delete = false;
        fogcloud_log("delete device success, system need reboot...");
        mico_rtos_lock_mutex(&app_conext->mico_context->flashContentInRam_mutex);
        MicoFogCloudRestoreDefault(app_conext);
        MICOUpdateConfiguration(app_conext);
        mico_rtos_unlock_mutex(&app_conext->mico_context->flashContentInRam_mutex);
        // system restart
        app_conext->micoStatus.sys_state = eState_Software_Reset;
        if(app_conext->micoStatus.sys_state_change_sem){
          mico_rtos_set_semaphore(&app_conext->micoStatus.sys_state_change_sem);
        }
      }
      else{
        fogcloud_log("delete device failed, err = %d.", err);
      }
    }
    
    mico_thread_sleep(1);
    if(app_conext->appStatus.fogcloudStatus.isOTAInProgress){
      continue;  // ota is in progress, the oled && system led will be holding
    }
    
    if(app_conext->appStatus.fogcloudStatus.isCloudConnected){
      set_RF_LED_cloud_connected(app_conext);  // toggle LED
    }
    else{
      set_RF_LED_cloud_disconnected(app_conext);  // stop LED blink
    }
  }
  
exit:
  fogcloud_log("fogcloud_main_thread exit err=%d.", err);
  if(NULL != msg_recv_queue_mutex){
    mico_rtos_deinit_mutex(&msg_recv_queue_mutex);
  }
  if(NULL != msg_recv_queue){
    mico_rtos_deinit_queue(&msg_recv_queue);
  }
  mico_rtos_delete_thread(NULL);
  return;
}


/*******************************************************************************
 *                        FogCloud  interfaces init
 ******************************************************************************/
// reset default value
void MicoFogCloudRestoreDefault(app_context_t* app_conext)
{
  // reset all MicoFogCloud config params
  memset((void*)&(app_conext->appConfig.fogcloudConfig), 
         0, sizeof(fogcloud_config_t));
  
  app_conext->appConfig.fogcloudConfig.isActivated = false;
  app_conext->appConfig.fogcloudConfig.owner_binding = false;
  app_conext->appConfig.fogcloudConfig.deviceId, DEFAULT_DEVICE_ID);
  app_conext->appConfig.fogcloudConfig.masterDeviceKey, DEFAULT_DEVICE_KEY);
  app_conext->appConfig.fogcloudConfig.romVersion, DEFAULT_ROM_VERSION);
  
  sprintf(app_conext->appConfig.fogcloudConfig.loginId, DEFAULT_LOGIN_ID);
  sprintf(app_conext->appConfig.fogcloudConfig.devPasswd, DEFAULT_DEV_PASSWD);
  sprintf(app_conext->appConfig.fogcloudConfig.userToken, context->micoStatus.mac);
}

OSStatus MicoStartFogCloudService(app_context_t* const app_conext)
{
  OSStatus err = kUnknownErr;
  
  //init MicoFogCloud status
  app_conext->appStatus.fogcloudStatus.isCloudConnected = false;
  app_conext->appStatus.fogcloudStatus.RecvRomFileSize = 0;
  app_conext->appStatus.fogcloudStatus.isActivated = app_conext->mico_context->flashContentInRam.appConfig.fogcloudConfig.isActivated;
  app_conext->appStatus.fogcloudStatus.isOTAInProgress = false;
  
  //init fogcloud service interface
  err = fogCloudInit(app_conext->mico_context);
  require_noerr_action(err, exit, 
                       fogcloud_log("ERROR: FogCloud interface init failed!") );
  
  err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED, (void *)fogNotify_WifiStatusHandler, app_conext);
  require_noerr_action(err, exit, 
                       fogcloud_log("ERROR: MICOAddNotification (mico_notify_WIFI_STATUS_CHANGED) failed!") );
  
  // start MicoFogCloud main thread (dev reset && ota check, then start fogcloud service)
  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "fog_main", 
                                fogcloud_main_thread, STACK_SIZE_FOGCLOUD_MAIN_THREAD, 
                                app_conext );
  
exit:
  return err;
}


/*******************************************************************************
*                            MicoFogCloud get state
*******************************************************************************/
// cloud connect state
bool MicoFogCloudIsConnect(app_context_t* const context)
{
  if(NULL == context){
    return false;
  }
  return context->appStatus.fogcloudStatus.isCloudConnected;
}

// device activate state
bool MicoFogCloudIsActivated(app_context_t* const context)
{
  if(NULL == context){
    return false;
  }
  return context->mico_context->flashConentInRam.appConfig.fogcloudConfig.isActivated;
}


/*******************************************************************************
*                           FogCloud control interfaces
******************************************************************************/
//activate
OSStatus MicoFogCloudActivate(app_context_t* const context, 
                              MVDActivateRequestData_t activateData)
{
  OSStatus err = kUnknownErr;

  err = fogCloudDevActivate(context->mico_context, activateData);
  require_noerr_action(err, exit, 
                       fogcloud_log("ERROR: device activate failed! err=%d", err) );
  return kNoErr;
  
exit:
  return err;
}

//authorize
OSStatus MicoFogCloudAuthorize(app_context_t* const context,
                               MVDAuthorizeRequestData_t authorizeData)
{
  OSStatus err = kUnknownErr;
  app_context_t *app_conext = context;
  
  if(context->mico_context->flashContentInRam.appConfig.fogcloudConfig.isActivated){
    err = fogCloudDevAuthorize(app_conext->mico_context, authorizeData);
    require_noerr_action(err, exit, 
                         fogcloud_log("ERROR: device authorize failed! err=%d", err) );
  }
  else{
    fogcloud_log("ERROR: device not activate!");
    err = kStateErr;
  }
  
exit:
  return err;
}

//OTA
OSStatus MicoFogCloudFirmwareUpdate(app_context_t* const context,
                                    MVDOTARequestData_t OTAData)
{
  OSStatus err = kUnknownErr;
  app_context_t *app_conext = context;
  
  err = fogCloudDevFirmwareUpdate(app_conext->mico_context, OTAData);
  require_noerr_action(err, exit, 
                       fogcloud_log("ERROR: Firmware Update error! err=%d", err) );
  return kNoErr;
  
exit:
  return err;
}

//reset device info on cloud
OSStatus MicoFogCloudResetCloudDevInfo(app_context_t* const context,
                                       MVDResetRequestData_t devResetData)
{
  OSStatus err = kUnknownErr;
  app_context_t *app_conext = context;
  
  err = fogCloudResetCloudDevInfo(app_conext->mico_context, devResetData);
  require_noerr_action(err, exit, 
                       fogcloud_log("ERROR: reset device info on cloud error! err=%d", err) );
  return kNoErr;
  
exit:
  return err;
}

//get state of the device( e.g. isActivate/isConnected)
OSStatus MicoFogCloudGetState(app_context_t* const context,
                              MVDGetStateRequestData_t getStateRequestData,
                              void* outDevState)
{
  //OSStatus err = kUnknownErr;
  app_context_t *app_conext = context;
  json_object* report = (json_object*)outDevState;
  
  if((NULL == context) || (NULL == outDevState)){
    return kParamErr;
  }
  
  json_object_object_add(report, "isActivated",
                         json_object_new_boolean(app_conext->mico_context->flashContentInRam.appConfig.fogcloudConfig.isActivated)); 
    json_object_object_add(report, "isBinding",
                         json_object_new_boolean(app_conext->mico_context->flashContentInRam.appConfig.fogcloudConfig.owner_binding)); 
  json_object_object_add(report, "isConnected",
                         json_object_new_boolean(app_conext->appStatus.fogcloudStatus.isCloudConnected));
  json_object_object_add(report, "version",
                         json_object_new_string(app_conext->mico_context->flashContentInRam.appConfig.fogcloudConfig.romVersion));
  
  return kNoErr;
}


/*******************************************************************************
*                       MicoFogCloud message send interface
******************************************************************************/
// MCU => Cloud
// if topic is NULL, send to default topic: device_id/out,
// else send to sub-channel: device_id/out/<topic>
OSStatus MicoFogCloudMsgSend(app_context_t* const context, const char* topic, 
                                   unsigned char *inBuf, unsigned int inBufLen)
{
  fogcloud_log_trace();
  OSStatus err = kUnknownErr;
  
  err = fogCloudSendtoChannel(topic, inBuf, inBufLen);  // transfer raw data
  require_noerr_action( err, exit, fogcloud_log("ERROR: send to cloud error! err=%d", err) );
  return kNoErr;
  
exit:
  return err;
}


/*******************************************************************************
* MicoFogCloud message exchange: push message into recv queue
******************************************************************************/
// handle cloud msg here, for example: send to USART or echo to cloud
OSStatus MicoFogCloudCloudMsgProcess(app_context_t* context, 
                                     const char* topic, const unsigned int topicLen,
                                     unsigned char *inBuf, unsigned int inBufLen)
{
  fogcloud_log_trace();
  OSStatus err = kUnknownErr;
  uint32_t real_msg_len = 0;

  // push msg into queue
  fogcloud_msg_t *real_msg;
  fogcloud_msg_t *p_tem_msg = NULL;  // msg pointer
  
  real_msg_len = sizeof(fogcloud_msg_t) + topicLen + inBufLen;
  if(FOGCLOUD_TOTAL_BUF_LENGTH < (total_recv_buf_len + real_msg_len)){
    return kNoMemoryErr;
  }
  real_msg = (fogcloud_msg_t*)malloc(real_msg_len);
  if (real_msg == NULL){
    return kNoMemoryErr;
  }
  total_recv_buf_len += real_msg_len;
  
	memset(real_msg, '\0', sizeof(real_msg));
  real_msg->topic_len = topicLen;
  real_msg->data_len = inBufLen;
  memcpy(real_msg->data, topic, topicLen);
  memcpy(real_msg->data + topicLen, inBuf, inBufLen);
  //memcpy(real_msg->data + topicLen + inBufLen, '\0', 1);
  
  if(NULL != msg_recv_queue){
    mico_rtos_lock_mutex(&msg_recv_queue_mutex);
    if(mico_rtos_is_queue_full(&msg_recv_queue)){
      fogcloud_log("WARNGING: FogCloud msg overrun, abandon old messages!");
      err = mico_rtos_pop_from_queue(&msg_recv_queue, &p_tem_msg, 0);  // just pop old msg pointer from queue 
      if(kNoErr == err){
        if(NULL != p_tem_msg){  // delete msg, free msg memory
          free(p_tem_msg);
          p_tem_msg = NULL;
        }
      }
      else{
        mico_rtos_unlock_mutex(&msg_recv_queue_mutex);
        fogcloud_log("WARNGING: FogCloud msg overrun, abandon current message!");
        if(NULL != real_msg){  // queue full, new msg abandoned
          free(real_msg);
          real_msg = NULL;
        }
        return kOverrunErr;
      }
    }
    // insert a msg into recv queue
    if (kNoErr != mico_rtos_push_to_queue(&msg_recv_queue, &real_msg, 0)) {  // just push msg pointer in queue
      free(real_msg);
      real_msg = NULL;
      err = kWriteErr;
    }
    else{
      err = kNoErr;
    }
    mico_rtos_unlock_mutex(&msg_recv_queue_mutex);
  }
  else{
    if(NULL != real_msg){
      free(real_msg);
      real_msg = NULL;
    }
    return kNotInitializedErr;
  }
  
  return err;
}

// recv msg from queue
OSStatus MicoFogCloudMsgRecv(app_context_t* const context, fogcloud_msg_t **msg, uint32_t timeout_ms)
{
  fogcloud_log_trace();
  OSStatus err = kUnknownErr;
  
  if(NULL == msg){
    return kParamErr;
  }
     
  if(NULL != msg_recv_queue){
    mico_rtos_lock_mutex(&msg_recv_queue_mutex);
    if(mico_rtos_is_queue_empty(&msg_recv_queue)){
      mico_rtos_unlock_mutex(&msg_recv_queue_mutex);
      return kUnderrunErr;
    }
    err = mico_rtos_pop_from_queue(&msg_recv_queue, msg, timeout_ms);  // just pop msg pointer from queue
    mico_rtos_unlock_mutex(&msg_recv_queue_mutex);
    if(kNoErr == err){
      total_recv_buf_len -= (sizeof(fogcloud_msg_t) - 1 + (*msg)->topic_len + (*msg)->data_len);
    }
  }
  else{
    err = kNotInitializedErr;
  }
  
  return err;
}
