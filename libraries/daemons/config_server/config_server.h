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

#include "Platform.h"

#include "SocketUtils.h"
#include "HTTPUtils.h"
#include "StringUtils.h"
#include "CheckSumUtils.h"

#include "json.h"

// typedef enum _config_type_t{
//   config_type_bool,
//   config_type_int,
//   config_type_float,
//   config_type_string,
//   config_type_object,
//   config_type_null,
// } config_type_t;

// typedef union {
//     bool          bool_value;
//     int           int_value;
//     double        float_value;
//     char *        string_value;
//     json_object * json_value;
//   } config_value;







/* Start local config server */
OSStatus config_server_start ( mico_Context_t *inContext );

OSStatus config_server_stop ( void );


/* config server delegate */
void config_server_delegate_recv( const char *key, json_object *value, bool *need_reboot, mico_Context_t *inContext );

void config_server_delegate_report( json_object *app_menu, mico_Context_t *inContext );



/* Create device info menu on EasyLink APP that connected to local config server */
OSStatus config_server_create_sector        (json_object* sector_array, char* const name, json_object *cell_array);

OSStatus config_server_create_string_cell   (json_object* cell_array, char* const name, char* const content, char* const privilege, json_object* secectionArray);

OSStatus config_server_create_number_cell   (json_object* cell_array, char* const name, int content, char* const privilege, json_object* secectionArray);

OSStatus config_server_create_float_cell    (json_object* cell_array, char* const name, float content, char* const privilege, json_object* secectionArray);

OSStatus config_server_create_bool_cell     (json_object* cell_array, char* const name, boolean content, char* const privilege);

OSStatus config_server_create_sub_menu_cell (json_object* cell_array, char* const name, json_object* sector_array);


