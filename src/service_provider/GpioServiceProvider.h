/****************************** Gpio service **********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _GPIO_SERVICE_PROVIDER_H_
#define _GPIO_SERVICE_PROVIDER_H_

#include <service_provider/ServiceProvider.h>
#include <service_provider/HttpServiceProvider.h>
#include <service_provider/EmailServiceProvider.h>
#include <service_provider/DatabaseServiceProvider.h>

#include <Ticker.h>

#define GPIO_PAYLOAD_DATA_KEY "data"
#define GPIO_PAYLOAD_MODE_KEY "mode"
#define GPIO_PAYLOAD_VALUE_KEY "val"
#define GPIO_PAYLOAD_MAC_KEY "mac_id"
#define GPIO_ALERT_PIN_KEY "alrtkey"

// currently 100 ms is kept as lowest blink time
#define GPIO_DIGITAL_BLINK_MIN_DURATION_MS 100

class DigitalBlinker
{

private:
  uint8_t m_pin;
  uint32_t m_duration;
  Ticker m_ticker;

public:
  /**
   * DigitalBlinker constructor.
   */
  DigitalBlinker(uint8_t pin, uint32_t duration) : m_pin(pin),
                                                   m_duration(duration)
  {
    pinMode(this->m_pin, OUTPUT);
    this->start();
  }

  /**
   * DigitalBlinker destructor.
   */
  ~DigitalBlinker()
  {
    this->stop();
  };

  /**
   * blink callback function
   */
  void blink()
  {
    __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, this->m_pin, !__i_dvc_ctrl.gpioRead(DIGITAL_READ, this->m_pin));
  }

  void setConfig(uint8_t pin, uint32_t duration)
  {
    this->m_pin = pin;
    this->m_duration = duration;
  }

  void updateConfig(uint8_t pin, uint32_t duration)
  {
    if (this->m_pin != pin || this->m_duration != duration)
    {
      this->setConfig(pin, duration);

      if (this->isRunning())
      {
        this->stop();
        this->start();
      }
    }
  }

  void start()
  {
    if (!this->isRunning() && GPIO_DIGITAL_BLINK_MIN_DURATION_MS <= this->m_duration)
    {
      this->m_ticker.attach_ms(this->m_duration, std::bind(&DigitalBlinker::blink, this));
    }
  }

  void stop()
  {
    this->m_ticker.detach();
  }

  bool isRunning()
  {
    return this->m_ticker.active();
  }
};

/**
 * GpioServiceProvider class
 */
class GpioServiceProvider : public ServiceProvider
{

public:
  /**
   * GpioServiceProvider constructor.
   */
  GpioServiceProvider();
  /**
   * GpioServiceProvider destructor
   */
  ~GpioServiceProvider();

  void begin(iWiFiInterface *_wifi, iWiFiClientInterface *_wifi_client);
  void enable_update_gpio_table_from_copy(void);
  void appendGpioJsonPayload(String &_payload, bool isAlertPost = false);
  void applyGpioJsonPayload(char *_payload, uint16_t _payload_length);
#ifdef ENABLE_EMAIL_SERVICE
  bool handleGpioEmailAlert(void);
#endif
  void handleGpioOperations(void);
  void handleGpioModes(int _gpio_config_type = GPIO_MODE_CONFIG);
  uint8_t getGpioFromPinMap(uint8_t _pin);
  bool handleGpioHttpRequest(bool isAlertPost = false);
  void printGpioConfigLogs(void);
  bool is_exceptional_gpio_pin(uint8_t _pin);

  /**
   * @var gpio_config_table m_gpio_config_copy
   */
  gpio_config_table m_gpio_config_copy;
  /**
   * @var	int|0 m_gpio_http_request_cb_id
   */
  int m_gpio_http_request_cb_id;
  /**
   * @var	bool|true m_update_gpio_table_from_copy
   */
  bool m_update_gpio_table_from_copy;

protected:
  /**
   * @var	iWiFiClientInterface  *m_wifi_client
   */
  iWiFiClientInterface *m_wifi_client;
  iWiFiInterface *m_wifi;
  DigitalBlinker *m_digital_blinker[MAX_DIGITAL_GPIO_PINS];
};

extern GpioServiceProvider __gpio_service;

#endif
