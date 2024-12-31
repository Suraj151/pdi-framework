/******************************* Client helper ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_HTTP_CLIENT) || defined(ENABLE_EMAIL_SERVICE)

#include "ClientHelper.h"

/*  client support functions */

bool connectToServer(iClientInterface *client, char *host, uint16_t port, uint16_t timeout)
{
  int result = 0;

  if (nullptr != client && nullptr != host)
  {
    LogFmtI("Client: Connecting to %s : %d\n", host, port);
    client->setTimeout(timeout);
    // client->keepAlive();
    result = client->connect((const uint8_t *)host, port);
  }

  LogFmtI("Client Connect result : %d\n", result);

  return result != 0;
}

bool isConnected(iClientInterface *client)
{
  return (nullptr != client) ? client->connected() : false;
}

bool disconnect(iClientInterface *client)
{
  return isConnected(client) ? client->disconnect() : true;
}

bool sendPacket(iClientInterface *client, uint8_t *buffer, uint16_t len)
{
  bool status = false;

  if (isConnected(client) && nullptr != buffer)
  {
    uint16_t sentBytes = 0;
    uint16_t _buf_len = len; // strlen((char*)buffer);
    // len = _buf_len < len ? _buf_len : len;
    status = true;

    while (len > 0)
    {
      uint16_t sendlen = len > 250 ? 250 : len;
      uint8_t *_buff_pointer = buffer + sentBytes;
      uint16_t _sent = client->write(_buff_pointer, sendlen);
      sentBytes += _sent;
      len -= _sent;

      LogI("Client: sending packets : ");
      for (int i = 0; i < sendlen; i++)
      {
        LogFmtI("%c", (char)_buff_pointer[i]);
      }
      LogFmtI("\nClient: sent %d/%d\n", sentBytes, _buf_len);

      if (len == 0)
      {
        status = true;
        break;
      }

      if (_sent != sendlen)
      {
        LogE("Client: send packet - failed to send\n");
        status = false;
        break;
      }

      __i_dvc_ctrl.wait(0);
    }
  }
  else
  {
    LogE("Client: send packet - notConnected/bufferIsNull\n");
  }

  return status;
}

uint16_t readPacket(iClientInterface *client, uint8_t *buffer, uint16_t maxlen, int32_t timeout, uint8_t readUntilChar)
{

  uint16_t len = 0;
  int32_t t = timeout;

  if (nullptr != client && nullptr != buffer)
  {

    while (t > 0)
    {

      while (client->available() > 0)
      {

        char c = client->read();
        t = timeout; // reset the timeout
        buffer[len] = c;
        len++;

        if (0 != readUntilChar && ((uint8_t)c == readUntilChar))
        {
          return len;
        }

        if (maxlen == 0)
        {
          return 0;
        }

        if (len == maxlen)
        {
          return len;
        }
      }
      t -= 1;
      __i_dvc_ctrl.wait(1);
    }
  }

  return len;
}

#endif