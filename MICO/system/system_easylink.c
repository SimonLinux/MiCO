/**
******************************************************************************
* @file    mico_system_easylink.c 
* @author  William Xu
* @version V1.0.0
* @date    20-July-2015
* @brief   This file provide the easylink function and FTC server for quick 
*          provisioning and first time configuration.
******************************************************************************
*
*  The MIT License
*  Copyright (c) 2015 MXCHIP Inc.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy 
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights 
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is furnished
*  to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in
*  all copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
*  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************
*/

#include "MICO.h"

#include "StringUtils.h"
#include "HTTPUtils.h"
#include "MDNSUtils.h"
#include "SocketUtils.h"
#include "JSON-C/json.h"
#include "mico_system_config.h"
#include "system.h"

// EasyLink Soft AP mode, HTTP configuration message define
#define kEasyLinkURLAuth          "/auth-setup"

/* Functions define in MiCOConfigDelegate.c, can be customized by developer */
extern void         ConfigWillStart               ( mico_Context_t * const inContext );
extern void         ConfigWillStop                ( mico_Context_t * const inContext );
extern void         ConfigEasyLinkIsSuccess       ( mico_Context_t * const inContext );
extern void         ConfigSoftApWillStart         ( mico_Context_t * const inContext );
extern OSStatus     ConfigELRecvAuthData          ( char * userInfo, mico_Context_t * const inContext );

/* Internal vars and functions */
static mico_semaphore_t   easylink_sem; /**< Used to suspend thread while connecting. */
static bool               easylink_success = false; /**< true: connect to wlan, false: start soft ap mode or roll back to previoude settings */
static uint32_t           easylinkIndentifier = 0; /**< Unique for an easylink instance. */

/* Perform easylink and connect to wlan */
static void easylink_thread(void *inContext);
static OSStatus mico_easylink_bonjour_start( WiFi_Interface interface, mico_Context_t * const inContext );
static OSStatus mico_easylink_bonjour_update( WiFi_Interface interface, mico_Context_t * const inContext );
static void remove_bonjour_for_easylink(void);


/* MiCO callback when WiFi status is changed */
void EasyLinkNotify_WifiStatusHandler(WiFiEvent event, mico_Context_t * const inContext)
{
  system_log_trace();
  require(inContext, exit);

  switch (event) {
  case NOTIFY_STATION_UP:
    /* Connected to AP, means that the wlan configuration is right, update configuration in flash and update 
        bongjour txt record with new "easylinkIndentifier" */
    mico_easylink_bonjour_update( Station, inContext );
    inContext->flashContentInRam.micoSystemConfig.configured = allConfigured;
    MICOUpdateConfiguration(inContext); //Update Flash content
    mico_rtos_set_semaphore(&easylink_sem); //Notify Easylink thread
    break;
  case NOTIFY_AP_DOWN:
    /* Remove bonjour service under soft ap interface */
    bonjour_service_suspend( "_easylink_config._tcp.local.", Soft_AP, true );
    break;
  default:
    break;
  }
exit:
  return;
}

/* MiCO callback when EasyLink is finished step 1, return SSID and KEY */
void EasyLinkNotify_EasyLinkCompleteHandler(network_InitTypeDef_st *nwkpara, mico_Context_t * const inContext)
{
  system_log_trace();
  OSStatus err = kNoErr;
  
  require_action(inContext, exit, err = kParamErr);
  require_action(nwkpara, exit, err = kTimeoutErr);

  /* Store SSID and KEY*/
  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  memcpy(inContext->flashContentInRam.micoSystemConfig.ssid, nwkpara->wifi_ssid, maxSsidLen);
  memset(inContext->flashContentInRam.micoSystemConfig.bssid, 0x0, 6);
  memcpy(inContext->flashContentInRam.micoSystemConfig.user_key, nwkpara->wifi_key, maxKeyLen);
  inContext->flashContentInRam.micoSystemConfig.user_keyLength = strlen(nwkpara->wifi_key);
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
  system_log("Get SSID: %s, Key: %s", inContext->flashContentInRam.micoSystemConfig.ssid, inContext->flashContentInRam.micoSystemConfig.user_key);

exit:
  if( err != kNoErr){
    /*EasyLink timeout or error*/    
    system_log("EasyLink step 1 ERROR, err: %d", err);
    easylink_success = false;
    mico_rtos_set_semaphore(&easylink_sem);    
  }
  return;
}

