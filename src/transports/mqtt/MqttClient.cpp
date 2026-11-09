/******************************** MQTT File ***********************************
This file is part of the pdi stack. It is written with the reference
of https://github.com/tuanpmt/esp_mqtt


This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#include <config/Config.h>

#if defined(ENABLE_MQTT_SERVICE)

#include "MqttClient.h"

void mqttConnectedCb(uint32_t *args)
{
  LogI("MQTT: connected cb\n");
  MQTTClient *_client = reinterpret_cast<MQTTClient *>(args);
  if (nullptr != _client)
  {
    _client->m_mqttClient.mqtt_connected = true;
  }
}

void mqttDisconnectedCb(uint32_t *args)
{
  LogI("MQTT: disconnect cb\n");
  MQTTClient *_client = reinterpret_cast<MQTTClient *>(args);

  if (nullptr != _client)
  {
    _client->m_mqttClient.mqtt_connected = false;
    _client->clear_all_subscribed_topics();
  }
}

void mqttPublishedCb(uint32_t *args)
{
  LogI("MQTT: published cb\n");
}

void mqttSubscribedCb(uint32_t *args)
{
  LogI("MQTT: subscribed cb\n");
}

void mqttUnsubscribedCb(uint32_t *args)
{
  LogI("MQTT: unsubscribed cb\n");
}

void mqttDataCb(uint32_t *args, const char *topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
  char *topicBuf = pdiutil::safe_new_array<char>(topic_len + 1), *dataBuf = pdiutil::safe_new_array<char>(data_len + 1);

  if (nullptr != topicBuf)
  {
    memcpy(topicBuf, topic, topic_len);
    topicBuf[topic_len] = 0;
  }
  if (nullptr != dataBuf)
  {
    memcpy(dataBuf, data, data_len);
    dataBuf[data_len] = 0;
  }

  if (nullptr != topicBuf && nullptr != dataBuf)
  {
    LogI("MQTT: data cb\nMQTT: Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);
  }

  pdiutil::safe_delete_array(topicBuf);
  pdiutil::safe_delete_array(dataBuf);
}

/**
 * Whether a connected client has a packet queued to send, a qos 1 publish unacknowledged or an answer due.
 */
bool MQTTClient::has_pending_work()
{
  if (!this->is_mqtt_connected())
  {
    return false;
  }
  return MQTT_DATA_SENT == this->m_mqttClient.connState || 0 != this->m_mqttClient.m_inflight ||
         !this->m_mqttClient.m_sendq.empty();
}

bool MQTTClient::is_mqtt_connected()
{

  if (
      this->m_mqttClient.connState == MQTT_DELETING ||
      this->m_mqttClient.connState == MQTT_DISCONNECTED)
  {
    return false;
  }
  return (isConnected(this->m_client) && this->m_mqttClient.mqtt_connected);
}

bool MQTTClient::is_topic_subscribed(char *_topic)
{
  for (uint16_t i = 0; i < this->m_mqttClient.subscribed_topics.size(); i++)
  {
    if (nullptr != this->m_mqttClient.subscribed_topics[i].topic)
    {
      if (__are_str_equals(this->m_mqttClient.subscribed_topics[i].topic, _topic, strlen(this->m_mqttClient.subscribed_topics[i].topic)))
      {
        return true;
      }
    }
  }
  return false;
}

void MQTTClient::clear_all_subscribed_topics()
{
  for (uint16_t i = 0; i < this->m_mqttClient.subscribed_topics.size(); i++)
  {
    pdiutil::safe_delete_array(this->m_mqttClient.subscribed_topics[i].topic);
  }
  this->m_mqttClient.subscribed_topics.clear();
}

void MQTTClient::add_to_subscribed_topics(char *_topic, uint8_t _qos)
{
  int _len = strlen(_topic);
  mqtt_subscribed_topics_t _subscribe_topic;
  _subscribe_topic.topic = pdiutil::safe_new_array<char>(_len + 1);
  if (nullptr != _subscribe_topic.topic)
  {
    strcpy(_subscribe_topic.topic, _topic);
    _subscribe_topic.topic[_len] = 0;
    _subscribe_topic.qos = _qos;

    this->m_mqttClient.subscribed_topics.push_back(_subscribe_topic);
  }
}

/**
 * Queue one packet for sending, refusing rather than making room. A caller
 * that is told no has not sent anything and must not record that it did.
 */
bool MQTTClient::queue_packet(uint8_t *_data, uint16_t _len)
{
  uint8_t *_copy = nullptr;
  if (nullptr != _data && 0 != _len && this->m_mqttClient.m_sendq_bytes + _len <= MQTT_SEND_QUEUE_MAX)
  {
    _copy = pdiutil::safe_new_array<uint8_t>(_len);
  }
  if (nullptr != _copy)
  {
    memcpy(_copy, _data, _len);
    size_t _before = this->m_mqttClient.m_sendq.size();
    this->m_mqttClient.m_sendq.push_back({_copy, _len});
    if (this->m_mqttClient.m_sendq.size() > _before)
    {
      this->m_mqttClient.m_sendq_bytes += _len;
      return true;
    }
    pdiutil::safe_delete_array(_copy);
  }

  SysLogW("MQTT: send queue full, packet refused, queue(%u/%u)\n",
          (unsigned)this->m_mqttClient.m_sendq_bytes, (unsigned)MQTT_SEND_QUEUE_MAX);
  return false;
}

/**
 * Takes an acknowledged qos 1 publish out of the window; false when the id was not waiting.
 */
bool MQTTClient::release_inflight(uint16_t _id)
{
  for (uint8_t i = 0; i < this->m_mqttClient.m_inflight; i++)
  {
    if (this->m_mqttClient.m_inflight_ids[i] == _id)
    {
      this->m_mqttClient.m_inflight--;
      this->m_mqttClient.m_inflight_ids[i] = this->m_mqttClient.m_inflight_ids[this->m_mqttClient.m_inflight];
      this->m_mqttClient.m_inflight_tick = 0;
      return true;
    }
  }
  return false;
}

/**
 * Releases every packet still waiting to be sent.
 */
void MQTTClient::drop_queue(void)
{
  for (uint32_t i = 0; i < this->m_mqttClient.m_sendq.size(); i++)
  {
    pdiutil::safe_delete_array(this->m_mqttClient.m_sendq[i].m_data);
  }
  this->m_mqttClient.m_sendq.clear();
  this->m_mqttClient.m_sendq_bytes = 0;
}

