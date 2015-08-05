#include <pb.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include <sitewhere.h>
#include <sitewhere-arduino.pb.h>


/****************************************************/
#include "time.h"
#include "MicoAES.h"

#include "mico.h"
#include "libemqtt.h"
#include "custom.h"

#define mqtt(M, ...) custom_log("MICO", M, ##__VA_ARGS__)
#define mqtt_log_trace() custom_log_trace("MICO")

mqtt_broker_handle_t broker_mqtt = NULL;
mico_Context_t *context;
/*****************************************************/
void loop();

/** Callback function header */
void callback(char* topic, byte* payload, unsigned int length);

/** Message buffer */
extern uint8_t buffer[300];

/** Keeps up with whether we have registered */
extern bool registered;



/** Timestamp for last event */
uint32_t lastEvent = 0;

/** MQTT client name */

extern char* clientName;

/** Unique hardware id for this device */
extern char* hardwareId;

/** Device specification token for hardware configuration */
extern char* specificationToken;

/** Outbound MQTT topic */
extern char* outbound;

/** Inbound custom command topic */
extern char* Command1;

/** Inbound system command topic */
extern char* System1;

#define RCVBUFSIZE 1024
uint8_t packet_buffer[RCVBUFSIZE];

int read_packet(int timeout,mqtt_broker_handle_t* broker)
{
  int socket_id = (int)broker->socket_info;

  if(timeout > 0)
  {
    fd_set readfds;
    struct timeval_t tmv;

    // Initialize the file descriptor set
    FD_ZERO (&readfds);
    FD_SET (socket_id, &readfds);

    // Initialize the timeout data structure
    tmv.tv_sec = timeout;
    tmv.tv_usec = 0;

    // select returns 0 if timeout, 1 if input available, -1 if error
//    if(select(1, &readfds, NULL, NULL, &tmv))
//      return -2;
  }

  int total_bytes = 0, bytes_rcvd, packet_length;
  memset(packet_buffer, 0, sizeof(packet_buffer));

  if((bytes_rcvd = read(socket_id, (packet_buffer+total_bytes), RCVBUFSIZE)) <= 0) {
    return -1;
  }

  total_bytes += bytes_rcvd; // Keep tally of total bytes
  if (total_bytes < 2)
    return -1;

  // now we have the full fixed header in packet_buffer
  // parse it for remaining length and number of bytes
  uint16_t rem_len = mqtt_parse_rem_len(packet_buffer);
  uint8_t rem_len_bytes = mqtt_num_rem_len_bytes(packet_buffer);

  //packet_length = packet_buffer[1] + 2; // Remaining length + fixed header length
  // total packet length = remaining length + byte 1 of fixed header + remaning length part of fixed header
  packet_length = rem_len + rem_len_bytes + 1;

  while(total_bytes < packet_length) // Reading the packet
  {
    if((bytes_rcvd = read(socket_id, (packet_buffer+total_bytes), RCVBUFSIZE)) <= 0)
      return -1;
    total_bytes += bytes_rcvd; // Keep tally of total bytes
  }

  return packet_length;
}



int len = 0;
uint8_t MQTT_REGISTERED =0;
/** Set up processing */
void mqtt_client()
{
    mqtt("Connected to ethernet.");
    strcpy(broker_mqtt.clientid,hardwareId);
    mqtt_init(&broker_mqtt,hardwareId);

    mqtt_connect(&broker_mqtt);
    read_packet(10,&broker_mqtt);
    if(MQTTParseMessageType(packet_buffer) == MQTT_MSG_CONNACK)
    {
        mqtt("Connected to MQTT.");
    }

  if(packet_buffer[3] == 0x00)
  {
        mqtt("CONNACK successed!\n");
        mqtt_subscribe(&broker_mqtt,Command1,0);
        read_packet(10,&broker_mqtt);

        if(MQTTParseMessageType(packet_buffer) != MQTT_MSG_SUBACK)
        {
            mqtt("SUBACK expected!\n");
            return;
        }


        mqtt_subscribe(&broker_mqtt,System1,0);
        read_packet(10,&broker_mqtt);

        if(MQTTParseMessageType(packet_buffer) != MQTT_MSG_SUBACK)
        {
            mqtt("SUBACK expected!\n");
            return;
        }

        mqtt("subscirbe successed");

        if (len = sw_register(hardwareId, specificationToken, buffer, sizeof(buffer), NULL))
        {
            mqtt_publish(&broker_mqtt,outbound,buffer,len,0);
            MQTT_REGISTERED =1;
            mqtt("Sent registration.");

        }

  }
  else
  {
      mqtt("CONNACK failed!\n");
  }
}
uint32_t now_event;
uint8_t MICO_report[15];



/** Main MQTT processing loop */
void mqtt_loop_thread(void *inContext)
{
    mico_rtc_time_t time;
    struct tm currentTime;
  /** Only send events after registered and at most every five seconds */
    while(1)
    {
            if (registered){
                memset(buffer,0,300);
                unsigned int len = 0;

                if (len = sw_alert(hardwareId, "MicoKit-3288", "is alive", NULL, buffer, sizeof(buffer), NULL)) {
                    mqtt_publish(&broker_mqtt,outbound,buffer,len,0);
                    mqtt("Sent alert.");
                }
                sleep(10);
            }
    }
}