/* MiCO callback when EasyLink is finished step 2, return extra data 
  data format: AuthData#Identifier(localIp/netMask/gateWay/dnsServer) 
  Auth data: Provide to application, application will decide if this is a proter configuration for currnet device 
  Identifier: Unique id for every easylink instance send by easylink mobile app
  localIp/netMask/gateWay/dnsServer: Device static ip address, use DHCP if not exist 
*/
void EasyLinkNotify_EasyLinkGetExtraDataHandler(int datalen, char* data, mico_Context_t * const inContext)
{
  system_log_trace();
  OSStatus err = kNoErr;
  int index ;
  uint32_t *identifier, ipInfoCount;
  char *debugString;

  require_action(inContext, exit, err = kParamErr);

  debugString = DataToHexStringWithSpaces( (const uint8_t *)data, datalen );
  system_log("Get user info: %s", debugString);
  free(debugString);

  /* Find '#' that seperate anthdata and identifier*/
  for(index = datalen - 1; index>=0; index-- ){
    if(data[index] == '#' &&( (datalen - index) == 5 || (datalen - index) == 25 ) )
      break;
  }
  require_action(index >= 0, exit, err = kParamErr);

  /* Check auth data by device */
  data[index++] = 0x0;
  err = ConfigELRecvAuthData(data, inContext);
  require_noerr(err, exit);

  /* Read identifier */
  identifier = (uint32_t *)&data[index];
  easylinkIndentifier = *identifier;

  /* Identifier: 1 x uint32_t or Identifier/localIp/netMask/gateWay/dnsServer: 5 x uint32_t */
  ipInfoCount = (datalen - index)/sizeof(uint32_t);
  require_action(ipInfoCount >= 1, exit, err = kParamErr);

  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  
  if(ipInfoCount == 1){ //Use DHCP to obtain local ip address
    inContext->flashContentInRam.micoSystemConfig.dhcpEnable = true;
    system_log("Get auth info: %s, EasyLink identifier: %x", data, easylinkIndentifier);
  }else{ //Use static ip address 
    inContext->flashContentInRam.micoSystemConfig.dhcpEnable = false;
    inet_ntoa(inContext->flashContentInRam.micoSystemConfig.localIp, *(uint32_t *)(identifier+1));
    inet_ntoa(inContext->flashContentInRam.micoSystemConfig.netMask, *(uint32_t *)(identifier+2));
    inet_ntoa(inContext->flashContentInRam.micoSystemConfig.gateWay, *(uint32_t *)(identifier+3));
    inet_ntoa(inContext->flashContentInRam.micoSystemConfig.dnsServer, *(uint32_t *)(identifier+4));
    strcpy((char *)inContext->micoStatus.localIp, inContext->flashContentInRam.micoSystemConfig.localIp);
    strcpy((char *)inContext->micoStatus.netMask, inContext->flashContentInRam.micoSystemConfig.netMask);
    strcpy((char *)inContext->micoStatus.gateWay, inContext->flashContentInRam.micoSystemConfig.gateWay);
    strcpy((char *)inContext->micoStatus.dnsServer, inContext->flashContentInRam.micoSystemConfig.dnsServer);

    system_log("Get auth info: %s, EasyLink identifier: %x, local IP info:%s %s %s %s ", data, easylinkIndentifier, inContext->flashContentInRam.micoSystemConfig.localIp,\
    inContext->flashContentInRam.micoSystemConfig.netMask, inContext->flashContentInRam.micoSystemConfig.gateWay,inContext->flashContentInRam.micoSystemConfig.dnsServer);
  }
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);

exit:
  if( err != kNoErr){
    /*EasyLink error*/    
    system_log("EasyLink step 2 ERROR, err: %d", err);
    easylink_success = false;
  }else
    /* Easylink success after step 1 and step 2 */
    easylink_success = true;

  mico_rtos_set_semaphore(&easylink_sem);    
  return;
}

