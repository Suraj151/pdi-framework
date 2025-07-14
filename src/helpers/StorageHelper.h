/******************************* Storage helper *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _STORAGE_HELPER_H_
#define _STORAGE_HELPER_H_

#include <interface/pdi.h>
#include <config/Config.h>
#include <utility/Utility.h>

/* STORAGE support functions */

// Returns the MIME type string associated with a MIME type
// Returns nullptr if the MIME type is not recognized
static const char* getMimeTypeString(mimetype_t type) {
    switch (type) {
        case MIME_TYPE_TEXT_PLAIN: return ROPTR_TO_CHAR_NEED_DEL("text/plain");
        case MIME_TYPE_TEXT_HTML: return ROPTR_TO_CHAR_NEED_DEL("text/html");
        case MIME_TYPE_TEXT_CSS: return ROPTR_TO_CHAR_NEED_DEL("text/css");
        case MIME_TYPE_TEXT_JAVASCRIPT: return ROPTR_TO_CHAR_NEED_DEL("text/javascript");
        case MIME_TYPE_TEXT_CSV: return ROPTR_TO_CHAR_NEED_DEL("text/csv");
        case MIME_TYPE_APPLICATION_JSON: return ROPTR_TO_CHAR_NEED_DEL("application/json");
        case MIME_TYPE_APPLICATION_XML: return ROPTR_TO_CHAR_NEED_DEL("application/xml");
        case MIME_TYPE_APPLICATION_OCTET_STREAM: return ROPTR_TO_CHAR_NEED_DEL("application/octet-stream");
        case MIME_TYPE_APPLICATION_PDF: return ROPTR_TO_CHAR_NEED_DEL("application/pdf");
        case MIME_TYPE_APPLICATION_ZIP: return ROPTR_TO_CHAR_NEED_DEL("application/zip");
        case MIME_TYPE_APPLICATION_GZIP: return ROPTR_TO_CHAR_NEED_DEL("application/gzip");
        case MIME_TYPE_APPLICATION_X_WWW_FORM_URLENCODED: return ROPTR_TO_CHAR_NEED_DEL("application/x-www-form-urlencoded");
        case MIME_TYPE_IMAGE_GIF: return ROPTR_TO_CHAR_NEED_DEL("image/gif");
        case MIME_TYPE_IMAGE_JPEG: return ROPTR_TO_CHAR_NEED_DEL("image/jpeg");
        case MIME_TYPE_IMAGE_PNG: return ROPTR_TO_CHAR_NEED_DEL("image/png");
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
