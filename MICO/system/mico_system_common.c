/**
******************************************************************************
* @file    EasyLink.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide the easylink function and FTC server for quick 
*          provisioning and first time configuration.
******************************************************************************
*
*  The MIT License
*  Copyright (c) 2014 MXCHIP Inc.
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
#include "MDNSUtils.h"
#include "StringUtils.h"
#include "mico_system_context.h"
#include "MICONotificationCenter.h"
#include "mico_system.h"
#include "time.h"

void system_version(char *str, int len)
{
  snprintf( str, len, "%s, build at %s %s", APP_INFO, __TIME__, __DATE__);
}

static void micoNotify_DHCPCompleteHandler(IPStatusTypedef *pnet, mico_Context_t * const inContext)
{
  system_log_trace();
  require(inContext, exit);
  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  strcpy((char *)inContext->micoStatus.localIp, pnet->ip);
  strcpy((char *)inContext->micoStatus.netMask, pnet->mask);
  strcpy((char *)inContext->micoStatus.gateWay, pnet->gate);
  strcpy((char *)inContext->micoStatus.dnsServer, pnet->dns);
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
exit:
  return;
}

static void micoNotify_ConnectFailedHandler(OSStatus err, mico_Context_t * const inContext)
{
  system_log_trace();
  (void)inContext;
  system_log("Wlan Connection Err %d", err);
}

static void micoNotify_WlanFatalErrHandler(mico_Context_t * const inContext)
{
  system_log_trace();
  (void)inContext;
  system_log("Wlan Fatal Err!");
  MicoSystemReboot();
}

void micoNotify_StackOverflowErrHandler(char *taskname, mico_Context_t * const inContext)
{
  system_log_trace();
  (void)inContext;
  system_log("Thread %s overflow, system rebooting", taskname);
  MicoSystemReboot();
}

void micoNotify_WifiStatusHandler(WiFiEvent event, mico_Context_t * const inContext)
{
  system_log_trace();
  (void)inContext;
  switch (event) {
  case NOTIFY_STATION_UP:
    system_log("Station up");
    MicoRfLed(true);
    break;
  case NOTIFY_STATION_DOWN:
    system_log("Station down");
    MicoRfLed(false);
    break;
  case NOTIFY_AP_UP:
    system_log("uAP established");
    MicoRfLed(true);
    break;
  case NOTIFY_AP_DOWN:
    system_log("uAP deleted");
    MicoRfLed(false);
    break;
  default:
    break;
  }
  return;
}

void micoNotify_WiFIParaChangedHandler(apinfo_adv_t *ap_info, char *key, int key_len, mico_Context_t * const inContext)
{
  system_log_trace();
  bool _needsUpdate = false;
  require(inContext, exit);
  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  if(strncmp(inContext->flashContentInRam.micoSystemConfig.ssid, ap_info->ssid, maxSsidLen)!=0){
    strncpy(inContext->flashContentInRam.micoSystemConfig.ssid, ap_info->ssid, maxSsidLen);
    _needsUpdate = true;
  }

  if(memcmp(inContext->flashContentInRam.micoSystemConfig.bssid, ap_info->bssid, 6)!=0){
    memcpy(inContext->flashContentInRam.micoSystemConfig.bssid, ap_info->bssid, 6);
    _needsUpdate = true;
  }

  if(inContext->flashContentInRam.micoSystemConfig.channel != ap_info->channel){
    inContext->flashContentInRam.micoSystemConfig.channel = ap_info->channel;
    _needsUpdate = true;
  }
  
  if(inContext->flashContentInRam.micoSystemConfig.security != ap_info->security){
    inContext->flashContentInRam.micoSystemConfig.security = ap_info->security;
    _needsUpdate = true;
  }

  if(memcmp(inContext->flashContentInRam.micoSystemConfig.key, key, maxKeyLen)!=0){
    memcpy(inContext->flashContentInRam.micoSystemConfig.key, key, maxKeyLen);
    _needsUpdate = true;
  }

  if(inContext->flashContentInRam.micoSystemConfig.keyLength != key_len){
    inContext->flashContentInRam.micoSystemConfig.keyLength = key_len;
    _needsUpdate = true;
  }

  if(_needsUpdate== true)  
    MICOUpdateConfiguration(inContext);
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
  
exit:
  return;
}

OSStatus system_notification_init( mico_Context_t * const inContext)
{
  OSStatus err = kNoErr;

  err = MICOInitNotificationCenter( inContext );
  require_noerr( err, exit ); 

  err = MICOAddNotification( mico_notify_WIFI_CONNECT_FAILED, (void *)micoNotify_ConnectFailedHandler );
  require_noerr( err, exit );

  err = MICOAddNotification( mico_notify_WIFI_Fatal_ERROR, (void *)micoNotify_WlanFatalErrHandler );
  require_noerr( err, exit ); 

  err = MICOAddNotification( mico_notify_Stack_Overflow_ERROR, (void *)micoNotify_StackOverflowErrHandler );
  require_noerr( err, exit );

  err = MICOAddNotification( mico_notify_DHCP_COMPLETED, (void *)micoNotify_DHCPCompleteHandler );
  require_noerr( err, exit ); 

  err = MICOAddNotification( mico_notify_WIFI_STATUS_CHANGED, (void *)micoNotify_WifiStatusHandler );
  require_noerr( err, exit );

  err = MICOAddNotification( mico_notify_WiFI_PARA_CHANGED, (void *)micoNotify_WiFIParaChangedHandler );
  require_noerr( err, exit ); 

exit:
  return err;
}

void system_connect_wifi_normal( mico_Context_t * const inContext)
{
  system_log_trace();
  network_InitTypeDef_adv_st wNetConfig;
  memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_adv_st));
  
  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  strncpy((char*)wNetConfig.ap_info.ssid, inContext->flashContentInRam.micoSystemConfig.ssid, maxSsidLen);
  wNetConfig.ap_info.security = SECURITY_TYPE_AUTO;
  memcpy(wNetConfig.key, inContext->flashContentInRam.micoSystemConfig.user_key, maxKeyLen);
  wNetConfig.key_len = inContext->flashContentInRam.micoSystemConfig.user_keyLength;
  wNetConfig.dhcpMode = inContext->flashContentInRam.micoSystemConfig.dhcpEnable;
  strncpy((char*)wNetConfig.local_ip_addr, inContext->flashContentInRam.micoSystemConfig.localIp, maxIpLen);
  strncpy((char*)wNetConfig.net_mask, inContext->flashContentInRam.micoSystemConfig.netMask, maxIpLen);
  strncpy((char*)wNetConfig.gateway_ip_addr, inContext->flashContentInRam.micoSystemConfig.gateWay, maxIpLen);
  strncpy((char*)wNetConfig.dnsServer_ip_addr, inContext->flashContentInRam.micoSystemConfig.dnsServer, maxIpLen);
  wNetConfig.wifi_retry_interval = 100;
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);

  system_log("connect to %s.....", wNetConfig.ap_info.ssid);
  micoWlanStartAdv(&wNetConfig);
}

void system_connect_wifi_fast( mico_Context_t * const inContext)
{
  system_log_trace();
  network_InitTypeDef_adv_st wNetConfig;
  memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_adv_st));
  
  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  strncpy((char*)wNetConfig.ap_info.ssid, inContext->flashContentInRam.micoSystemConfig.ssid, maxSsidLen);
  memcpy(wNetConfig.ap_info.bssid, inContext->flashContentInRam.micoSystemConfig.bssid, 6);
  wNetConfig.ap_info.channel = inContext->flashContentInRam.micoSystemConfig.channel;
  wNetConfig.ap_info.security = inContext->flashContentInRam.micoSystemConfig.security;
  memcpy(wNetConfig.key, inContext->flashContentInRam.micoSystemConfig.key, inContext->flashContentInRam.micoSystemConfig.keyLength);
  wNetConfig.key_len = inContext->flashContentInRam.micoSystemConfig.keyLength;
  if(inContext->flashContentInRam.micoSystemConfig.dhcpEnable == true)
    wNetConfig.dhcpMode = DHCP_Client;
  else
    wNetConfig.dhcpMode = DHCP_Disable;
  strncpy((char*)wNetConfig.local_ip_addr, inContext->flashContentInRam.micoSystemConfig.localIp, maxIpLen);
  strncpy((char*)wNetConfig.net_mask, inContext->flashContentInRam.micoSystemConfig.netMask, maxIpLen);
  strncpy((char*)wNetConfig.gateway_ip_addr, inContext->flashContentInRam.micoSystemConfig.gateWay, maxIpLen);
  strncpy((char*)wNetConfig.dnsServer_ip_addr, inContext->flashContentInRam.micoSystemConfig.dnsServer, maxIpLen);
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);

  wNetConfig.wifi_retry_interval = 100;
  system_log("Connect to %s.....", wNetConfig.ap_info.ssid);
  micoWlanStartAdv(&wNetConfig);
}

static  mico_Context_t* context = NULL;

OSStatus system_context_init( mico_Context_t** out_context )
{
  OSStatus err = kNoErr;

  if( context !=  NULL) {
    free( context );
    context = NULL;
  }

  /*Read current configurations*/
  context = ( mico_Context_t *)malloc(sizeof(mico_Context_t) );
  require_action( context, exit, err = kNoMemoryErr );
  memset(context, 0x0, sizeof(mico_Context_t));
  mico_rtos_init_mutex(&context->flashContentInRam_mutex);
  MICOReadConfiguration( context );  
  *out_context = context;  

