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

// Returns the MIME type string associated with a MIME type
// Returns nullptr if the MIME type is not recognized
static const char* getMimeTypeString(mimetype_t type) {
    switch (type) {
        case MIME_TYPE_TEXT_PLAIN: return "text/plain";
        case MIME_TYPE_TEXT_HTML: return "text/html";
        case MIME_TYPE_TEXT_CSS: return "text/css";
        case MIME_TYPE_TEXT_JAVASCRIPT: return "text/javascript";
        case MIME_TYPE_TEXT_CSV: return "text/csv";
        case MIME_TYPE_APPLICATION_JSON: return "application/json";
        case MIME_TYPE_APPLICATION_XML: return "application/xml";
        case MIME_TYPE_APPLICATION_OCTET_STREAM: return "application/octet-stream";
        case MIME_TYPE_APPLICATION_PDF: return "application/pdf";
        case MIME_TYPE_APPLICATION_ZIP: return "application/zip";
        case MIME_TYPE_APPLICATION_GZIP: return "application/gzip";
        case MIME_TYPE_APPLICATION_X_WWW_FORM_URLENCODED: return "application/x-www-form-urlencoded";
        case MIME_TYPE_IMAGE_GIF: return "image/gif";
        case MIME_TYPE_IMAGE_JPEG: return "image/jpeg";
        case MIME_TYPE_IMAGE_PNG: return "image/png";
        default: return nullptr;
    }
}

// Returns the file extension associated with a MIME type
// Returns nullptr if the MIME type does not have a standard file extension
static const char* getMimeTypeExtension(mimetype_t type) {
    switch (type) {
        case MIME_TYPE_TEXT_PLAIN: return ".txt";
        case MIME_TYPE_TEXT_HTML: return ".html";
        case MIME_TYPE_TEXT_CSS: return ".css";
        case MIME_TYPE_TEXT_JAVASCRIPT: return ".js";
        case MIME_TYPE_TEXT_CSV: return ".csv";
        case MIME_TYPE_APPLICATION_JSON: return ".json";
        case MIME_TYPE_APPLICATION_XML: return ".xml";
        case MIME_TYPE_APPLICATION_OCTET_STREAM: return ".bin";
        case MIME_TYPE_APPLICATION_PDF: return ".pdf";
        case MIME_TYPE_APPLICATION_ZIP: return ".zip";
        case MIME_TYPE_APPLICATION_GZIP: return ".gz";
        case MIME_TYPE_APPLICATION_X_WWW_FORM_URLENCODED: return ".urlencoded";
        case MIME_TYPE_IMAGE_GIF: return ".gif";
        case MIME_TYPE_IMAGE_JPEG: return ".jpeg";
        case MIME_TYPE_IMAGE_PNG: return ".png";
        default: return nullptr;
    }
}

#endif
