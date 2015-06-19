/**
******************************************************************************
* @file    MICOEntrance.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   MICO system main entrance.
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

#include "time.h"
#include "MicoPlatform.h"
#include "platform.h"
#include "MICODefine.h"
#include "MICOAppDefine.h"

#include "MICONotificationCenter.h"
#include "MICOSystemMonitor.h"
#include "MicoCli.h"
#include "EasyLink/EasyLink.h"
#include "SoftAP/EasyLinkSoftAP.h"
#include "WPS/WPS.h"
#include "WAC/MFi_WAC.h"
#include "StringUtils.h"

#if defined (CONFIG_MODE_EASYLINK) || defined (CONFIG_MODE_EASYLINK_WITH_SOFTAP)
#include "EasyLink/EasyLink.h"
#endif

#if defined (CONFIG_MODE_AIRKISS)
#include "Airkiss/Airkiss.h"
#endif

#include "tftp/tftp.h"

static mico_Context_t *context;
static mico_timer_t _watchdog_reload_timer;

static mico_system_monitor_t mico_monitor;

const char *eaProtocols[1] = {EA_PROTOCOL};

#define mico_log(M, ...) custom_log("MICO", M, ##__VA_ARGS__)
#define mico_log_trace() custom_log_trace("MICO")

WEAK void sendNotifySYSWillPowerOff(void){

}

/* ========================================
User provide callback functions 
======================================== */
void micoNotify_ReadAppInfoHandler(char *str, int len, mico_Context_t * const inContext)
{
  (void)inContext;
  snprintf( str, len, "%s, build at %s %s", APP_INFO, __TIME__, __DATE__);
}

bool needsUpdate = false;

USED void PlatformEasyLinkButtonClickedCallback(void)
{
  mico_log_trace();
    
  if(context->flashContentInRam.micoSystemConfig.easyLinkByPass != EASYLINK_BYPASS_NO){
    context->flashContentInRam.micoSystemConfig.easyLinkByPass = EASYLINK_BYPASS_NO;
    needsUpdate = true;
  }

  if(context->flashContentInRam.micoSystemConfig.configured == allConfigured){
    context->flashContentInRam.micoSystemConfig.configured = wLanUnConfigured;
    needsUpdate = true;
  }
 
  context->micoStatus.sys_state = eState_Software_Reset;
  require(context->micoStatus.sys_state_change_sem, exit);
  mico_rtos_set_semaphore(&context->micoStatus.sys_state_change_sem);
exit: 
  return;
}

USED void PlatformEasyLinkButtonLongPressedCallback(void)
{
  mico_log_trace();

  mico_log("PlatformEasyLinkButtonLongPressedCallback");
  MICORestoreDefault(context);
  context->micoStatus.sys_state = eState_Software_Reset;
  require(context->micoStatus.sys_state_change_sem, exit);
  mico_rtos_set_semaphore(&context->micoStatus.sys_state_change_sem);
exit: 
  return;
}

USED void PlatformStandbyButtonClickedCallback(void)
 {
    mico_log_trace();
    context->micoStatus.sys_state = eState_Standby;
    require(context->micoStatus.sys_state_change_sem, exit);
    mico_rtos_set_semaphore(&context->micoStatus.sys_state_change_sem);
exit: 
    return;
 }

void micoNotify_WifiStatusHandler(WiFiEvent event, mico_Context_t * const inContext)
{
  mico_log_trace();
  (void)inContext;
  switch (event) {
  case NOTIFY_STATION_UP:
    mico_log("Station up");
    MicoRfLed(true);
    break;
  case NOTIFY_STATION_DOWN:
    mico_log("Station down");
    MicoRfLed(false);
    break;
  case NOTIFY_AP_UP:
    mico_log("uAP established");
    MicoRfLed(true);
    break;
  case NOTIFY_AP_DOWN:
    mico_log("uAP deleted");
    MicoRfLed(false);
    break;
  default:
    break;
  }
  return;
}