exit:
  return err;  
}


OSStatus system_context_read( mico_Context_t** out_context )
{
  OSStatus err = kNoErr;
  require_action( context, exit, err = kNotPreparedErr );
  *out_context = context;

exit:
  return err;
} 

OSStatus system_network_daemen_start( mico_Context_t * const inContext )
{
  IPStatusTypedef para;

  wifimgr_debug_enable(true);
  MicoInit();
  MicoSysLed(true);
  system_log("Free memory %d bytes", MicoGetMemoryInfo()->free_memory); 
  micoWlanGetIPStatus(&para, Station);
  formatMACAddr(inContext->micoStatus.mac, (char *)&para.mac);
  MicoGetRfVer(inContext->micoStatus.rf_version, sizeof(inContext->micoStatus.rf_version));
  system_log("%s mxchipWNet library version: %s", APP_INFO, MicoGetVer());
  system_log("Wi-Fi driver version %s, mac %s", inContext->micoStatus.rf_version, inContext->micoStatus.mac);

  if(inContext->flashContentInRam.micoSystemConfig.rfPowerSaveEnable == true){
    micoWlanEnablePowerSave();
  }

  if(inContext->flashContentInRam.micoSystemConfig.mcuPowerSaveEnable == true){
    MicoMcuPowerSaveConfig(true);
  }  
  
  return kNoErr;
}


OSStatus system_current_time_get( struct tm* time )
{
  mico_rtc_time_t mico_time;
  /*Read current time from RTC.*/
  if( MicoRtcGetTime(&mico_time) == kNoErr ){
    time->tm_sec = mico_time.sec;
    time->tm_min = mico_time.min;
    time->tm_hour = mico_time.hr;
    time->tm_mday = mico_time.date;
    time->tm_wday = mico_time.weekday;
    time->tm_mon = mico_time.month - 1;
    time->tm_year = mico_time.year + 100;
    //system_log("Current Time: %s",asctime(&currentTime));
    return kNoErr;
  }else
    return kGeneralErr;
    //system_log("RTC function unsupported");
}


OSStatus mico_system_current_time_get( struct tm* time )
{
  return system_current_time_get( time );
}

void mico_system_power_perform( mico_system_state_t new_state )
{
  system_power_perform( new_state );
}

OSStatus mico_system_context_read( mico_Context_t** context_out )
{
  return system_context_read( context_out );
}


OSStatus mico_system_init( mico_Context_t** out_context )
{
  return system_init( out_context );
}