OSStatus system_easylink_start( mico_Context_t * const inContext)
{
  system_log_trace();
  OSStatus err = kUnknownErr;
  mico_thread_t easylink_thread_handler;

  /* Start easylink thread */
  err = mico_rtos_create_thread(&easylink_thread_handler, MICO_APPLICATION_PRIORITY, "EASYLINK", easylink_thread, 0x1000, (void*)inContext );
  require_noerr_string( err, exit, "ERROR: Unable to start the EasyLink thread." );
  //mico_rtos_thread_join(&easylink_thread_handler);

exit:
  return err;
}

void easylink_thread(void *inContext)
{
  system_log_trace();
  OSStatus err = kNoErr;
  mico_Context_t *Context = inContext;
#if ( MICO_CONFIG_MODE == CONFIG_MODE_EASYLINK_WITH_SOFTAP ) || (MICO_CONFIG_MODE == CONFIG_MODE_SOFT_AP)
  network_InitTypeDef_st wNetConfig;
#endif

  easylinkIndentifier = 0x0;
  easylink_success = false;

  /* Register notifications */
  MICOAddNotification( mico_notify_WIFI_STATUS_CHANGED, (void *)EasyLinkNotify_WifiStatusHandler );
  MICOAddNotification( mico_notify_EASYLINK_WPS_COMPLETED, (void *)EasyLinkNotify_EasyLinkCompleteHandler );
  MICOAddNotification( mico_notify_EASYLINK_GET_EXTRA_DATA, (void *)EasyLinkNotify_EasyLinkGetExtraDataHandler );

  mico_rtos_init_semaphore(&easylink_sem, 1);

  /* Start bonjour service for easylink mode */
  err = mico_easylink_bonjour_start( Station, Context );
  require_noerr( err, exit );

  /* Start config server */
#ifdef MICO_CONFIG_SERVER_ENABLE
  err = MICOStartConfigServer( Context );
  require_noerr(err, exit);
#endif

  /* Skip Easylink mode */    
  if(Context->flashContentInRam.micoSystemConfig.easyLinkByPass == EASYLINK_BYPASS){
    Context->flashContentInRam.micoSystemConfig.easyLinkByPass = EASYLINK_BYPASS_NO;
    MICOUpdateConfiguration( Context );
    system_connect_wifi_fast( Context );
    goto exit;
  }
/* If use CONFIG_MODE_SOFT_AP only, skip easylink mode, establish soft ap directly */ 
  ConfigWillStart( Context ); 
restart:
#if ( MICO_CONFIG_MODE != CONFIG_MODE_SOFT_AP ) 
  system_log("Start easylink commbo mode");
  micoWlanStartEasyLinkPlus(EasyLink_TimeOut/1000);

  mico_rtos_get_semaphore(&easylink_sem, MICO_WAIT_FOREVER);
#endif

  /* EasyLink Success */
  if( easylink_success == true ){
    ConfigEasyLinkIsSuccess( Context );
    system_connect_wifi_normal( Context );
    err = mico_rtos_get_semaphore( &easylink_sem, EasyLink_ConnectWlan_Timeout );
    /*SSID or Password is not correct, module cannot connect to wlan, so restart EasyLink again*/
    require_noerr_string( err, restart, "Re-start easylink commbo mode" );
    goto exit;
  }
  /* EasyLink failed */
  else{
#if ( MICO_CONFIG_MODE == CONFIG_MODE_EASYLINK_WITH_SOFTAP ) || (MICO_CONFIG_MODE == CONFIG_MODE_SOFT_AP) //start soft ap mode 
    mico_thread_msleep(20);

    ConfigSoftApWillStart( Context );

    memset(&wNetConfig, 0, sizeof(network_InitTypeDef_st));
    wNetConfig.wifi_mode = Soft_AP;
    snprintf(wNetConfig.wifi_ssid, 32, "EasyLink_%c%c%c%c%c%c", Context->micoStatus.mac[9],  Context->micoStatus.mac[10],
                                                                Context->micoStatus.mac[12], Context->micoStatus.mac[13],
                                                                Context->micoStatus.mac[15], Context->micoStatus.mac[16] );
    strcpy((char*)wNetConfig.wifi_key, "");
    strcpy((char*)wNetConfig.local_ip_addr, "10.10.10.1");
    strcpy((char*)wNetConfig.net_mask, "255.255.255.0");
    strcpy((char*)wNetConfig.gateway_ip_addr, "10.10.10.1");
    wNetConfig.dhcpMode = DHCP_Server;
    micoWlanStart(&wNetConfig);
    system_log("Establish soft ap: %s.....", wNetConfig.wifi_ssid);

    /* Config server is required in soft ap mode */
    err = mico_easylink_bonjour_start( Soft_AP, Context );
    require_noerr( err, exit );

    mico_rtos_get_semaphore(&easylink_sem, MICO_WAIT_FOREVER);
#else // CONFIG_MODE_EASYLINK mode
    /*so roll back to previous settings  (if it has) and connect*/
    if(Context->flashContentInRam.micoSystemConfig.configured != unConfigured){
      Context->flashContentInRam.micoSystemConfig.configured = allConfigured;
      MICOUpdateConfiguration(Context);
      system_connect_wifi_normal( Context );
    }else{
      /*module should power down in default setting*/
      micoWlanPowerOff();
    }
#endif
  }

exit:
  ConfigWillStop( Context );
  SetTimer( 60*1000 , remove_bonjour_for_easylink );
  MICORemoveNotification( mico_notify_WIFI_STATUS_CHANGED, (void *)EasyLinkNotify_WifiStatusHandler );
  MICORemoveNotification( mico_notify_EASYLINK_WPS_COMPLETED, (void *)EasyLinkNotify_EasyLinkCompleteHandler );
  MICORemoveNotification( mico_notify_EASYLINK_GET_EXTRA_DATA, (void *)EasyLinkNotify_EasyLinkGetExtraDataHandler );
  
#ifndef MICO_CONFIG_SERVER_ENABLE
  err = MICOStopConfigServer( );
  require_noerr(err, exit);
#endif 

  mico_rtos_deinit_semaphore( &easylink_sem );
  easylink_sem = NULL;

  mico_rtos_delete_thread( NULL );
  return;
}

