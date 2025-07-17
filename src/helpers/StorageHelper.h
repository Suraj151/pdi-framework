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
static rofn::ROPTR getMimeTypeString(mimetype_t type) {
    switch (type) {
        case MIME_TYPE_TEXT_PLAIN: return ROPTR_WRAP("text/plain");
        case MIME_TYPE_TEXT_HTML: return ROPTR_WRAP("text/html");
        case MIME_TYPE_TEXT_CSS: return ROPTR_WRAP("text/css");
        case MIME_TYPE_TEXT_JAVASCRIPT: return ROPTR_WRAP("text/javascript");
        case MIME_TYPE_TEXT_CSV: return ROPTR_WRAP("text/csv");
        case MIME_TYPE_APPLICATION_JSON: return ROPTR_WRAP("application/json");
        case MIME_TYPE_APPLICATION_XML: return ROPTR_WRAP("application/xml");
        case MIME_TYPE_APPLICATION_OCTET_STREAM: return ROPTR_WRAP("application/octet-stream");
        case MIME_TYPE_APPLICATION_PDF: return ROPTR_WRAP("application/pdf");
        case MIME_TYPE_APPLICATION_ZIP: return ROPTR_WRAP("application/zip");
        case MIME_TYPE_APPLICATION_GZIP: return ROPTR_WRAP("application/gzip");
        case MIME_TYPE_APPLICATION_X_WWW_FORM_URLENCODED: return ROPTR_WRAP("application/x-www-form-urlencoded");
        case MIME_TYPE_IMAGE_GIF: return ROPTR_WRAP("image/gif");
        case MIME_TYPE_IMAGE_JPEG: return ROPTR_WRAP("image/jpeg");
        case MIME_TYPE_IMAGE_PNG: return ROPTR_WRAP("image/png");
        default: return ROPTR_WRAP("text/plain");
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
