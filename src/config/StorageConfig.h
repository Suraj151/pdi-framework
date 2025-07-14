/***************************** Storage Config page ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2025
******************************************************************************/
#ifndef _STORAGE_CONFIG_H_
#define _STORAGE_CONFIG_H_

#include "Common.h"

/**
 * Storage configurations
 */

// enum StorageType {
//     STORAGE_TYPE_FLASH,
//     STORAGE_TYPE_SPIFFS,
//     STORAGE_TYPE_LITTLEFS,
//     STORAGE_TYPE_MAX
// };

// enum StorageAccessMode {
//     STORAGE_ACCESS_MODE_R,
//     STORAGE_ACCESS_MODE_W,
//     STORAGE_ACCESS_MODE_X,
//     STORAGE_ACCESS_MODE_MAX
// };

typedef enum {
    MIME_TYPE_TEXT_PLAIN,
    MIME_TYPE_TEXT_HTML,
    MIME_TYPE_TEXT_CSS,
    MIME_TYPE_TEXT_JAVASCRIPT,
    MIME_TYPE_TEXT_CSV,
    MIME_TYPE_APPLICATION_JSON,
    MIME_TYPE_APPLICATION_XML,
    MIME_TYPE_APPLICATION_OCTET_STREAM,
    MIME_TYPE_APPLICATION_PDF,
    MIME_TYPE_APPLICATION_ZIP,
    MIME_TYPE_APPLICATION_GZIP,
    MIME_TYPE_APPLICATION_X_WWW_FORM_URLENCODED,
    MIME_TYPE_IMAGE_GIF,
    MIME_TYPE_IMAGE_JPEG,
    MIME_TYPE_IMAGE_PNG,
    MIME_TYPE_MAX
} mimetype_t;


#endif