bool MQTTClient::remove_from_subscribed_topics(char *_topic)
{
  for (uint16_t i = 0; i < this->m_mqttClient.subscribed_topics.size(); i++)
  {
    if (nullptr != this->m_mqttClient.subscribed_topics[i].topic)
    {
      if (__are_str_equals(this->m_mqttClient.subscribed_topics[i].topic, _topic, strlen(this->m_mqttClient.subscribed_topics[i].topic)))
      {
        pdiutil::safe_delete_array(this->m_mqttClient.subscribed_topics[i].topic);
        this->m_mqttClient.subscribed_topics[i].topic = nullptr;
        this->m_mqttClient.subscribed_topics.erase(this->m_mqttClient.subscribed_topics.begin() + i);
        return true;
      }
    }
  }
  return false;
}

void MQTTClient::deliver_publish(uint8_t *message, size_t length)
{

  mqtt_event_data_t event_data;

  event_data.topic_length = length;
  event_data.topic = (char *)mqtt_get_publish_topic(message, &event_data.topic_length);
  event_data.data_length = length;
  event_data.data = (char *)mqtt_get_publish_data(message, &event_data.data_length);

  if (nullptr == event_data.topic || nullptr == event_data.data)
  {
    return;
  }

  if (nullptr != this->m_dataCb)
  {
    this->m_dataCb(this->m_mqttDataCallbackArgs, event_data.topic, event_data.topic_length, event_data.data, event_data.data_length);
  }
  else
  {
    mqttDataCb(this->m_mqttDataCallbackArgs, event_data.topic, event_data.topic_length, event_data.data, event_data.data_length);
  }
}

// void mqtt_wificlient_delete( MQTT_Client *m_mqttClient ){
//
//     if ( m_mqttClient->m_client != nullptr) {
//         m_mqttClient->m_client->stopAll();
//         delete m_mqttClient->m_client;
//         m_mqttClient->m_client = nullptr;
//     }
// }

/**
 * Reads every packet already waiting, up to a bound, so a burst is not taken one per tick.
 */
void MQTTClient::mqtt_client_recv_waiting()
{
  for (uint8_t i = 0; i < MQTT_RECV_PACKETS_PER_TASK; i++)
  {
    this->mqtt_client_recv();

    if (MQTT_DATA != this->m_mqttClient.connState || nullptr == this->m_client || this->m_client->available() < 1)
    {
      return;
    }
  }
}

void MQTTClient::mqtt_client_recv()
{
  if (nullptr == this->m_mqttClient.mqtt_state.in_buffer)
  {
    return;
  }

  uint8_t *_standing = this->m_mqttClient.mqtt_state.in_buffer;
  size_t _standing_length = this->m_mqttClient.mqtt_state.in_buffer_length;
  uint8_t *_wide = nullptr;
  int len = this->readFullPacket(_standing, _standing_length, MQTT_PACKET_READ_WAIT_MS, &_wide);
  if (nullptr != _wide)
  {
    this->m_mqttClient.mqtt_state.in_buffer = _wide;
    this->m_mqttClient.mqtt_state.in_buffer_length = len;
  }

  LogI("MQTT: recieved packet size : %d\n", len);
  LogI("MQTT: recieved packets : ");
  for (int i = 0; i < len; i++)
  {
    LogI("%c", this->m_mqttClient.mqtt_state.in_buffer[i]);
    __i_dvc_ctrl.wait(0);
  }
  LogI("\n");

  __i_dvc_ctrl.wait(0);

  if (len > 0 && (size_t)len <= this->m_mqttClient.mqtt_state.in_buffer_length)
  {
    uint8_t msg_type = mqtt_get_type(this->m_mqttClient.mqtt_state.in_buffer);
    uint8_t msg_qos = mqtt_get_qos(this->m_mqttClient.mqtt_state.in_buffer);
    uint16_t msg_id = mqtt_get_id(this->m_mqttClient.mqtt_state.in_buffer, this->m_mqttClient.mqtt_state.in_buffer_length);

    if (!(MQTT_MSG_TYPE_PUBLISH == msg_type && 0 == msg_qos))
    {
      this->m_mqttClient.keepAliveTick = 0;
      this->m_mqttClient.readTimeout = 0;
      this->m_mqttClient.sendTimeout = MQTT_MS_TO_TICKS(MQTT_SEND_TIMEOUT_MS);
    }

    LogI("MQTT: recieved msg type(%d), qos(%d)\n", msg_type, msg_qos);

    switch (this->m_mqttClient.connState)
    {
    case MQTT_CONNECT_SENT:

      if (MQTT_MSG_TYPE_CONNACK == msg_type)
      {
        uint8_t connack_code = mqtt_get_connect_return_code(this->m_mqttClient.mqtt_state.in_buffer, len);

        if (MQTT_MSG_TYPE_CONNECT != this->m_mqttClient.mqtt_state.pending_msg_type)
        {
          SysLogE("MQTT: Invalid packet recieved\n");
          this->m_mqttClient.connState = MQTT_HOST_RECONNECT_REQ;
        }
        else if (MQTT_CONNACK_ACCEPTED != connack_code)
        {
          SysLogE("MQTT: connection refused by broker, code %d\n", (int)connack_code);
          this->m_mqttClient.connState = MQTT_CONNECT_FAILED;
          this->m_mqttClient.host_connect_tick = 0;
          this->disconnectServer();
        }
        else
        {
          LogI("MQTT: CONNACK recieved\n");
          this->m_mqttClient.connState = MQTT_DATA;
          if (nullptr != this->m_connectedCb)
          {
            this->m_connectedCb((uint32_t *)this);
          }
        }
      }
      break;
    case MQTT_DISCONNECT_SENT:
      // ignore
      if (nullptr != this->m_disconnectedCb)
      {
        this->m_disconnectedCb((uint32_t *)this);
      }
      this->m_mqttClient.connState = MQTT_DISCONNECTED;
      break;
    case MQTT_DATA_SENT:
    case MQTT_PING_SENT:
    case MQTT_DATA:
    case MQTT_KEEPALIVE_REQ:

      this->m_mqttClient.mqtt_state.message_length_read = len;
      this->m_mqttClient.mqtt_state.message_length = mqtt_get_total_length(this->m_mqttClient.mqtt_state.in_buffer, this->m_mqttClient.mqtt_state.message_length_read);
      this->m_mqttClient.connState = MQTT_DATA;

      switch (msg_type)
      {

      case MQTT_MSG_TYPE_SUBACK:

        if (MQTT_MSG_TYPE_SUBSCRIBE == this->m_mqttClient.mqtt_state.pending_msg_type && this->m_mqttClient.mqtt_state.pending_msg_id == msg_id)
        {
          LogS("MQTT: Subscribe successful\n");
        }
        if (nullptr != this->m_subscribedCb)
        {
          this->m_subscribedCb((uint32_t *)this);
        }
        break;

      case MQTT_MSG_TYPE_UNSUBACK:

        if (MQTT_MSG_TYPE_UNSUBSCRIBE == this->m_mqttClient.mqtt_state.pending_msg_type && this->m_mqttClient.mqtt_state.pending_msg_id == msg_id)
        {
          LogS("MQTT: UnSubscribe successful\n");
        }
        if (nullptr != this->m_unsubscribedCb)
        {
          this->m_unsubscribedCb((uint32_t *)this);
        }
        break;

      case MQTT_MSG_TYPE_PUBLISH:

        if (1 == msg_qos)
        {
          this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_puback(&this->m_mqttClient.mqtt_state.mqtt_connection, msg_id);
        }
        else if (2 == msg_qos)
        {
          this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_pubrec(&this->m_mqttClient.mqtt_state.mqtt_connection, msg_id);
        }
        if (1 == msg_qos || 2 == msg_qos)
        {
          LogI("MQTT: Queue response QoS: %d\r\n", msg_qos);
          if (!this->queue_packet(this->m_mqttClient.mqtt_state.outbound_message->data,
                                  this->m_mqttClient.mqtt_state.outbound_message->length))
          {
          }
        }

        this->deliver_publish(this->m_mqttClient.mqtt_state.in_buffer, this->m_mqttClient.mqtt_state.message_length_read);
        break;

      case MQTT_MSG_TYPE_PUBACK:

        if (this->release_inflight(msg_id))
        {
          LogI("MQTT: received MQTT_MSG_TYPE_PUBACK, finish QoS1 publish\n");
          if (nullptr != this->m_publishedCb)
          {
            this->m_publishedCb((uint32_t *)this);
          }
        }

        break;

      case MQTT_MSG_TYPE_PUBREC:

        this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_pubrel(&this->m_mqttClient.mqtt_state.mqtt_connection, msg_id);

        this->queue_packet(this->m_mqttClient.mqtt_state.outbound_message->data,
                           this->m_mqttClient.mqtt_state.outbound_message->length);
        break;

      case MQTT_MSG_TYPE_PUBREL:

        this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_pubcomp(&this->m_mqttClient.mqtt_state.mqtt_connection, msg_id);

        this->queue_packet(this->m_mqttClient.mqtt_state.outbound_message->data,
                           this->m_mqttClient.mqtt_state.outbound_message->length);
        break;

      case MQTT_MSG_TYPE_PUBCOMP:

        if (MQTT_MSG_TYPE_PUBLISH == this->m_mqttClient.mqtt_state.pending_msg_type && this->m_mqttClient.mqtt_state.pending_msg_id == msg_id)
        {
          LogI("MQTT: received MQTT_MSG_TYPE_PUBCOMP, finish QoS2 publish\n");
          if (nullptr != this->m_publishedCb)
          {
            this->m_publishedCb((uint32_t *)this);
          }
        }
        break;

      case MQTT_MSG_TYPE_PINGREQ:

        this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_pingresp(&this->m_mqttClient.mqtt_state.mqtt_connection);

        this->queue_packet(this->m_mqttClient.mqtt_state.outbound_message->data,
                           this->m_mqttClient.mqtt_state.outbound_message->length);
        break;

      case MQTT_MSG_TYPE_PINGRESP:
        LogI("MQTT: MQTT_MSG_TYPE_PINGRESP recieved\n");
        this->m_mqttClient.m_ping_awaited = false;
        break;
      default:
        LogI("MQTT: Uknown msg type received.\n");
        break;
      }
      break;
    default:
      LogW("MQTT: Uknown msg type received.\n");
      break;
    }
  }
  else if (0 == len)
  {
    LogW("MQTT: No response received yet\n");
    if(this->m_mqttClient.connState == MQTT_PING_SENT)  // Go for the pending data queue if did not received ping resp yet
      this->m_mqttClient.connState = MQTT_DATA;
  }
  else
  {
    LogW("MQTT: response too long\n");
  }
  this->m_mqttClient.mqtt_state.in_buffer = _standing;
  this->m_mqttClient.mqtt_state.in_buffer_length = _standing_length;
  pdiutil::safe_delete_array(_wide);
  // if( this->m_mqttClient.connState == MQTT_DATA ) this->MQTT_Task();
}

