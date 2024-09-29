/******************************* Client helper *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _CLIENT_HELPER_H_
#define _CLIENT_HELPER_H_

#include <interface/pdi.h>
#include <config/Config.h>
#include <utility/Utility.h>

/* client support functions */

bool connectToServer( iClientInterface *client, char *host, uint16_t port, uint16_t timeout=500 );
bool disconnect( iClientInterface *client );
bool isConnected( iClientInterface *client );
bool sendPacket( iClientInterface *client, uint8_t *buffer, uint16_t len );
uint16_t readPacket( iClientInterface *client, uint8_t *buffer, uint16_t maxlen, int32_t timeout, uint8_t readUntilChar=0 );

#endif
