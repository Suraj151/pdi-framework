/********************************* HTTP helper *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _HTTP_HELPER_H_
#define _HTTP_HELPER_H_

#include <interface/pdi.h>
#include <config/Config.h>
#include <utility/Utility.h>

/* HTTP support functions */

// Get the HTTP status string for a given status code
// Returns a static const char* that should be deleted by the caller to avoid memory leaks
static const char* getHttpStatusString(int code) {
    switch (code) {
        case HTTP_RESP_OK: return ROPTR_TO_CHAR_NEED_DEL("OK");
        case HTTP_RESP_MULTIPLE_CHOICES: return ROPTR_TO_CHAR_NEED_DEL("Multiple Choices");
        case HTTP_RESP_MOVED_PERMANENTLY: return ROPTR_TO_CHAR_NEED_DEL("Moved Permanently");
        case HTTP_RESP_FOUND: return ROPTR_TO_CHAR_NEED_DEL("Found");
        case HTTP_RESP_SEE_OTHER: return ROPTR_TO_CHAR_NEED_DEL("See Other");
        case HTTP_RESP_NOT_MODIFIED: return ROPTR_TO_CHAR_NEED_DEL("Not Modified");
        case HTTP_RESP_USE_PROXY: return ROPTR_TO_CHAR_NEED_DEL("Use Proxy");
        case HTTP_RESP_TEMPORARY_REDIRECT: return ROPTR_TO_CHAR_NEED_DEL("Temporary Redirect");
        case HTTP_RESP_PERMANENT_REDIRECT: return ROPTR_TO_CHAR_NEED_DEL("Permanent Redirect");
        case HTTP_RESP_BAD_REQUEST: return ROPTR_TO_CHAR_NEED_DEL("Bad Request");
        case HTTP_RESP_UNAUTHORIZED: return ROPTR_TO_CHAR_NEED_DEL("Unauthorized");
        case HTTP_RESP_PAYMENT_REQUIRED: return ROPTR_TO_CHAR_NEED_DEL("Payment Required");
        case HTTP_RESP_FORBIDDEN: return ROPTR_TO_CHAR_NEED_DEL("Forbidden");
        case HTTP_RESP_NOT_FOUND: return ROPTR_TO_CHAR_NEED_DEL("Not Found");
        case HTTP_RESP_METHOD_NOT_ALLOWED: return ROPTR_TO_CHAR_NEED_DEL("Method Not Allowed");
        case HTTP_RESP_REQUEST_TIMEOUT: return ROPTR_TO_CHAR_NEED_DEL("Request Timeout");
        case HTTP_RESP_INTERNAL_SERVER_ERROR: return ROPTR_TO_CHAR_NEED_DEL("Internal Server Error");
        case HTTP_RESP_NOT_IMPLEMENTED: return ROPTR_TO_CHAR_NEED_DEL("Not Implemented");
        case HTTP_RESP_BAD_GATEWAY: return ROPTR_TO_CHAR_NEED_DEL("Bad Gateway");
        case HTTP_RESP_SERVICE_UNAVAILABLE: return ROPTR_TO_CHAR_NEED_DEL("Service Unavailable");
        case HTTP_RESP_GATEWAY_TIMEOUT: return ROPTR_TO_CHAR_NEED_DEL("Gateway Timeout");
        case HTTP_RESP_HTTP_VERSION_NOT_SUPPORTED: return ROPTR_TO_CHAR_NEED_DEL("HTTP Version Not Supported");
        case HTTP_RESP_VARIANT_ALSO_NEGOTIATES: return ROPTR_TO_CHAR_NEED_DEL("Variant Also Negotiates");
        default: return ROPTR_TO_CHAR_NEED_DEL("Unknown Status Code");
    }
}

/// URL decode a percent-encoded string
static void urlDecode(pdiutil::string& text){

    pdiutil::string decoded;
    unsigned int len = text.length();
    unsigned int i = 0;
    while (i < len)
    {
        char decodedChar;
        char encodedChar = text[i++];
        if ((encodedChar == '%') && (i + 1 < len))
        {
            // Read two hex digits after '%'
            char hex[3] = { text[i], text[i+1], '\0' };
            decodedChar = static_cast<char>(strtol(hex, nullptr, 16));
            i += 2;
        }
        else if (encodedChar == '+')
        {
            decodedChar = ' ';
        }
        else
        {
            decodedChar = encodedChar;  // normal ascii char
        }
        decoded += decodedChar;
    }
    text = decoded;
}


#endif