void MQTTClient::mqtt_client_connect()
{

  mqtt_msg_init(&this->m_mqttClient.mqtt_state.mqtt_connection, this->m_mqttClient.mqtt_state.out_buffer, this->m_mqttClient.mqtt_state.out_buffer_length);
  this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_connect(&this->m_mqttClient.mqtt_state.mqtt_connection, this->m_mqttClient.mqtt_state.connect_info);
  this->m_mqttClient.mqtt_state.pending_msg_type = mqtt_get_type(this->m_mqttClient.mqtt_state.outbound_message->data);
  this->m_mqttClient.mqtt_state.pending_msg_id = mqtt_get_id(this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length);

  this->m_mqttClient.sendTimeout = MQTT_MS_TO_TICKS(MQTT_SEND_TIMEOUT_MS);
  LogI("MQTT: Sending, type: %d, id: %x\r\n", this->m_mqttClient.mqtt_state.pending_msg_type, this->m_mqttClient.mqtt_state.pending_msg_id);
  bool result = sendPacket(this->m_client, this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length);

  if (result)
  {
    LogI("MQTT: connect packet sent\n");
    this->m_mqttClient.connState = MQTT_CONNECT_SENT;
    this->m_mqttClient.readTimeout = 0;
  }
  else
  {
    SysLogE("MQTT: connect packet send failed\n");
    this->m_mqttClient.connState = MQTT_CONNECT_FAILED;
    this->m_mqttClient.host_connect_tick = 0;
    this->disconnectServer();
  }
  this->m_mqttClient.mqtt_state.outbound_message = nullptr;
  // this->MQTT_Task();
}

void MQTTClient::mqtt_client_disconnect()
{
  this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_disconnect(&this->m_mqttClient.mqtt_state.mqtt_connection);
  this->m_mqttClient.mqtt_state.pending_msg_type = mqtt_get_type(this->m_mqttClient.mqtt_state.outbound_message->data);
  this->m_mqttClient.mqtt_state.pending_msg_id = mqtt_get_id(this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length);

  this->m_mqttClient.sendTimeout = MQTT_MS_TO_TICKS(MQTT_SEND_TIMEOUT_MS);
  LogI("MQTT: Sending, type: %d, id: %x\r\n", this->m_mqttClient.mqtt_state.pending_msg_type, this->m_mqttClient.mqtt_state.pending_msg_id);
  bool result = sendPacket(this->m_client, this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length);

  if (result)
  {
    LogI("MQTT: disconnect packet sent\n");
    this->m_mqttClient.connState = MQTT_DISCONNECT_SENT;
    this->m_mqttClient.readTimeout = 0;
  }
  else
  {
    SysLogE("MQTT: disconnect packet send failed\n");
    this->m_mqttClient.connState = MQTT_DISCONNECT_FAILED;
    this->m_mqttClient.host_connect_tick = 0;
  }
  this->disconnectServer();
  this->m_mqttClient.mqtt_state.outbound_message = nullptr;
}