void micoNotify_DHCPCompleteHandler(IPStatusTypedef *pnet, mico_Context_t * const inContext)
{
  mico_log_trace();
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

void micoNotify_WiFIParaChangedHandler(apinfo_adv_t *ap_info, char *key, int key_len, mico_Context_t * const inContext)
{
  mico_log_trace();
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


void micoNotify_ConnectFailedHandler(OSStatus err, mico_Context_t * const inContext)
{
  mico_log_trace();
  (void)inContext;
  mico_log("Wlan Connection Err %d", err);
}

void micoNotify_WlanFatalErrHandler(mico_Context_t * const inContext)
{
  mico_log_trace();
  (void)inContext;
  mico_log("Wlan Fatal Err!");
  MicoSystemReboot();
}

void micoNotify_StackOverflowErrHandler(char *taskname, mico_Context_t * const inContext)
{
  mico_log_trace();
  (void)inContext;
  mico_log("Thread %s overflow, system rebooting", taskname);
  MicoSystemReboot(); 
}

void _ConnectToAP( mico_Context_t * const inContext)
{
  mico_log_trace();
  network_InitTypeDef_adv_st wNetConfig;
  mico_log("connect to %s.....", inContext->flashContentInRam.micoSystemConfig.ssid);
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
  micoWlanStartAdv(&wNetConfig);
}

static void _watchdog_reload_timer_handler( void* arg )
{
  (void)(arg);
  MICOUpdateSystemMonitor(&mico_monitor, APPLICATION_WATCHDOG_TIMEOUT_SECONDS*1000);
}

mico_Context_t *getGlobalContext(void)
{
  return context;
}

#ifdef MICO_CLI_ENABLE
static void tftp_Command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
    tftp_file_info_t cmdinfo;
    int tftpcmd;
    uint32_t ip;

    if (argc != 7) {
        goto WRONGCMD;
    }
    if (strcmp(argv[2], "put")==0) {
        tftpcmd = 0;
    } else if (strcmp(argv[2], "get") == 0) {
        tftpcmd = 1;
    } else {
        goto WRONGCMD;
    }

    ip = inet_addr(argv[1]);
    if (strcmp(argv[4], "i")==0) {
        cmdinfo.flashtype = MICO_INTERNAL_FLASH;
    } else if (strcmp(argv[4], "s") == 0) {
        cmdinfo.flashtype = MICO_SPI_FLASH;
    } else {
        goto WRONGCMD;
    }

    cmdinfo.flashaddr = strtoul(argv[5], NULL, 0);
    strncpy(cmdinfo.filename, argv[3], 32);

    cmdinfo.filelen= strtoul(argv[6], NULL, 0);
    if (tftpcmd == 0) { // put
        cmd_printf("tftp put to %s, filenmae %s. from %s flash, address 0x%x, len %d\r\n", argv[1], cmdinfo.filename,
            cmdinfo.flashtype==MICO_INTERNAL_FLASH?"internal":"SPI", cmdinfo.flashaddr, cmdinfo.filelen);
        tsend(&cmdinfo, ip);
    } else { // get
        cmd_printf("tftp get from %s, filenmae %s. to %s flash, address 0x%x, len %d\r\n", argv[1], cmdinfo.filename,
            cmdinfo.flashtype==MICO_INTERNAL_FLASH?"internal":"SPI", cmdinfo.flashaddr, cmdinfo.filelen);
        tget(&cmdinfo, ip);
    }
    return;
    
WRONGCMD:
    cmd_printf("Usage: tftp <ip> put <filename> i/s <flashaddr> <flashlen>\r\n"
               "       tftp <ip> get <filenmae> i/s <flashaddr> <flashlen>\r\n"
               "        i=internale flash, s=SPI flash");
}

static void uptime_Command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
    cmd_printf("UP time %dms\r\n", mico_get_time());
}

static void ota_Command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
    mico_fource_ota();
}

static const struct cli_command user_clis[] = {
    {"tftp",     "tftp",                        tftp_Command},
    {"time",     "system time",                 uptime_Command},
    {"ota",      "system ota",                  ota_Command},
};
#endif

void mico_write_ota_tbl(int len)
{
    memset(&context->flashContentInRam.bootTable, 0, sizeof(boot_table_t));
    context->flashContentInRam.bootTable.length = len;
    context->flashContentInRam.bootTable.start_address = UPDATE_START_ADDRESS;
    context->flashContentInRam.bootTable.type = 'A';
    context->flashContentInRam.bootTable.upgrade_type = 'U';
    MICOUpdateConfiguration(context);
}

