/**
******************************************************************************
* @file    MICOConfigServer.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   Local TCP server for mico device configuration 
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
#include "platform_config.h"

#include "SocketUtils.h"
#include "Platform.h"
#include "HTTPUtils.h"
#include "mico_system.h"
#include "StringUtils.h"
#include "CheckSumUtils.h"
#include "system.h"
#include "JSON-C/json.h"

#define config_log(M, ...) custom_log("CONFIG SERVER", M, ##__VA_ARGS__)
#define config_log_trace() custom_log_trace("CONFIG SERVER")

#define kCONFIGURLRead          "/config-read"
#define kCONFIGURLWrite         "/config-write"
#define kCONFIGURLWriteByUAP    "/config-write-uap"  /* Don't reboot but connect to AP immediately */
#define kCONFIGURLOTA           "/OTA"

#define kMIMEType_MXCHIP_OTA    "application/ota-stream"

typedef struct _configContext_t{
  uint32_t offset;
  bool     isFlashLocked;
  CRC16_Context crc16_contex;
} configContext_t;

extern OSStatus     ConfigIncommingJsonMessage( const char *input, bool *need_reboot, mico_Context_t * const inContext );
extern json_object* ConfigCreateReportJsonMessage( mico_Context_t * const inContext );

static void localConfiglistener_thread(void *inContext);
static void localConfig_thread(void *inFd);
static mico_Context_t *Context;
static OSStatus _LocalConfigRespondInComingMessage(int fd, HTTPHeader_t* inHeader, mico_Context_t * const inContext);
static OSStatus onReceivedData(struct _HTTPHeader_t * httpHeader, uint32_t pos, uint8_t * data, size_t len, void * userContext );
static void onClearHTTPHeader(struct _HTTPHeader_t * httpHeader, void * userContext );

bool is_config_server_established = false;

/* Defined in uAP config mode */
extern OSStatus     ConfigIncommingJsonMessageUAP( const char *input, mico_Context_t * const inContext );

static mico_semaphore_t close_listener_sem = NULL, close_client_sem[ MAX_TCP_CLIENT_PER_SERVER ] = { NULL };



OSStatus MICOStartConfigServer ( void )
{
  int i = 0;
  OSStatus err = kNoErr;
  mico_Context_t* context = NULL;
  
  context = m_system_context_get( );
  require( context, exit );
  
  if( is_config_server_established )
    return kNoErr;

  config_log("Start config server");

  close_listener_sem = NULL;
  for (; i < MAX_TCP_CLIENT_PER_SERVER; i++)
    close_client_sem[ i ] = NULL;
  err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "Config Server", localConfiglistener_thread, STACK_SIZE_LOCAL_CONFIG_SERVER_THREAD, (void*)context );
  require_noerr(err, exit);
  is_config_server_established = true;
  mico_thread_msleep(200);

exit:
  return err;
}

OSStatus MICOStopConfigServer( void )
{
  int i = 0;
  OSStatus err = kNoErr;

  if( !is_config_server_established )
    return kNoErr;

  for (; i < MAX_TCP_CLIENT_PER_SERVER; i++){
    if( close_client_sem[ i ] != NULL )
      mico_rtos_set_semaphore( &close_client_sem[ i ] );
  }
  mico_thread_msleep(50);

  if( close_listener_sem != NULL )
    mico_rtos_set_semaphore( &close_listener_sem );

  config_log(" Wait for 2s!");
  mico_thread_msleep(500);
  is_config_server_established = false;
  
  return err;
}