OSStatus ConfigIncommingJsonMessageUAP( const char *input, mico_Context_t * const inContext )
{
  OSStatus err = kNoErr;
  json_object *new_obj;
  system_log_trace();
  inContext->flashContentInRam.micoSystemConfig.easyLinkByPass = EASYLINK_BYPASS_NO;

  new_obj = json_tokener_parse(input);
  system_log("Recv config object=%s", input);
  require_action(new_obj, exit, err = kUnknownErr);
  
  json_object_object_foreach(new_obj, key, val) {
    if(!strcmp(key, "SSID")){
      strncpy(inContext->flashContentInRam.micoSystemConfig.ssid, json_object_get_string(val), maxSsidLen);
      inContext->flashContentInRam.micoSystemConfig.channel = 0;
      memset(inContext->flashContentInRam.micoSystemConfig.bssid, 0x0, 6);
      inContext->flashContentInRam.micoSystemConfig.security = SECURITY_TYPE_AUTO;
      memcpy(inContext->flashContentInRam.micoSystemConfig.key, inContext->flashContentInRam.micoSystemConfig.user_key, maxKeyLen);
      inContext->flashContentInRam.micoSystemConfig.keyLength = inContext->flashContentInRam.micoSystemConfig.user_keyLength;
    }else if(!strcmp(key, "PASSWORD")){
      inContext->flashContentInRam.micoSystemConfig.security = SECURITY_TYPE_AUTO;
      strncpy(inContext->flashContentInRam.micoSystemConfig.key, json_object_get_string(val), maxKeyLen);
      strncpy(inContext->flashContentInRam.micoSystemConfig.user_key, json_object_get_string(val), maxKeyLen);
      inContext->flashContentInRam.micoSystemConfig.keyLength = strlen(inContext->flashContentInRam.micoSystemConfig.key);
      inContext->flashContentInRam.micoSystemConfig.user_keyLength = strlen(inContext->flashContentInRam.micoSystemConfig.key);
      memcpy(inContext->flashContentInRam.micoSystemConfig.key, inContext->flashContentInRam.micoSystemConfig.user_key, maxKeyLen);
      inContext->flashContentInRam.micoSystemConfig.keyLength = inContext->flashContentInRam.micoSystemConfig.user_keyLength;
    }else if(!strcmp(key, "DHCP")){
      inContext->flashContentInRam.micoSystemConfig.dhcpEnable   = json_object_get_boolean(val);
    }else if(!strcmp(key, "IDENTIFIER")){
      easylinkIndentifier = (uint32_t)json_object_get_int(val);
    }else if(!strcmp(key, "IP")){
      strncpy(inContext->flashContentInRam.micoSystemConfig.localIp, json_object_get_string(val), maxIpLen);
    }else if(!strcmp(key, "NETMASK")){
      strncpy(inContext->flashContentInRam.micoSystemConfig.netMask, json_object_get_string(val), maxIpLen);
    }else if(!strcmp(key, "GATEWAY")){
      strncpy(inContext->flashContentInRam.micoSystemConfig.gateWay, json_object_get_string(val), maxIpLen);
    }else if(!strcmp(key, "DNS1")){
      strncpy(inContext->flashContentInRam.micoSystemConfig.dnsServer, json_object_get_string(val), maxIpLen);
    }
  }
  json_object_put(new_obj);

exit:

  return err; 
}