void MQTTClient::mqtt_send_keepalive()
{
  LogI("\r\nMQTT: Send keepalive packet to %s:%d!\r\n", this->m_host, this->m_port);
  this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_pingreq(&this->m_mqttClient.mqtt_state.mqtt_connection);
  // this->m_mqttClient.mqtt_state.pending_msg_type = MQTT_MSG_TYPE_PINGREQ;
  this->m_mqttClient.mqtt_state.pending_msg_type = mqtt_get_type(this->m_mqttClient.mqtt_state.outbound_message->data);
  this->m_mqttClient.mqtt_state.pending_msg_id = mqtt_get_id(this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length);

  this->m_mqttClient.sendTimeout = MQTT_MS_TO_TICKS(MQTT_SEND_TIMEOUT_MS);
  LogI("MQTT: Sending, type: %d, id: %x\r\n", this->m_mqttClient.mqtt_state.pending_msg_type, this->m_mqttClient.mqtt_state.pending_msg_id);
  bool result = sendPacket(this->m_client, this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length);

  if (result)
  {
    this->m_mqttClient.keepAliveTick = 0;
    this->m_mqttClient.readTimeout = 0;
    if (!this->m_mqttClient.m_ping_awaited)
    {
      this->m_mqttClient.m_ping_wait_tick = 0;
      this->m_mqttClient.m_ping_awaited = true;
    }
    this->m_mqttClient.connState = MQTT_PING_SENT;
  }
  else
  {
    this->m_mqttClient.host_connect_tick = 0;
    this->m_mqttClient.connState = MQTT_PING_FAILED;
    this->disconnectServer();
  }
  // this->MQTT_Task();
  this->m_mqttClient.mqtt_state.outbound_message = nullptr;
}

/**
 * Sends what the queue holds, up to MQTT_SEND_PACKETS_PER_TASK packets and only while no answer
 * from the broker is due; a packet the connection has no room for yet stays queued.
 */
void MQTTClient::send_queued()
{
  for (uint8_t _sent = 0; _sent < MQTT_SEND_PACKETS_PER_TASK && MQTT_DATA == this->m_mqttClient.connState &&
                          !this->m_mqttClient.m_sendq.empty(); _sent++)
  {
    mqtt_packet_t _packet = this->m_mqttClient.m_sendq[0];
    bool _windowed = MQTT_MSG_TYPE_PUBLISH == mqtt_get_type(_packet.m_data) && 1 == mqtt_get_qos(_packet.m_data);
    if ((_windowed && this->m_mqttClient.m_inflight >= MQTT_INFLIGHT_MAX) || !this->send_room_for(_packet.m_len))
    {
      return;
    }
    this->m_mqttClient.m_sendq.erase(this->m_mqttClient.m_sendq.begin());
    this->m_mqttClient.m_sendq_bytes -= _packet.m_len;
    uint8_t *dataBuffer = _packet.m_data;
    uint16_t dataLen = _packet.m_len;
    LogI("MQTT: getting queue packets :");
    for (int i = 0; i < dataLen; i++)
    {
      LogI("%c", (char)dataBuffer[i]);
      __i_dvc_ctrl.wait(0);
    }
    LogI("\n");

    uint8_t msg_qos = mqtt_get_qos(dataBuffer);
    uint8_t msg_type = mqtt_get_type(dataBuffer);

    this->m_mqttClient.mqtt_state.pending_msg_type = msg_type;
    this->m_mqttClient.mqtt_state.pending_msg_id = mqtt_get_id(dataBuffer, dataLen);

    this->m_mqttClient.sendTimeout = MQTT_MS_TO_TICKS(MQTT_SEND_TIMEOUT_MS);
    bool result = sendPacket(this->m_client, dataBuffer, dataLen);

    if (result)
    {
      LogI("MQTT: data packet sent of id: %d\n", this->m_mqttClient.mqtt_state.pending_msg_id);
      bool _answered = (MQTT_MSG_TYPE_PUBLISH == msg_type) ? (2 == msg_qos) :
                       (MQTT_MSG_TYPE_PUBREL == msg_type || MQTT_MSG_TYPE_SUBSCRIBE == msg_type ||
                        MQTT_MSG_TYPE_UNSUBSCRIBE == msg_type || MQTT_MSG_TYPE_PINGREQ == msg_type);
      if (_windowed)
      {
        if (0 == this->m_mqttClient.m_inflight)
        {
          this->m_mqttClient.m_inflight_tick = 0;
        }
        this->m_mqttClient.m_inflight_ids[this->m_mqttClient.m_inflight++] = this->m_mqttClient.mqtt_state.pending_msg_id;
      }
      this->m_mqttClient.connState = _answered ? MQTT_DATA_SENT : MQTT_DATA;
      this->m_mqttClient.readTimeout = 0;
    }
    else
    {
      SysLogE("MQTT: data packet send failed\n");
      this->m_mqttClient.connState = MQTT_DATA_FAILED;
      this->m_mqttClient.host_connect_tick = 0;
      this->disconnectServer();
    }

    this->m_mqttClient.mqtt_state.outbound_message = nullptr;
    pdiutil::safe_delete_array(_packet.m_data);
  }
}

/**
 * Whether the connection takes the first piece of a packet within MQTT_SEND_ROOM_WAIT_MS; once it
 * has taken nothing for MQTT_SEND_TIMEOUT_MS the session is ended as a failed send.
 */
bool MQTTClient::send_room_for(uint16_t _len)
{
  if (nullptr == this->m_client)
  {
    return false;
  }
  uint16_t _piece = pdistd::min((uint16_t)TCP_WRITE_CHUNK_MAX, _len);
  uint32_t _began = __i_dvc_ctrl.millis_now();
  bool _room = this->m_client->availableforwrite(_piece);
  while (!_room && (__i_dvc_ctrl.millis_now() - _began) < MQTT_SEND_ROOM_WAIT_MS)
  {
    __i_dvc_ctrl.yield();
    _room = this->m_client->availableforwrite(_piece);
  }
  if (_room)
  {
    this->m_mqttClient.m_send_stalled = false;
    return true;
  }

  uint32_t _now = __i_dvc_ctrl.millis_now();
  if (!this->m_mqttClient.m_send_stalled)
  {
    this->m_mqttClient.m_send_stalled = true;
    this->m_mqttClient.m_send_stalled_ms = _now;
  }
  else if ((_now - this->m_mqttClient.m_send_stalled_ms) >= MQTT_SEND_TIMEOUT_MS)
  {
    SysLogE("MQTT: the connection took nothing for %u ms, reconnecting\n", (unsigned)MQTT_SEND_TIMEOUT_MS);
    this->m_mqttClient.m_send_stalled = false;
    this->m_mqttClient.connState = MQTT_DATA_FAILED;
    this->m_mqttClient.host_connect_tick = 0;
    this->disconnectServer();
  }
  return false;
}

