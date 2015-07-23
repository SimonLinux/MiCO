/**
******************************************************************************
* @file    MFi_WAC.h 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide header file for start a Apple WAC (wireless accessory
*          configuration) function thread.
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

#pragma once

#include "Common.h"
#include "mico_wlan.h"
#include "JSON-C/json.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char*  protocol;
	char*  hdVersion;
	char*  fwVersion;
	char*  rfVersion;
} OTA_Versions_t;

typedef struct
{
  //char *name;
  char *service_name;
  char *host_name;
  char *instance_name;
  char *txt_record;
  uint16_t service_port;
} bonjour_init_t;

/** Initialize the CLI module
 *
 * \return kNoErr on success
 * \return error code otherwise.
 *
 * \note This function is called by the wm_core_init function. Applications need
 * not explicity call this function if they have already called wm_core_init().
 *
 */
int mico_cli_init(void);


/* NTP client that sync system clock with internet NTP server */
OSStatus MICOStartNTPClient             ( void );


/* mdns service */
OSStatus mico_service_mdns_add_record( bonjour_init_t init, WiFi_Interface interface, uint32_t time_to_live );

void mico_service_mdns_suspend_record( char *service_name, WiFi_Interface interface, bool will_remove );

void mico_service_mdns_resume_record( char *service_name, WiFi_Interface interface );

void mico_service_mdns_update_txt_record( char *service_name, WiFi_Interface interface, char *txt_record );


/* Start local config server */
OSStatus MICOStartConfigServer          ( void );

OSStatus MICOStopConfigServer           ( void );

/* Create device info menu on EasyLink APP that connected to local config server */
OSStatus MICOAddTopMenu(json_object **deviceInfo, char* const name, json_object* sectors, OTA_Versions_t versions);

OSStatus MICOAddSector(json_object* sectors, char* const name, json_object *menus);

OSStatus MICOAddStringCellToSector(json_object* menus, char* const name, char* const content, char* const privilege, json_object* secectionArray);

OSStatus MICOAddNumberCellToSector(json_object* menus, char* const name, int content, char* const privilege, json_object* secectionArray);

OSStatus MICOAddFloatCellToSector(json_object* menus, char* const name, float content, char* const privilege, json_object* secectionArray);

OSStatus MICOAddSwitchCellToSector(json_object* menus, char* const name, boolean content, char* const privilege);

OSStatus MICOAddMenuCellToSector(json_object* menus, char* const name, json_object* lowerSectors);

/* Download a new firmware and update from TFTP server use a predefined ssid, key, ip address */
void mico_force_ota(void);


#ifdef __cplusplus
} /*extern "C" */
#endif