int application_start(void)
{
  OSStatus err = kNoErr;
  IPStatusTypedef para;
  struct tm currentTime;
  mico_rtc_time_t time;
  char wifi_ver[64] = {0};
  mico_log_trace(); 

  /*Read current configurations*/
  context = ( mico_Context_t *)malloc(sizeof(mico_Context_t) );
  require_action( context, exit, err = kNoMemoryErr );
  memset(context, 0x0, sizeof(mico_Context_t));
  mico_rtos_init_mutex(&context->flashContentInRam_mutex);
  mico_rtos_init_semaphore(&context->micoStatus.sys_state_change_sem, 1); 

  MICOReadConfiguration( context );

  err = MICOInitNotificationCenter  ( context );

  err = MICOAddNotification( mico_notify_READ_APP_INFO, (void *)micoNotify_ReadAppInfoHandler );
  require_noerr( err, exit );  

  err = MICOAddNotification( mico_notify_WIFI_CONNECT_FAILED, (void *)micoNotify_ConnectFailedHandler );
  require_noerr( err, exit ); 

  err = MICOAddNotification( mico_notify_WIFI_Fatal_ERROR, (void *)micoNotify_WlanFatalErrHandler );
  require_noerr( err, exit ); 

  err = MICOAddNotification( mico_notify_Stack_Overflow_ERROR, (void *)micoNotify_StackOverflowErrHandler );
  require_noerr( err, exit ); 

  /*wlan driver and tcpip init*/
  mico_log( "MiCO starting..." );
  MicoInit();
#ifdef MICO_CLI_ENABLE
  MicoCliInit();
	cli_register_commands(user_clis, sizeof(user_clis)/sizeof(struct cli_command));
#endif
  MicoSysLed(true);
  mico_log("Free memory %d bytes", MicoGetMemoryInfo()->free_memory); 
  micoWlanGetIPStatus(&para, Station);
  formatMACAddr(context->micoStatus.mac, (char *)&para.mac);
  MicoGetRfVer(wifi_ver, sizeof(wifi_ver));
  mico_log("%s mxchipWNet library version: %s", APP_INFO, MicoGetVer());
  mico_log("Wi-Fi driver version %s, mac %s", wifi_ver, context->micoStatus.mac);
 
  /*Start system monotor thread*/
  err = MICOStartSystemMonitor(context);
  require_noerr_action( err, exit, mico_log("ERROR: Unable to start the system monitor.") );

  err = MICORegisterSystemMonitor(&mico_monitor, APPLICATION_WATCHDOG_TIMEOUT_SECONDS*1000);
  require_noerr( err, exit );
  mico_init_timer(&_watchdog_reload_timer,APPLICATION_WATCHDOG_TIMEOUT_SECONDS*1000/2, _watchdog_reload_timer_handler, NULL);
  mico_start_timer(&_watchdog_reload_timer);

  /* Enter test mode, call a build-in test function amd output on MFG UART */
  if(MicoShouldEnterMFGMode()==true){
    mico_log( "Enter MFG mode by MFG button" );
    mico_mfg_test(context);
  }
  
  /*Read current time from RTC.*/
  if( MicoRtcGetTime(&time) == kNoErr ){
    currentTime.tm_sec = time.sec;
    currentTime.tm_min = time.min;
    currentTime.tm_hour = time.hr;
    currentTime.tm_mday = time.date;
    currentTime.tm_wday = time.weekday;
    currentTime.tm_mon = time.month - 1;
    currentTime.tm_year = time.year + 100;
    mico_log("Current Time: %s",asctime(&currentTime));
  }else
    mico_log("RTC function unsupported");
  
  /* Regisist notifications */
  err = MICOAddNotification( mico_notify_WIFI_STATUS_CHANGED, (void *)micoNotify_WifiStatusHandler );
  require_noerr( err, exit ); 

  if( context->flashContentInRam.micoSystemConfig.configured == wLanUnConfigured ||
      context->flashContentInRam.micoSystemConfig.configured == unConfigured){
    mico_log("Empty configuration. Starting configuration mode...");

#if (MICO_CONFIG_MODE == CONFIG_MODE_EASYLINK) || (MICO_CONFIG_MODE == CONFIG_MODE_EASYLINK_WITH_SOFTAP)
  err = startEasyLink( context );
  require_noerr( err, exit );
#elif (MICO_CONFIG_MODE == CONFIG_MODE_SOFT_AP)
  err = startEasyLinkSoftAP( context );
  require_noerr( err, exit );
#elif (MICO_CONFIG_MODE == CONFIG_MODE_AIRKISS)
  err = startAirkiss( context );
  require_noerr( err, exit );
#elif (MICO_CONFIG_MODE == CONFIG_MODE_WPS) || MICO_CONFIG_MODE == defined (CONFIG_MODE_WPS_WITH_SOFTAP)
  err = startWPS( context );
  require_noerr( err, exit );
#elif ( MICO_CONFIG_MODE == CONFIG_MODE_WAC)
  WACPlatformParameters_t* WAC_Params = NULL;
  WAC_Params = calloc(1, sizeof(WACPlatformParameters_t));
  require(WAC_Params, exit);

  str2hex((unsigned char *)para.mac, WAC_Params->macAddress, 6);
  WAC_Params->isUnconfigured          = 1;
  WAC_Params->supportsAirPlay         = 0;
  WAC_Params->supportsAirPrint        = 0;
  WAC_Params->supports2_4GHzWiFi      = 1;
  WAC_Params->supports5GHzWiFi        = 0;
  WAC_Params->supportsWakeOnWireless  = 0;

  WAC_Params->firmwareRevision =  FIRMWARE_REVISION;
  WAC_Params->hardwareRevision =  HARDWARE_REVISION;
  WAC_Params->serialNumber =      SERIAL_NUMBER;
  WAC_Params->name =              context->flashContentInRam.micoSystemConfig.name;
  WAC_Params->model =             MODEL;
  WAC_Params->manufacturer =      MANUFACTURER;

  WAC_Params->numEAProtocols =    1;
  WAC_Params->eaBundleSeedID =    BUNDLE_SEED_ID;
  WAC_Params->eaProtocols =       (char **)eaProtocols;

  err = startMFiWAC( context, WAC_Params, MICO_I2C_CP, 1200);
  free(WAC_Params);
  require_noerr( err, exit );
#else
  #error "Wi-Fi configuration mode is not defined"
#endif
  }
#ifdef MFG_MODE_AUTO
  else if( context->flashContentInRam.micoSystemConfig.configured == mfgConfigured ){
    mico_log( "Enter MFG mode automatically" );
    mico_mfg_test(context);
    mico_thread_sleep(MICO_NEVER_TIMEOUT);
  }
#endif

  else{
    mico_log("Available configuration. Starting Wi-Fi connection...");
    
    err = MICOAddNotification( mico_notify_WiFI_PARA_CHANGED, (void *)micoNotify_WiFIParaChangedHandler );
    require_noerr( err, exit ); 

    err = MICOAddNotification( mico_notify_DHCP_COMPLETED, (void *)micoNotify_DHCPCompleteHandler );
    require_noerr( err, exit );  
   
    if(context->flashContentInRam.micoSystemConfig.rfPowerSaveEnable == true){
      micoWlanEnablePowerSave();
    }

    if(context->flashContentInRam.micoSystemConfig.mcuPowerSaveEnable == true){
      MicoMcuPowerSaveConfig(true);
    }

    /*Local configuration server*/
    if(context->flashContentInRam.micoSystemConfig.configServerEnable == true){
      err =  MICOStartConfigServer(context);
      require_noerr_action( err, exit, mico_log("ERROR: Unable to start the local server thread.") );
    }

    err =  MICOStartNTPClient(context);
    require_noerr_action( err, exit, mico_log("ERROR: Unable to start the NTP client thread.") );

    /*Start mico application*/
    err = MICOStartApplication( context );
    require_noerr( err, exit );

    _ConnectToAP( context );
  }

  mico_log("Free memory %d bytes", MicoGetMemoryInfo()->free_memory) ; 
  
  /*System status changed*/
  while(mico_rtos_get_semaphore(&context->micoStatus.sys_state_change_sem, MICO_WAIT_FOREVER)==kNoErr){
    
    if(needsUpdate == true)
      MICOUpdateConfiguration(context);
    
    switch(context->micoStatus.sys_state){
      case eState_Normal:
        break;
      case eState_Software_Reset:
        sendNotifySYSWillPowerOff();
        mico_thread_msleep(500);
        MicoSystemReboot();
        break;
      case eState_Wlan_Powerdown:
        sendNotifySYSWillPowerOff();
        mico_thread_msleep(500);
        micoWlanPowerOff();
        break;
      case eState_Standby:
        mico_log("Enter standby mode");
        sendNotifySYSWillPowerOff();
        mico_thread_msleep(200);
        micoWlanPowerOff();
        MicoSystemStandBy(MICO_WAIT_FOREVER);
        break;
      default:
        break;
    }
  }
    
  require_noerr_action( err, exit, mico_log("Closing main thread with err num: %d.", err) );

exit:
  mico_rtos_delete_thread(NULL);
  return kNoErr;
}
