/***************************** WiFiClient helper ******************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "WiFiClientHelper.h"

/*   WiFi client support functions */

bool connectToServer( iWiFiClientInterface *client, char* host, uint16_t port, uint16_t timeout ){

  int result = 0;

  if ( nullptr != client && nullptr != host ) {
    #ifdef EW_SERIAL_LOG
    Log(F("Client: Connecting to "));Log(host);Log(F(" : "));Logln(port);
    #endif
    client->setTimeout(timeout);
    // client->keepAlive();
    result = client->connect( host, port);
  }
  #ifdef EW_SERIAL_LOG
  Log(F("Client: Connect result -: ")); Logln(result);
  #endif
  return result != 0;
}

bool isConnected( iWiFiClientInterface *client ) {

  return ( nullptr != client ) ? client->connected() : false;
}

bool disconnect( iWiFiClientInterface *client ) {

  return isConnected(client) ? client->stop(500) : true;
}

bool sendPacket( iWiFiClientInterface *client, uint8_t *buffer, uint16_t len ) {

  bool status = false;

  if( nullptr != client && nullptr != buffer ){

    uint16_t sentBytes = 0;
    uint16_t _buf_len = strlen((char*)buffer);
    len =  _buf_len < len ? _buf_len : len;
    status = true;

    while (len > 0) {

      #ifdef EW_SERIAL_LOG
      Log(F("Client: Sending to "));Log(client->remoteIP());Log(F(" : "));Logln(client->remotePort());
      #endif
      if ( isConnected(client) ) {
        // send 250 bytes at most at a time, can adjust this later based on Client

        uint16_t sendlen = len > 250 ? 250 : len;
        uint8_t  *_buff_pointer = buffer+sentBytes;
        uint16_t _sent = client->write( _buff_pointer, sendlen);
        sentBytes += _sent;
        len -= _sent;

        #ifdef EW_SERIAL_LOG
        Log(F("Client: sending packets : "));
        for (int i = 0; i < sendlen; i++) {
          Log((char)_buff_pointer[i]);
        }
        Logln();
        Log(F("Client: sent ")); Log(sentBytes); Log(F("/")); Logln(_buf_len);
        #endif

        if (len == 0) {
  	      status = true;
          break;
        }
        if (_sent != sendlen) {
          #ifdef EW_SERIAL_LOG
          Logln(F("Client: send packet - failed to send"));
          #endif
  	      status = false;
          break;
        }
      } else {
        #ifdef EW_SERIAL_LOG
        Logln(F("Client: send packet - connection failed"));
        #endif
        status = false;
        break;
      }
      __i_dvc_ctrl.wait(0);
    }
  }

  return status;
}

uint16_t readPacket( iWiFiClientInterface *client, uint8_t *buffer, uint16_t maxlen, int16_t timeout ) {

  uint16_t len = 0;
  int16_t t = timeout;

  // #ifdef EW_SERIAL_LOG
  // Log(F("Client: Reading from "));Log(client->remoteIP());Log(F(" : "));Logln(client->remotePort());
  // Log(F("Client: Reading packets : "));
  // #endif
  if( nullptr != client && nullptr != buffer ){

    while ( isConnected(client) && ( timeout >= 0 ) ) {

      while ( client->available() ) {

        char c = client->read();
        timeout = t;  // reset the timeout
        buffer[len] = c;
        len++;
        // #ifdef EW_SERIAL_LOG
        // Log(c);
        // #endif

        if( maxlen == 0 ){
          // #ifdef EW_SERIAL_LOG
          // Logln();
          // #endif
          return 0;
        }

        if( len == maxlen ){
          // #ifdef EW_SERIAL_LOG
          // Logln();
          // #endif
          return len;
        }
      }
      timeout -= 1;
      __i_dvc_ctrl.wait(1);
    }
  }
  // #ifdef EW_SERIAL_LOG
  // Logln();
  // #endif
  return len;
}