void MQTTClient::MQTT_Task()
{

  LogI("MQTT: Task %d\n", (int)this->m_mqttClient.connState);

  switch (this->m_mqttClient.connState)
  {
  default:
    break;
  case MQTT_HOST_RECONNECT_REQ:
  case MQTT_HOST_RECONNECT:
    this->disconnectServer();
    this->Connect();
    break;
  case MQTT_CONNECT_SENT:
  case MQTT_DISCONNECT_SENT:
  case MQTT_DATA_SENT:
  case MQTT_PING_SENT:
    this->mqtt_client_recv_waiting();
    if (MQTT_DATA == this->m_mqttClient.connState && isConnected(this->m_client))
    {
      this->send_queued();
    }
    break;
  case MQTT_HOST_CONNECTING:
    if (isConnected(this->m_client))
    {
      LogI("MQTT: Connected to broker %s:%d\r\n", this->m_host, this->m_port);
      this->mqtt_client_connect();
    }else{
      SysLogE("MQTT: Unable to connect to broker\r\n");
    }
    break;
  case MQTT_DISCONNECT_REQ:
    this->mqtt_client_disconnect();
    break;
  case MQTT_KEEPALIVE_REQ:
    this->mqtt_send_keepalive();
    break;
  case MQTT_DATA:
    if (!isConnected(this->m_client))
    {
      SysLogE("MQTT: broker closed the connection\n");
      this->m_mqttClient.connState = MQTT_HOST_RECONNECT_REQ;
      break;
    }
    // if ( QUEUE_IsEmpty(&this->m_mqttClient.msgQueue) || this->m_mqttClient.sendTimeout != 0 ) {
    this->mqtt_client_recv_waiting();
    this->send_queued();
    break;
  }
  __i_dvc_ctrl.yield(); // yield purpose
}

void MQTTClient::mqtt_client_delete()
{

  // mqtt_wificlient_delete(m_mqttClient);

  // if (this->m_mqttClient.user_data != nullptr) {
  //   delete this->m_mqttClient.user_data;
  //   this->m_mqttClient.user_data = nullptr;
  // }
  pdiutil::safe_delete_array(this->m_host);

  pdiutil::safe_delete_array(this->m_mqttClient.connect_info.client_id);

  pdiutil::safe_delete_array(this->m_mqttClient.connect_info.username);

  pdiutil::safe_delete_array(this->m_mqttClient.connect_info.password);

  pdiutil::safe_delete_array(this->m_mqttClient.connect_info.will_topic);

  pdiutil::safe_delete_array(this->m_mqttClient.connect_info.will_message);

  pdiutil::safe_delete_array(this->m_mqttClient.mqtt_state.in_buffer);

  pdiutil::safe_delete_array(this->m_mqttClient.mqtt_state.out_buffer);

  this->drop_queue();

  if (nullptr != this->m_client)
  {
    // delete this->m_client;
    this->m_client->flush(FLUSH_ALL);
    this->m_client->close();
    this->m_client = nullptr;
  }

  this->clear_all_subscribed_topics();
}

bool MQTTClient::Subscribe(char *topic, uint8_t qos)
{

  uint8_t dataBuffer[MQTT_BUF_SIZE];
  uint16_t dataLen;
  memset(dataBuffer, 0, MQTT_BUF_SIZE);

  if (!isConnected(this->m_client))
  {
    return false;
  }

  this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_subscribe(&this->m_mqttClient.mqtt_state.mqtt_connection,
                                                                      topic, qos,
                                                                      &this->m_mqttClient.mqtt_state.pending_msg_id);

  LogI("MQTT: queue subscribe, topic \"%s\", id: %d\r\n", topic, this->m_mqttClient.mqtt_state.pending_msg_id);
  // bool result = sendPacket(this->m_client, this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length );

  if (!this->queue_packet(this->m_mqttClient.mqtt_state.outbound_message->data,
                          this->m_mqttClient.mqtt_state.outbound_message->length))
  {
    return false;
  }

  if (!this->is_topic_subscribed(topic))
  {
    this->add_to_subscribed_topics(topic, qos);
  }
  this->m_mqttClient.sendTimeout = MQTT_MS_TO_TICKS(MQTT_SEND_TIMEOUT_MS);
  return true;
}

bool MQTTClient::UnSubscribe(char *topic)
{

  uint8_t dataBuffer[MQTT_BUF_SIZE];
  uint16_t dataLen;
  memset(dataBuffer, 0, MQTT_BUF_SIZE);

  this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_unsubscribe(&this->m_mqttClient.mqtt_state.mqtt_connection,
                                                                        topic,
                                                                        &this->m_mqttClient.mqtt_state.pending_msg_id);

  LogI("MQTT: queue un-subscribe, topic \"%s\", id: %d\r\n", topic, this->m_mqttClient.mqtt_state.pending_msg_id);

  if (!this->queue_packet(this->m_mqttClient.mqtt_state.outbound_message->data,
                          this->m_mqttClient.mqtt_state.outbound_message->length))
  {
    return false;
  }

  this->remove_from_subscribed_topics(topic);
  this->m_mqttClient.sendTimeout = MQTT_MS_TO_TICKS(MQTT_SEND_TIMEOUT_MS);
  return true;
}

bool MQTTClient::Publish(const char *topic, const char *data, size_t data_length, uint8_t qos, uint8_t retain)
{

  if (!isConnected(this->m_client))
  {
    return false;
  }

  mqtt_connection_t *_connection = &this->m_mqttClient.mqtt_state.mqtt_connection;
  mqtt_connection_t _wide;
  uint8_t *_widebuf = nullptr;
  uint32_t _needed = mqtt_publish_length(topic, data_length, qos);
  if (_needed > _connection->buffer_length)
  {
    size_t _granted = 0;
    if (_needed <= MQTT_PACKET_MAX)
    {
      _widebuf = pdiutil::safe_new_array_upto<uint8_t>(_needed, _needed, _granted);
    }
    if (nullptr == _widebuf)
    {
      SysLogE("MQTT: a %u byte publish does not fit, limit %u\n", (unsigned)_needed, (unsigned)MQTT_PACKET_MAX);
      return false;
    }
    _wide = *_connection;
    _wide.buffer = _widebuf;
    _wide.buffer_length = (uint16_t)_needed;
    _connection = &_wide;
  }

  // LogI("MQTT: publish, topic: %s, data: %s, datalen: %d, qos: %d, retain: %d, pendmsg: %d\r\n", topic, data, data_length, qos, retain, this->m_mqttClient.mqtt_state.pending_msg_id);
  mqtt_message_t *_message = mqtt_msg_publish(_connection,
                                              topic, data, data_length,
                                              qos, retain,
                                              &this->m_mqttClient.mqtt_state.pending_msg_id);
  this->m_mqttClient.mqtt_state.mqtt_connection.message_id = _connection->message_id;
  this->m_mqttClient.mqtt_state.outbound_message = (_connection == &_wide) ? nullptr : _message;

  bool _queued = false;
  if (nullptr == _message || 0 == _message->length)
  {
    SysLogE("MQTT: Queuing publish failed\n");
  }
  else
  {
    LogI("MQTT: queuing publish, length: %d, queue size(%u/%u)\r\n", _message->length, (unsigned)this->m_mqttClient.m_sendq_bytes, (unsigned)MQTT_SEND_QUEUE_MAX);
    _queued = this->queue_packet(_message->data, _message->length);
  }
  pdiutil::safe_delete_array(_widebuf);

  if (!_queued)
  {
    return false;
  }
  this->m_mqttClient.sendTimeout = MQTT_MS_TO_TICKS(MQTT_SEND_TIMEOUT_MS);
  return true;
}

