/***************************** HTTP Config page *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#ifndef _HTTP_CONFIG_H_
#define _HTTP_CONFIG_H_

#include "Common.h"

/* Http defs */
typedef enum {
    HTTP_RESP_OK = 200,
    HTTP_RESP_MULTIPLE_CHOICES = 300,
    HTTP_RESP_MOVED_PERMANENTLY = 301,
    HTTP_RESP_FOUND = 302,
    HTTP_RESP_SEE_OTHER = 303,
    HTTP_RESP_NOT_MODIFIED = 304,
    HTTP_RESP_USE_PROXY = 305,
    HTTP_RESP_TEMPORARY_REDIRECT = 307,
    HTTP_RESP_PERMANENT_REDIRECT = 308,
    HTTP_RESP_BAD_REQUEST = 400,
    HTTP_RESP_UNAUTHORIZED = 401,
    HTTP_RESP_PAYMENT_REQUIRED = 402,
    HTTP_RESP_FORBIDDEN = 403,
    HTTP_RESP_NOT_FOUND = 404,
    HTTP_RESP_METHOD_NOT_ALLOWED = 405,
    HTTP_RESP_REQUEST_TIMEOUT = 408,
    HTTP_RESP_INTERNAL_SERVER_ERROR = 500,
    HTTP_RESP_NOT_IMPLEMENTED = 501,
    HTTP_RESP_BAD_GATEWAY = 502,
    HTTP_RESP_SERVICE_UNAVAILABLE = 503,
    HTTP_RESP_GATEWAY_TIMEOUT = 504,
    HTTP_RESP_HTTP_VERSION_NOT_SUPPORTED = 505,
    HTTP_RESP_VARIANT_ALSO_NEGOTIATES = 506,
    HTTP_RESP_MAX = 999
} http_resp_code_t;

typedef enum {
    HTTP_ERROR_CONNECTION_FAILED = -1,
    HTTP_ERROR_MAX = -999
} http_err_code_t;

typedef enum {
    HTTP_VERSION_1_0,
    HTTP_VERSION_1_1,
    HTTP_VERSION_2,
    HTTP_VERSION_3,
    HTTP_VERSION_MAX
} http_version_t;

typedef enum {
    HTTP_METHOD_GET, 
    HTTP_METHOD_POST, 
    HTTP_METHOD_HEAD, 
    HTTP_METHOD_PUT, 
    HTTP_METHOD_PATCH, 
    HTTP_METHOD_DELETE, 
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_MAX, 
} http_method_t;


#ifndef HTTP_HOST_CONNECT_TIMEOUT
#define HTTP_HOST_CONNECT_TIMEOUT 2500
#endif

#ifndef HTTP_DEFAULT_PORT
#define HTTP_DEFAULT_PORT 80
#endif

#ifndef HTTP_DEFAULT_VERSION
#define HTTP_DEFAULT_VERSION HTTP_VERSION_1_1
#endif

#ifndef HTTP_DEFAULT_KEEP_ALIVE_MS
#define HTTP_DEFAULT_KEEP_ALIVE_MS 30000
#endif

// Client specific defines
#define HTTP_CLIENT_BUF_SIZE 640
#define HTTP_CLIENT_READINTERVAL_MS 10
#define HTTP_CLIENT_MAX_READ_MS 1500

#define HTTP_HEADER_KEY_HOST            "Host"
#define HTTP_HEADER_KEY_USER_AGENT      "User-Agent"
#define HTTP_HEADER_KEY_ACCEPT_ENCODING "Accept-Encoding"
#define HTTP_HEADER_KEY_CONNECTION      "Connection"
#define HTTP_HEADER_KEY_LOCATION        "Location"
#define HTTP_HEADER_KEY_AUTHORIZATION   "Authorization"
#define HTTP_HEADER_KEY_CONTENT_TYPE    "Content-Type"
#define HTTP_HEADER_KEY_CONTENT_LENGTH  "Content-Length"
#define HTTP_HEADER_KEY_KEEP_ALIVE      "Keep-Alive"
#define HTTP_HEADER_KEY_ACCESS_CONTROL_ALLOW_ORIGIN "Access-Control-Allow-Origin"
#define HTTP_HEADER_KEY_CONTENT_DISPOSITION "Content-Disposition"

// #define HTTP_VERSION_1_0_STR            "HTTP/1.0"
// #define HTTP_VERSION_1_1_STR            "HTTP/1.1"
// #define HTTP_VERSION_2_STR              "HTTP/2.0"
// #define HTTP_VERSION_3_STR              "HTTP/3.0"

// #define HTTP_METHOD_GET_STR             "GET"
// #define HTTP_METHOD_POST_STR            "POST"
// #define HTTP_METHOD_HEAD_STR            "HEAD"
// #define HTTP_METHOD_PUT_STR             "PUT"
// #define HTTP_METHOD_PATCH_STR           "PATCH"
// #define HTTP_METHOD_DELETE_STR          "DELETE"
// #define HTTP_METHOD_OPTIONS_STR         "OPTIONS"


/**
 * http parameter structure
 */
struct http_param_t{

    char *key = nullptr;
    char *value = nullptr;

    // Constructor
    http_param_t() : 
        key(nullptr),
        value(nullptr)
    {
    }

    // Parameterised Constructor
    http_param_t(const char *_key, const char *_value) : 
        key(nullptr),
        value(nullptr)
    {
        fill(_key, _value);
    }

    // Copy Constructor
    http_param_t(const http_param_t &obj):
        key(nullptr),
        value(nullptr)
    {
        fill(obj.key, obj.value);
    }

    // Assignment Operator
    http_param_t& operator=(const http_param_t &obj){

        if (this != &obj) {
            fill(obj.key, obj.value);
        }
        return *this;
    }

    // Destructor
    ~http_param_t(){
        clear();
    }

    // fill param bag
    void fill(const char *_key, const char *_value){

        clear(); // clear existing resources

        if (nullptr != _key){

            uint16_t _len = strlen(_key);
            key = new char[_len + 1];
            if (nullptr != key){
                memset(key, 0, _len + 1);
                strcpy(key, _key);
            }
        }

        if (nullptr != _value){

            uint16_t _len = strlen(_value);
            value = new char[_len + 1];
            if (nullptr != value){
                memset(value, 0, _len + 1);
                strcpy(value, _value);
            }
        }
    }

    // check if key matches with provided key 
    bool isKeyMatch(const char *_key) const{

        if (
            nullptr == _key || 
            nullptr == key || 
            strcmp(key, _key) != 0
        )
            return false;
        
        return true;
    }

    // set value if key is matching 
    bool setvalue(const char *_key, const char *_value){

        if (
            nullptr == _key || 
            nullptr == key || 
            strcmp(key, _key) != 0
        )
            return false;
        
        if (nullptr != value)
        {
            delete[] value;
            value = nullptr;
        }

        if (nullptr != _value){

            uint16_t _len = strlen(_value);
            value = new char[_len + 1];
            if (nullptr != value){
                memset(value, 0, _len + 1);
                strcpy(value, _value);
            }
        }

        return true;
    }

    // clear request resources and set to defaults
    void clear(){

        if (nullptr != key)
        {
            delete[] key;
            key = nullptr;
        }
        if (nullptr != value)
        {
            delete[] value;
            value = nullptr;
        }
    }
};

using http_header_t = http_param_t;
using http_query_t = http_param_t;
using http_file_t = http_param_t;

#endif