static OSStatus mico_easylink_bonjour_start( WiFi_Interface interface, mico_Context_t * const inContext )
{
  char *temp_txt= NULL;
  char *temp_txt2;
  OSStatus err = kNoErr;
  net_para_st para;
  bonjour_init_t init;

  temp_txt = malloc(500);
  require_action(temp_txt, exit, err = kNoMemoryErr);

  memset(&init, 0x0, sizeof(bonjour_init_t));

  micoWlanGetIPStatus(&para, interface);

  init.service_name = "_easylink_config._tcp.local.";

  /*   name(xxxxxx).local.  */
  snprintf( temp_txt, 100, "%s(%c%c%c%c%c%c).local.", MODEL, 
                                                     inContext->micoStatus.mac[9],  inContext->micoStatus.mac[10], \
                                                     inContext->micoStatus.mac[12], inContext->micoStatus.mac[13], \
                                                     inContext->micoStatus.mac[15], inContext->micoStatus.mac[16] );
  init.host_name = (char*)__strdup(temp_txt);

  /*   name(xxxxxx).   */
  snprintf( temp_txt, 100, "%s(%c%c%c%c%c%c)",       MODEL, 
                                                     inContext->micoStatus.mac[9],  inContext->micoStatus.mac[10], \
                                                     inContext->micoStatus.mac[12], inContext->micoStatus.mac[13], \
                                                     inContext->micoStatus.mac[15], inContext->micoStatus.mac[16] );
  init.instance_name = (char*)__strdup(temp_txt);

  init.service_port = MICO_CONFIG_SERVER_PORT;

  temp_txt2 = __strdup_trans_dot(FIRMWARE_REVISION);
  sprintf(temp_txt, "FW=%s.", temp_txt2);
  free(temp_txt2);
  
  temp_txt2 = __strdup_trans_dot(HARDWARE_REVISION);
  sprintf(temp_txt, "%sHD=%s.", temp_txt, temp_txt2);
  free(temp_txt2);

  temp_txt2 = __strdup_trans_dot(PROTOCOL);
  sprintf(temp_txt, "%sPO=%s.", temp_txt, temp_txt2);
  free(temp_txt2);

  temp_txt2 = __strdup_trans_dot(inContext->micoStatus.rf_version);
  sprintf(temp_txt, "%sRF=%s.", temp_txt, temp_txt2);
  free(temp_txt2);

  temp_txt2 = __strdup_trans_dot(inContext->micoStatus.mac);
  sprintf(temp_txt, "%sMAC=%s.", temp_txt, temp_txt2);
  free(temp_txt2);

  temp_txt2 = __strdup_trans_dot(MicoGetVer());
  sprintf(temp_txt, "%sOS=%s.", temp_txt, temp_txt2);
  free(temp_txt2);

  temp_txt2 = __strdup_trans_dot(MODEL);
  sprintf(temp_txt, "%sMD=%s.", temp_txt, temp_txt2);
  free(temp_txt2);

  temp_txt2 = __strdup_trans_dot(MANUFACTURER);
  sprintf(temp_txt, "%sMF=%s.", temp_txt, temp_txt2);
  free(temp_txt2);

#ifdef MICO_CONFIG_SERVER_ENABLE
  sprintf(temp_txt, "%sFTC=T.", temp_txt);
#else
  sprintf(temp_txt, "%sFTC=F.", temp_txt);
#endif
  
  if(interface == Soft_AP)
    sprintf(temp_txt, "%swlan unconfigured=T.", temp_txt);
  else
    sprintf(temp_txt, "%swlan unconfigured=F.", temp_txt);

  sprintf(temp_txt, "%sID=%u.", temp_txt, easylinkIndentifier);
  init.txt_record = (char*)__strdup(temp_txt);

  if(interface == Soft_AP)
    bonjour_service_add(init, interface, 1500);
  else
    bonjour_service_add(init, interface, 10);

  free(init.host_name);
  free(init.instance_name);
  free(init.txt_record);

exit:
  if(temp_txt) free(temp_txt);
  return err;
}

