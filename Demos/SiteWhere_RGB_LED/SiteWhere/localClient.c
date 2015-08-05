/**
  ******************************************************************************
  * @file    RemoteTcpClient.c
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   Create a TCP client thread, and connect to a remote server.
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
#include "MiCOAppDefine.h"
#include "libemqtt.h"

#define client_log(M, ...) custom_log("Local client", M, ##__VA_ARGS__)
#define client_log_trace() custom_log_trace("Local client")

#define CLOUD_RETRY  1

static bool _wifiConnected = false;
static mico_semaphore_t  _wifiConnected_sem = NULL;

extern  char* Command1;
/** Inbound system command topic */
extern char* System1;

void clientNotify_WifiStatusHandler(int event, app_context_t * const inContext)
{
  client_log_trace();
  (void)inContext;
  switch (event) {
  case NOTIFY_STATION_UP:
    _wifiConnected = true;
    mico_rtos_set_semaphore(&_wifiConnected_sem);
    break;
  case NOTIFY_STATION_DOWN:
    _wifiConnected = false;
    break;
  default:
    break;
  }
  return;
}
uint8_t connected_to_ethernet=0;
extern uint8_t MQTT_REGISTERED;

extern mqtt_broker_handle_t broker_mqtt;

int send_packet(void* socket_info, const void* buf, unsigned int count)
{
  return send((int)socket_info, buf, count, 0);
}

OSStatus sppWlanCommandProcess(unsigned char *inBuf, int *inBufLen, int inSocketFd, mico_Context_t * const inContext)
{
  client_log_trace();
  (void)inSocketFd;
  (void)inContext;
  OSStatus err = kUnknownErr;

    uint8_t *packet_buffer;

    packet_buffer = inBuf;

    client_log("Packet Header: 0x%x...\n", packet_buffer[0]);
    if(MQTTParseMessageType(packet_buffer) == MQTT_MSG_PUBLISH)
    {
      uint8_t topic[255], msg[1000];
      uint16_t len;
      len = mqtt_parse_pub_topic(packet_buffer, topic);
      topic[len] = '\0'; // for printf
      len = mqtt_parse_publish_msg(packet_buffer, msg);
      msg[len] = '\0';


        if (strcmp(topic, System1) == 0)
        {
            handleSystemCommand(msg, len);
        }
        else if (strcmp(topic, Command1) == 0)
        {
            handleSpecificationCommand(msg, len);
        }
    }

  *inBufLen = 0;
  return err;
}

void sitewhere_main_thread(void *inContext)
{
  client_log_trace();
  OSStatus err = kUnknownErr;
  int len;
  mico_Context_t *Context = inContext;
  struct sockaddr_t addr;
  fd_set readfds;
  fd_set writeSet;
  char ipstr[16];
  struct timeval_t t;
  int remoteTcpClient_fd = -1;
  uint8_t *inDataBuffer = NULL;
  int eventFd = -1;
  mico_queue_t queue;
 // socket_msg_t *msg;
  int sent_len, errno;

  mico_rtos_init_semaphore(&_wifiConnected_sem, 1);

  /* Regisist notifications */
  //err = MICOAddNotification( mico_notify_WIFI_STATUS_CHANGED, (void *)clientNotify_WifiStatusHandler );
  err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED, (void *)clientNotify_WifiStatusHandler, inContext);
  require_noerr( err, exit );

  inDataBuffer = malloc(1024);
  require_action(inDataBuffer, exit, err = kNoMemoryErr);


  while(1) {
    if(remoteTcpClient_fd == -1 ) {
      if(_wifiConnected == false){
        //  sleep(1);
          //continue;
       require_action_quiet(mico_rtos_get_semaphore(&_wifiConnected_sem, 2000000) == kNoErr, Continue, err = kTimeoutErr);
      }
      err = gethostbyname("sitewhere.chinacloudapp.cn", (uint8_t *)ipstr, 16);
      require_noerr(err, ReConnWithDelay);

      remoteTcpClient_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


      addr.s_ip = inet_addr(ipstr);
      addr.s_port = 1883;

      err = connect(remoteTcpClient_fd, &addr, sizeof(addr));
      require_noerr_quiet(err, ReConnWithDelay);
      client_log("Remote server connected at port: %d, fd: %d",  1883,
                 remoteTcpClient_fd);
      broker_mqtt.socket_info =(void *)remoteTcpClient_fd;
      broker_mqtt.send = send_packet;

      mqtt_client();

      connected_to_ethernet =1;
    }
    else
    {
      FD_ZERO(&readfds);
      FD_SET(remoteTcpClient_fd, &readfds);
      FD_SET(eventFd, &readfds);
      FD_ZERO(&writeSet );
      FD_SET(remoteTcpClient_fd, &writeSet );
      t.tv_sec = 4;
      t.tv_usec = 0;
      select(1, &readfds, &writeSet, NULL, &t);

      /*recv wlan data using remote client fd*/
          if(MQTT_REGISTERED==1)
          {
              if (FD_ISSET(remoteTcpClient_fd, &readfds)) {
                len = recv(remoteTcpClient_fd, inDataBuffer, 1024, 0);
                if(len <= 0) {
                  client_log("Remote client closed, fd: %d", remoteTcpClient_fd);
                  connected_to_ethernet =0;
                  goto ReConnWithDelay;
                }
                client_log("recv: %d", len);
                sppWlanCommandProcess(inDataBuffer, &len, remoteTcpClient_fd, Context);
              }
          }

    Continue:
      continue;

    ReConnWithDelay:
        if (eventFd >= 0) {
          mico_delete_event_fd(eventFd);
          eventFd = -1;
        }
        if(remoteTcpClient_fd != -1){
          SocketClose(&remoteTcpClient_fd);
        }
        sleep(CLOUD_RETRY);
    }
  }

exit:
  if(inDataBuffer) free(inDataBuffer);
  client_log("Exit: local client exit with err = %d", err);
  mico_rtos_delete_thread(NULL);
  return;
}