void localConfiglistener_thread(void *inContext)
{
  config_log_trace();
  OSStatus err = kUnknownErr;
  int j;
  Context = inContext;
  struct sockaddr_t addr;
  int sockaddr_t_size;
  fd_set readfds;
  char ip_address[16];
  
  int localConfiglistener_fd = -1;
  int close_listener_fd = -1;

  mico_rtos_init_semaphore( &close_listener_sem, 1);
  close_listener_fd = mico_create_event_fd( close_listener_sem );

  /*Establish a TCP server fd that accept the tcp clients connections*/ 
  localConfiglistener_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
  require_action(IsValidSocket( localConfiglistener_fd ), exit, err = kNoResourcesErr );
  addr.s_ip = INADDR_ANY;
  addr.s_port = MICO_CONFIG_SERVER_PORT;
  err = bind(localConfiglistener_fd, &addr, sizeof(addr));
  require_noerr( err, exit );

  err = listen(localConfiglistener_fd, 0);
  require_noerr( err, exit );

  config_log("Config Server established at port: %d, fd: %d", MICO_CONFIG_SERVER_PORT, localConfiglistener_fd);
  
  while(1){
    FD_ZERO(&readfds);
    FD_SET(localConfiglistener_fd, &readfds);
    FD_SET(close_listener_fd, &readfds);
    select(1, &readfds, NULL, NULL, NULL);

    /* Check close requests */
    if(FD_ISSET(close_listener_fd, &readfds)){
      mico_rtos_get_semaphore( &close_listener_sem, 0 );
      goto exit;
    }

    /* Check tcp connection requests */
    if(FD_ISSET(localConfiglistener_fd, &readfds)){
      sockaddr_t_size = sizeof(struct sockaddr_t);
      j = accept(localConfiglistener_fd, &addr, &sockaddr_t_size);
      if ( IsValidSocket( j ) ) {
        inet_ntoa(ip_address, addr.s_ip );
        config_log("Config Client %s:%d connected, fd: %d", ip_address, addr.s_port, j);
        if(kNoErr !=  mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Config Clients", localConfig_thread, STACK_SIZE_LOCAL_CONFIG_CLIENT_THREAD, &j) )
          SocketClose(&j);
      }
    }
  }

exit:
    if( close_listener_sem != NULL ){
      mico_delete_event_fd( close_listener_fd );
      mico_rtos_deinit_semaphore( &close_listener_sem );
      close_listener_sem = NULL;
    };
    config_log("Exit: Config listener exit with err = %d", err);
    SocketClose( &localConfiglistener_fd );
    mico_rtos_delete_thread(NULL);
    return;
}

void localConfig_thread(void *inFd)
{
  OSStatus err;
  int clientFd = *(int *)inFd;
  int clientFdIsSet;
  int close_sem_index;
  fd_set readfds;
  struct timeval_t t;
  HTTPHeader_t *httpHeader = NULL;
  int close_client_fd = -1;
  configContext_t httpContext = {0, false, 0};

  for( close_sem_index = 0; close_sem_index < MAX_TCP_CLIENT_PER_SERVER; close_sem_index++ ){
    if( close_client_sem[close_sem_index] == NULL )
      break;
  }

  if( close_sem_index == MAX_TCP_CLIENT_PER_SERVER){
    mico_rtos_delete_thread(NULL);
    return;
  }

  mico_rtos_init_semaphore( &close_client_sem[close_sem_index], 1);
  close_client_fd = mico_create_event_fd( close_client_sem[close_sem_index] );

  config_log_trace();
  httpHeader = HTTPHeaderCreateWithCallback(onReceivedData, onClearHTTPHeader, &httpContext);
  require_action( httpHeader, exit, err = kNoMemoryErr );
  HTTPHeaderClear( httpHeader );

  t.tv_sec = 60;
  t.tv_usec = 0;
  config_log("Free memory %d bytes", MicoGetMemoryInfo()->free_memory) ; 

  while(1){
    FD_ZERO(&readfds);
    FD_SET(clientFd, &readfds);
    FD_SET(close_client_fd, &readfds);
    clientFdIsSet = 0;

    if(httpHeader->len == 0){
      require(select(1, &readfds, NULL, NULL, &t) >= 0, exit);
      clientFdIsSet = FD_ISSET(clientFd, &readfds);
    }

    /* Check close requests */
    if(FD_ISSET(close_client_fd, &readfds)){
      mico_rtos_get_semaphore( &close_client_sem[close_sem_index], 0 );
      err = kConnectionErr;
      goto exit;
    }    
  
    if(clientFdIsSet||httpHeader->len){
      err = SocketReadHTTPHeader( clientFd, httpHeader );

      switch ( err )
      {
        case kNoErr:
          // Read the rest of the HTTP body if necessary
          //do{
          err = SocketReadHTTPBody( clientFd, httpHeader );
          
          if(httpHeader->dataEndedbyClose == true){
            err = _LocalConfigRespondInComingMessage( clientFd, httpHeader, Context );
            require_noerr(err, exit);
            err = kConnectionErr;
            goto exit;
          }else{
            require_noerr(err, exit);
            err = _LocalConfigRespondInComingMessage( clientFd, httpHeader, Context );
            require_noerr(err, exit);
          }

          HTTPHeaderClear( httpHeader );
        break;

        case EWOULDBLOCK:
            // NO-OP, keep reading
        break;

        case kNoSpaceErr:
          config_log("ERROR: Cannot fit HTTPHeader.");
          goto exit;
        
        case kConnectionErr:
          // NOTE: kConnectionErr from SocketReadHTTPHeader means it's closed
          config_log("ERROR: Connection closed.");
          goto exit;

        default:
          config_log("ERROR: HTTP Header parse internal error: %d", err);
          goto exit;
      }
    }
  }

exit:
  config_log("Exit: Client exit with err = %d", err);
  SocketClose(&clientFd);

  if( close_client_sem[close_sem_index] != NULL )
  {
    mico_delete_event_fd( close_client_fd );
    mico_rtos_deinit_semaphore( &close_client_sem[close_sem_index] );
    close_client_sem[close_sem_index] = NULL;
  };

  if(httpHeader) {
    HTTPHeaderClear( httpHeader );
    free(httpHeader);
  }
  mico_rtos_delete_thread(NULL);
  return;
}

static OSStatus onReceivedData(struct _HTTPHeader_t * inHeader, uint32_t inPos, uint8_t * inData, size_t inLen, void * inUserContext )
{
  OSStatus err = kUnknownErr;
  const char *    value;
  size_t          valueSize;
  configContext_t *context = (configContext_t *)inUserContext;
  mico_logic_partition_t* ota_partition = MicoFlashGetInfo( MICO_PARTITION_OTA_TEMP );

  err = HTTPGetHeaderField( inHeader->buf, inHeader->len, "Content-Type", NULL, NULL, &value, &valueSize, NULL );
  if(err == kNoErr && strnicmpx( value, valueSize, kMIMEType_MXCHIP_OTA ) == 0){
    printf("%d/", inPos);

    if( ota_partition->partition_owner == MICO_FLASH_NONE ){
      config_log("OTA storage is not exist");
      return kUnsupportedErr;
    }

     if(inPos == 0){
       context->offset = 0x0;
       CRC16_Init( &context->crc16_contex );
       mico_rtos_lock_mutex(&Context->flashContentInRam_mutex); //We are write the Flash content, no other write is possiable
       context->isFlashLocked = true;
       err = MicoFlashErase( MICO_PARTITION_OTA_TEMP, 0x0, ota_partition->partition_length);
       require_noerr(err, flashErrExit);
       err = MicoFlashWrite( MICO_PARTITION_OTA_TEMP, &context->offset, (uint8_t *)inData, inLen);
       require_noerr(err, flashErrExit);
       CRC16_Update( &context->crc16_contex, inData, inLen);
     }else{
       err = MicoFlashWrite( MICO_PARTITION_OTA_TEMP, &context->offset, (uint8_t *)inData, inLen);
       require_noerr(err, flashErrExit);
       CRC16_Update( &context->crc16_contex, inData, inLen);
     }
  }
  else{
    return kUnsupportedErr;
  }

  if(err!=kNoErr)  config_log("onReceivedData");
  return err;

flashErrExit:
  mico_rtos_unlock_mutex(&Context->flashContentInRam_mutex);
  return err;
}

static void onClearHTTPHeader(struct _HTTPHeader_t * inHeader, void * inUserContext )
{
  UNUSED_PARAMETER(inHeader);
  configContext_t *context = (configContext_t *)inUserContext;

  if(context->isFlashLocked == true){
    mico_rtos_unlock_mutex(&Context->flashContentInRam_mutex);
    context->isFlashLocked = false;
  }
 }

OSStatus _LocalConfigRespondInComingMessage(int fd, HTTPHeader_t* inHeader, mico_Context_t * const inContext)
{
  OSStatus err = kUnknownErr;
  const char *  json_str;
  uint8_t *httpResponse = NULL;
  size_t httpResponseLen = 0;
  json_object* report = NULL;
  bool need_reboot = false;
  uint16_t crc;
  configContext_t *http_context = (configContext_t *)inHeader->userContext;
  mico_logic_partition_t* ota_partition = MicoFlashGetInfo( MICO_PARTITION_OTA_TEMP );

  config_log_trace();

  if(HTTPHeaderMatchURL( inHeader, kCONFIGURLRead ) == kNoErr){    
    report = ConfigCreateReportJsonMessage( inContext );
    require( report, exit );
    json_str = json_object_to_json_string(report);
    require_action( json_str, exit, err = kNoMemoryErr );
    config_log("Send config object=%s", json_str);
    err =  CreateSimpleHTTPMessageNoCopy( kMIMEType_JSON, strlen(json_str), &httpResponse, &httpResponseLen );
    require_noerr( err, exit );
    require( httpResponse, exit );
    err = SocketSend( fd, httpResponse, httpResponseLen );
    require_noerr( err, exit );
    err = SocketSend( fd, (uint8_t *)json_str, strlen(json_str) );
    require_noerr( err, exit );
    config_log("Current configuration sent");
    goto exit;
  }
  else if(HTTPHeaderMatchURL( inHeader, kCONFIGURLWrite ) == kNoErr){
    if(inHeader->contentLength > 0){
      config_log("Recv new configuration, apply");

      err =  CreateSimpleHTTPOKMessage( &httpResponse, &httpResponseLen );
      require_noerr( err, exit );
      require( httpResponse, exit );
      err = SocketSend( fd, httpResponse, httpResponseLen );
      require_noerr( err, exit );

      err = ConfigIncommingJsonMessage( inHeader->extraDataPtr, &need_reboot, inContext );
      require_noerr( err, exit );
      inContext->flashContentInRam.micoSystemConfig.configured = allConfigured;
      m_system_context_update( inContext );

      if( need_reboot == true ){
        m_system_power_perform( inContext, eState_Software_Reset );
      }
    }
    goto exit;
  }
else if(HTTPHeaderMatchURL( inHeader, kCONFIGURLWriteByUAP ) == kNoErr){
    if(inHeader->contentLength > 0){
      config_log( "Recv new configuration from uAP, apply and connect to AP" );
      err = ConfigIncommingJsonMessageUAP( inHeader->extraDataPtr, inContext );
      require_noerr( err, exit );
      m_system_context_update( inContext );

      err =  CreateSimpleHTTPOKMessage( &httpResponse, &httpResponseLen );
      require_noerr( err, exit );
      require( httpResponse, exit );

      err = SocketSend( fd, httpResponse, httpResponseLen );
      require_noerr( err, exit );
      sleep(1);

      micoWlanSuspendSoftAP();
      system_connect_wifi_normal( inContext );
    }
    goto exit;
  }
  else if(HTTPHeaderMatchURL( inHeader, kCONFIGURLOTA ) == kNoErr && ota_partition->partition_owner != MICO_FLASH_NONE){
    if(inHeader->contentLength > 0){
      config_log("Receive OTA data!");
      CRC16_Final( &http_context->crc16_contex, &crc);
      memset(&inContext->flashContentInRam.bootTable, 0, sizeof(boot_table_t));
      inContext->flashContentInRam.bootTable.length = inHeader->contentLength;
      inContext->flashContentInRam.bootTable.start_address = ota_partition->partition_start_addr;
      inContext->flashContentInRam.bootTable.type = 'A';
      inContext->flashContentInRam.bootTable.upgrade_type = 'U';
      inContext->flashContentInRam.bootTable.crc = crc;
      if( inContext->flashContentInRam.micoSystemConfig.configured != allConfigured )
        inContext->flashContentInRam.micoSystemConfig.easyLinkByPass = EASYLINK_SOFT_AP_BYPASS;
      m_system_context_update( inContext );
      SocketClose( &fd );
      m_system_power_perform( inContext, eState_Software_Reset );
      mico_thread_sleep( MICO_WAIT_FOREVER );
    }
    goto exit;
  }
  else{
    return kNotFoundErr;
  };

 exit:
  if(inHeader->persistent == false)  //Return an err to close socket and exit the current thread
    err = kConnectionErr;
  if(httpResponse)  free(httpResponse);
  if(report)        json_object_put(report);

  return err;

}

