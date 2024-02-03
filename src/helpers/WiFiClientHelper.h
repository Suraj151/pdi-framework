/***************************** WiFiClient helper ******************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WIFI_CLIENT_HELPER_H_
#define _WIFI_CLIENT_HELPER_H_

#include <interface/pdi.h>
#include <config/Config.h>
#include <utility/Utility.h>

/*   WiFi client support functions */

bool connectToServer( iWiFiClientInterface *client, char *host, uint16_t port, uint16_t timeout=500 );
bool disconnect( iWiFiClientInterface *client );
bool isConnected( iWiFiClientInterface *client );
bool sendPacket( iWiFiClientInterface *client, uint8_t *buffer, uint16_t len );
uint16_t readPacket( iWiFiClientInterface *client, uint8_t *buffer, uint16_t maxlen, int16_t timeout );

#endif
