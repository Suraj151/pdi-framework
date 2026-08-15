/************** Over The Air firmware update service **************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _HTTP_OTA_SERVICE_PROVIDER_H_
#define _HTTP_OTA_SERVICE_PROVIDER_H_

#include <service_provider/ServiceProvider.h>
#include <service_provider/database/DatabaseServiceProvider.h>
#include <transports/http/HTTPClient.h>

/**
 * ota status enum
 */
enum http_ota_status{
  GET_VERSION_FAILED,
  VERSION_NOT_FOUND,
  UPDATE_FAILD,
  NO_UPDATES,
  UPDATE_OK,
  UNKNOWN
};

/**
 * OtaServiceProvider class
 */
class OtaServiceProvider : public ServiceProvider{

  public:

    /**
     * OtaServiceProvider constructor.
     */
    OtaServiceProvider();
    /**
     * OtaServiceProvider destructor.
     */
    ~OtaServiceProvider();

    bool initService(void *arg = nullptr) override;
    void handleOta();
    void handleOtaVersionRequest();
    void handleOtaVersionResponse( Http_Client *client );
    void setHttpHost(const char* _host);
    void printConfigToTerminal(iTerminalInterface *terminal) override;

#ifdef ENABLE_STORAGE_SERVICE
    /**
     * @brief Flashes a firmware image already present on the filesystem.
     *
     * The caller is expected to have answered the client before restarting,
     * so the restart is left to the caller rather than done here.
     *
     * @param _path Absolute path of the image to flash.
     * @return upgrade_status_t result of the attempt.
     */
    upgrade_status_t flashFromFile(const char *_path);

    /**
     * @brief Collects the firmware images available on the filesystem.
     *
     * @param _dir Directory to scan.
     * @param _images Receives the name of every matching image.
     */
    void collectLocalImages(const char *_dir, pdiutil::vector<pdiutil::string> &_images);
#endif

    /**
     * @var	Http_Client*|nullptr	m_http_client
     */
    Http_Client  *m_http_client;
};

extern OtaServiceProvider __ota_service;

#endif