MQTTClient::MQTTClient() : m_mqttClient(),
                           m_mqttDataCallbackArgs(reinterpret_cast<uint32_t *>(this)),
                           m_host(nullptr),
                           m_port(0),
                           m_security(0),
                           m_client(nullptr),
                           m_connectedCb(nullptr),
                           m_disconnectedCb(nullptr),
                           m_publishedCb(nullptr),
                           m_subscribedCb(nullptr),
                           m_unsubscribedCb(nullptr),
                           m_timeoutCb(nullptr),
                           m_dataCb(nullptr)
{
}

MQTTClient::~MQTTClient()
{
  this->mqtt_client_delete();
}

bool MQTTClient::begin(iClientInterface *_client, mqtt_general_config_table *_mqtt_general_configs, mqtt_lwt_config_table *_mqtt_lwt_configs)
{
  if (_client == nullptr || nullptr == _mqtt_general_configs || nullptr == _mqtt_lwt_configs)
  {
    return false;
  }

  DeleteClient();

  m_client = _client;

  if (strlen(_mqtt_general_configs->host) > 5 && _mqtt_general_configs->port > 0)
  {
    this->InitConnection(_mqtt_general_configs->host, _mqtt_general_configs->port, _mqtt_general_configs->security);
  }
  else
  {
    return false;
  }

  if (strlen(_mqtt_general_configs->client_id) > 0 && _mqtt_general_configs->keepalive > 5)
  {
    this->InitClient(
        _mqtt_general_configs->client_id,
        _mqtt_general_configs->username,
        _mqtt_general_configs->password,
        _mqtt_general_configs->keepalive,
        _mqtt_general_configs->clean_session);
  }
  else
  {
    SysLogW("MQTT: not connecting, a client id and a keepalive above 5 s are needed (keepalive %u)\n", (unsigned)_mqtt_general_configs->keepalive);
    this->mqtt_client_delete();
    return false;
  }

  if (strlen(_mqtt_lwt_configs->will_topic) > 0)
  {
    this->InitLWT(
        _mqtt_lwt_configs->will_topic,
        _mqtt_lwt_configs->will_message,
        _mqtt_lwt_configs->will_qos < MQTT_MAX_QOS_LEVEL ? _mqtt_lwt_configs->will_qos : MQTT_MAX_QOS_LEVEL,
        _mqtt_lwt_configs->will_retain);
  }

  LogI("MQTT: Connecting\n");

  this->OnConnected(mqttConnectedCb);
  this->OnDisconnected(mqttDisconnectedCb);
  this->OnPublished(mqttPublishedCb);
  this->OnSubscribed(mqttSubscribedCb);
  this->OnUnsubscribed(mqttUnsubscribedCb);
  // this->OnData( mqttDataCb );

  this->Connect();
  return true;
}

void MQTTClient::InitConnection(char *host, pdiutil::net_port_t port, uint8_t security)
{

  LogI("MQTT: InitConnection\n");

  int _host_len = strlen(host);
  memset(&this->m_mqttClient.mqtt_state, 0, sizeof(mqtt_state_t));
  memset(&this->m_mqttClient.connect_info, 0, sizeof(mqtt_connect_info_t));
  this->drop_queue();
  this->m_mqttClient.keepAliveTick = 0;
  this->m_mqttClient.sendTimeout = 0;
  this->m_mqttClient.readTimeout = 0;
  this->m_mqttClient.connState = MQTT_PUBLISH_RECV;
  this->m_mqttClient.host_connect_tick = 0;
  this->m_mqttClient.m_ping_wait_tick = 0;
  this->m_mqttClient.m_ping_awaited = false;
  this->m_mqttClient.m_send_stalled = false;
  this->m_mqttClient.m_inflight = 0;
  this->m_mqttClient.mqtt_connected = false;
  this->m_host = pdiutil::safe_new_array<char>(_host_len + 1);

  if (nullptr != this->m_host)
  {
    memset(this->m_host, 0, _host_len + 1);
    strcpy(this->m_host, host);
    this->m_host[_host_len] = 0;
  }

  this->m_port = port;
  this->m_security = security;
}

void MQTTClient::InitClient(char *client_id, char *client_user, char *client_pass, uint32_t keepAliveTime, uint8_t cleanSession)
{

  uint32_t temp;
  int _len = 0;

  LogI("MQTT: InitClient\n");

  memset(&this->m_mqttClient.connect_info, 0, sizeof(mqtt_connect_info_t));

  if (nullptr != client_id)
  {
    _len = strlen(client_id);
    this->m_mqttClient.connect_info.client_id = pdiutil::safe_new_array<char>(_len + 1);
    if (nullptr != this->m_mqttClient.connect_info.client_id)
    {
      strcpy(this->m_mqttClient.connect_info.client_id, client_id);
      this->m_mqttClient.connect_info.client_id[_len] = 0;
    }
  }

  if (nullptr != client_user)
  {
    _len = strlen(client_user);
    this->m_mqttClient.connect_info.username = pdiutil::safe_new_array<char>(_len + 1);
    if (nullptr != this->m_mqttClient.connect_info.username)
    {
      strcpy(this->m_mqttClient.connect_info.username, client_user);
      this->m_mqttClient.connect_info.username[_len] = 0;
    }
  }

  if (nullptr != client_pass)
  {
    _len = strlen(client_pass);
    this->m_mqttClient.connect_info.password = pdiutil::safe_new_array<char>(_len + 1);
    if (nullptr != this->m_mqttClient.connect_info.password)
    {
      strcpy(this->m_mqttClient.connect_info.password, client_pass);
      this->m_mqttClient.connect_info.password[_len] = 0;
    }
  }

  this->m_mqttClient.connect_info.keepalive = keepAliveTime > 5 ? keepAliveTime : MQTT_DEFAULT_KEEPALIVE;
  this->m_mqttClient.connect_info.clean_session = cleanSession;

  this->m_mqttClient.mqtt_state.in_buffer = pdiutil::safe_new_array<uint8_t>(MQTT_BUF_SIZE);
  if (nullptr != this->m_mqttClient.mqtt_state.in_buffer)
  {
    this->m_mqttClient.mqtt_state.in_buffer_length = MQTT_BUF_SIZE;
  }
  this->m_mqttClient.mqtt_state.out_buffer = pdiutil::safe_new_array<uint8_t>(MQTT_BUF_SIZE);
  if (nullptr != this->m_mqttClient.mqtt_state.out_buffer)
  {
    this->m_mqttClient.mqtt_state.out_buffer_length = MQTT_BUF_SIZE;
  }
  this->m_mqttClient.mqtt_state.connect_info = &this->m_mqttClient.connect_info;

  mqtt_msg_init(&this->m_mqttClient.mqtt_state.mqtt_connection, this->m_mqttClient.mqtt_state.out_buffer, this->m_mqttClient.mqtt_state.out_buffer_length);

  // this->MQTT_Task();
}