static OSStatus mico_easylink_bonjour_update( WiFi_Interface interface, mico_Context_t * const inContext )
{
  char *temp_txt= NULL;
  char *temp_txt2;
  OSStatus err = kNoErr;

  temp_txt = malloc(500);
  require_action(temp_txt, exit, err = kNoMemoryErr);

  temp_txt2 = __strdup_trans_dot(FIRMWARE_REVISION);
  sprintf(temp_txt, "FW=%s.", temp_txt2);
  free(temp_txt2);
  
  temp_txt2 = __strdup_trans_dot(HARDWARE_REVISION);
  sprintf(temp_txt, "%sHD=%s.", temp_txt, temp_txt2);
  free(temp_txt2);

  temp_txt2 = __strdup_trans_dot(PROTOCOL);
  sprintf(temp_txt, "%sPO=%s.", temp_txt, temp_txt2);
  free(temp_txt2);

  temp_txt2 = __strdup_trans_dot(inContext->micoStatus.rf_version);
  sprintf(temp_txt, "%sRF=%s.", temp_txt, temp_txt2);
  free(temp_txt2);

  temp_txt2 = __strdup_trans_dot(inContext->micoStatus.mac);
  sprintf(temp_txt, "%sMAC=%s.", temp_txt, temp_txt2);
  free(temp_txt2);

  temp_txt2 = __strdup_trans_dot(MicoGetVer());
  sprintf(temp_txt, "%sOS=%s.", temp_txt, temp_txt2);
  free(temp_txt2);

  temp_txt2 = __strdup_trans_dot(MODEL);
  sprintf(temp_txt, "%sMD=%s.", temp_txt, temp_txt2);
  free(temp_txt2);

  temp_txt2 = __strdup_trans_dot(MANUFACTURER);
  sprintf(temp_txt, "%sMF=%s.", temp_txt, temp_txt2);
  free(temp_txt2);

#ifdef MICO_CONFIG_SERVER_ENABLE
  sprintf(temp_txt, "%sFTC=T.", temp_txt);
#else
  sprintf(temp_txt, "%sFTC=F.", temp_txt);
#endif
  
  if(interface == Soft_AP)
    sprintf(temp_txt, "%swlan unconfigured=T.", temp_txt);
  else
    sprintf(temp_txt, "%swlan unconfigured=F.", temp_txt);

  sprintf(temp_txt, "%sID=%x.", temp_txt, easylinkIndentifier);

  bonjour_update_txt_record( "_easylink_config._tcp.local.", interface, temp_txt );

exit:
  if(temp_txt) free(temp_txt);
  return err;
}

static void remove_bonjour_for_easylink(void)
{
  bonjour_service_suspend( "_easylink_config._tcp.local.", Station, true );
}