void MQTTClient::InitLWT(char *will_topic, char *will_msg, uint8_t will_qos, uint8_t will_retain)
{

  int _len = 0;

  LogI("MQTT: InitLWT\n");

  if (nullptr != will_topic)
  {
    _len = strlen(will_topic);
    this->m_mqttClient.connect_info.will_topic = pdiutil::safe_new_array<char>(_len + 1);
    if (nullptr != this->m_mqttClient.connect_info.will_topic)
    {
      strcpy(this->m_mqttClient.connect_info.will_topic, will_topic);
      this->m_mqttClient.connect_info.will_topic[_len] = 0;
    }
  }

  if (nullptr != will_msg)
  {
    _len = strlen(will_msg);
    this->m_mqttClient.connect_info.will_message = pdiutil::safe_new_array<char>(_len + 1);
    if (nullptr != this->m_mqttClient.connect_info.will_message)
    {
      strcpy(this->m_mqttClient.connect_info.will_message, will_msg);
      this->m_mqttClient.connect_info.will_message[_len] = 0;
    }
  }

  this->m_mqttClient.connect_info.will_qos = will_qos;
  this->m_mqttClient.connect_info.will_retain = will_retain;
}

void MQTTClient::mqtt_timer()
{

  LogI("MQTT: mqtt timer : %d\t mqtt state : %d\n", (int)isConnected(this->m_client), (int)this->m_mqttClient.connState);
  LogI("MQTT: subscribed topics(QoS) : ");
  for (uint16_t i = 0; i < this->m_mqttClient.subscribed_topics.size(); i++)
  {
    LogI("%s(%d), ", this->m_mqttClient.subscribed_topics[i].topic, this->m_mqttClient.subscribed_topics[i].qos);
  }
  LogI("\n");
  // if( this->m_mqttClient.connState == MQTT_DELETING ) return;

  uint32_t _now = __i_dvc_ctrl.millis_now();
  bool _tick = (_now - this->m_mqttClient.m_last_tick_ms) >= MQTT_TIMER_INTERVAL_MS;
  if (_tick)
  {
    this->m_mqttClient.m_last_tick_ms = _now;
  }

  if (this->m_mqttClient.m_ping_awaited && _tick &&
      ++this->m_mqttClient.m_ping_wait_tick > MQTT_MS_TO_TICKS(MQTT_READ_TIMEOUT_MS))
  {
    this->m_mqttClient.m_ping_awaited = false;
    this->m_mqttClient.m_ping_wait_tick = 0;
    SysLogE("MQTT: keepalive ping not answered. trying reconnect\n");
    this->m_mqttClient.connState = MQTT_HOST_RECONNECT_REQ;
    if (nullptr != this->m_timeoutCb)
    {
      this->m_timeoutCb((uint32_t *)this);
    }
  }

  if (0 != this->m_mqttClient.m_inflight && _tick &&
      ++this->m_mqttClient.m_inflight_tick > MQTT_MS_TO_TICKS(MQTT_READ_TIMEOUT_MS))
  {
    this->m_mqttClient.m_inflight = 0;
    SysLogE("MQTT: a qos 1 publish was not acknowledged. trying reconnect\n");
    this->m_mqttClient.connState = MQTT_HOST_RECONNECT_REQ;
    if (nullptr != this->m_timeoutCb)
    {
      this->m_timeoutCb((uint32_t *)this);
    }
  }

  if (MQTT_DATA == this->m_mqttClient.connState)
  {
    if (_tick)
    {
      this->m_mqttClient.keepAliveTick++;
    }
    uint32_t _keep_alive_ms = (uint32_t)(this->m_mqttClient.mqtt_state.connect_info->keepalive * 0.85) * MILLISECOND_DURATION_1000;
    if (this->m_mqttClient.keepAliveTick > MQTT_MS_TO_TICKS(_keep_alive_ms))
    {
      this->m_mqttClient.connState = MQTT_KEEPALIVE_REQ;
    }
    this->MQTT_Task();
  }
  else if (
      MQTT_CONNECT_SENT == this->m_mqttClient.connState ||
      MQTT_DISCONNECT_SENT == this->m_mqttClient.connState ||
      MQTT_PING_SENT == this->m_mqttClient.connState ||
      MQTT_DATA_SENT == this->m_mqttClient.connState)
  {
    if (_tick)
    {
      this->m_mqttClient.readTimeout++;
    }
    if (this->m_mqttClient.readTimeout > MQTT_MS_TO_TICKS(MQTT_READ_TIMEOUT_MS))
    {
      this->m_mqttClient.readTimeout = 0;
      this->m_mqttClient.connState = MQTT_HOST_RECONNECT_REQ;
      if (nullptr != this->m_timeoutCb)
      {
        this->m_timeoutCb((uint32_t *)this);
      }
    }
    this->MQTT_Task();
  }
  else if (
      MQTT_HOST_RECONNECT_REQ == this->m_mqttClient.connState)
  {
    this->MQTT_Task();
  }
  else if (
      MQTT_HOST_CONNECTING == this->m_mqttClient.connState ||
      MQTT_CONNECT_FAILED == this->m_mqttClient.connState ||
      MQTT_DISCONNECT_FAILED == this->m_mqttClient.connState ||
      MQTT_PING_FAILED == this->m_mqttClient.connState ||
      MQTT_DATA_FAILED == this->m_mqttClient.connState)
  {
    if (_tick)
    {
      this->m_mqttClient.host_connect_tick++;
    }
    if (this->m_mqttClient.host_connect_tick > MQTT_MS_TO_TICKS(MQTT_HOST_CONNECT_TIMEOUT_MS))
    {
      this->m_mqttClient.host_connect_tick = 0;
      SysLogE("MQTT: host connect error. trying reconnect\n");
      this->m_mqttClient.connState = MQTT_HOST_RECONNECT;
      this->MQTT_Task();
    }
    else if( MQTT_HOST_CONNECTING == this->m_mqttClient.connState && isConnected(this->m_client) ){
      this->MQTT_Task();
    }
  }
  else
  {

    // probably this->m_mqttClient.connState == MQTT_DELETING
  }

  if (_tick && this->m_mqttClient.sendTimeout > 0)
  {
    this->m_mqttClient.sendTimeout--;
  }
  __i_dvc_ctrl.yield(); // yield purpose
}

void MQTTClient::Connect()
{
  if (nullptr == this->m_client)
  {
    return;
  }
  this->m_mqttClient.keepAliveTick = 0;
  this->m_mqttClient.host_connect_tick = 0;
  this->m_mqttClient.m_ping_wait_tick = 0;
  this->m_mqttClient.m_ping_awaited = false;
  this->m_mqttClient.m_send_stalled = false;
  this->m_mqttClient.m_inflight = 0;

  this->m_mqttClient.connState = MQTT_HOST_CONNECTING;

  // this->connectServer();
  connectToServer(this->m_client, this->m_host, this->m_port, MQTT_CONNECT_WAIT_MS);
  this->MQTT_Task();
}

void MQTTClient::Disconnect()
{
  this->m_mqttClient.connState = MQTT_DISCONNECT_REQ;
  this->MQTT_Task();
}

void MQTTClient::DeleteClient()
{
  this->m_mqttClient.connState = MQTT_DELETING;
  this->disconnectServer();
  this->mqtt_client_delete();
  // this->MQTT_Task();
}

void MQTTClient::OnConnected(MqttCallback connectedCb)
{
  this->m_connectedCb = connectedCb;
}

void MQTTClient::OnDisconnected(MqttCallback disconnectedCb)
{
  this->m_disconnectedCb = disconnectedCb;
}

void MQTTClient::OnData(MqttDataCallback dataCb)
{
  this->m_dataCb = dataCb;
}

void MQTTClient::OnPublished(MqttCallback publishedCb)
{
  this->m_publishedCb = publishedCb;
}

void MQTTClient::OnSubscribed(MqttCallback subscribedCb)
{
  this->m_subscribedCb = subscribedCb;
}

void MQTTClient::OnUnsubscribed(MqttCallback unsubscribedCb)
{
  this->m_unsubscribedCb = unsubscribedCb;
}

void MQTTClient::OnTimeout(MqttCallback timeoutCb)
{
  this->m_timeoutCb = timeoutCb;
}

/* client interface support functions */

bool MQTTClient::disconnectServer()
{
  if (this->m_disconnectedCb)
  {
    this->m_disconnectedCb((uint32_t *)this);
  }
  return disconnect(this->m_client);
}

/**
 * Reads one packet into buffer; one too big for it but within MQTT_PACKET_MAX is read into a
 * buffer of its exact size handed back in wide, which the caller frees.
 */
uint16_t MQTTClient::readFullPacket(uint8_t *buffer, uint16_t maxsize, uint16_t timeout, uint8_t **wide)
{
  uint8_t *pbuff = buffer;
  uint16_t rlen;

  memset(buffer, 0, maxsize);

  if(this->m_client && this->m_client->available() < 1 ){
    return 0;
  }

  // read the packet type:
  rlen = readPacket(this->m_client, pbuff, 1, timeout);
  if (rlen != 1)
    return 0;

  pbuff++;

  uint32_t value = 0;
  uint32_t multiplier = 1;
  uint8_t encodedByte;

  do
  {
    rlen = readPacket(this->m_client, pbuff, 1, timeout);
    if (rlen != 1)
      return lostPacketStep(buffer, maxsize, 0, 0);
    encodedByte = pbuff[0]; // save the last read val
    pbuff++;                // get ready for reading the next byte
    uint32_t intermediate = encodedByte & 0x7F;
    intermediate *= multiplier;
    value += intermediate;
    multiplier *= 128;
    if (multiplier > (128UL * 128UL * 128UL))
    {
      return lostPacketStep(buffer, maxsize, 0, 0);
    }
  } while (encodedByte & 0x80);

  uint32_t _header = (uint32_t)(pbuff - buffer);
  if (value > (maxsize - _header - 1) && (_header + value) <= MQTT_PACKET_MAX)
  {
    size_t _granted = 0;
    uint8_t *_whole = pdiutil::safe_new_array_upto<uint8_t>(_header + value, _header + value, _granted);
    if (nullptr != _whole)
    {
      memcpy(_whole, buffer, _header);
      rlen = readPacket(this->m_client, _whole + _header, value, timeout);
      if (rlen == value)
      {
        *wide = _whole;
        return (uint16_t)(_header + value);
      }
      pdiutil::safe_delete_array(_whole);
      return lostPacketStep(buffer, maxsize, rlen, value);
    }
  }

  if (value > (maxsize - _header - 1))
  {
    SysLogW("MQTT: a %u byte packet had no room and was dropped\n", (unsigned)(_header + value));
    uint32_t left = value;
    while (left > 0)
    {
      uint16_t room = (uint16_t)(maxsize - (pbuff - buffer) - 1);
      rlen = readPacket(this->m_client, pbuff, left < room ? (uint16_t)left : room, timeout);
      if (0 == rlen)
      {
        return lostPacketStep(buffer, maxsize, value - left, value);
      }
      left -= rlen;
    }
    memset(buffer, 0, maxsize);
    return 0;
  }
  else
  {
    rlen = readPacket(this->m_client, pbuff, value, timeout);
    if (rlen != value)
    {
      return lostPacketStep(buffer, maxsize, rlen, value);
    }
  }
  return ((pbuff - buffer) + rlen);
}

/**
 * Gives up on a packet that stopped arriving part way, which leaves the stream
 * out of step: the connection is dropped so the client connects afresh, and
 * nothing is returned.
 */
uint16_t MQTTClient::lostPacketStep(uint8_t *buffer, uint16_t maxsize, uint32_t got, uint32_t want)
{
  if (0 == want)
  {
    SysLogE("MQTT: a packet length could not be read, reconnecting\n");
  }
  else
  {
    SysLogE("MQTT: a packet stopped at %u of %u bytes, reconnecting\n", (unsigned)got, (unsigned)want);
  }
  memset(buffer, 0, maxsize);
  this->m_mqttClient.connState = MQTT_DATA_FAILED;
  this->m_mqttClient.host_connect_tick = 0;
  this->disconnectServer();
  return 0;
}

#endif
