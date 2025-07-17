/********************** HTTP Server Interface Impl ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2025
******************************************************************************/
#include <config/Config.h>

#if defined(ENABLE_HTTP_SERVER)

#include "HttpServerInterfaceImpl.h"
#include <helpers/ClientHelper.h>
#include <helpers/HttpHelper.h>
#include <helpers/StorageHelper.h>

CallBackVoidArgFn HttpServerInterfaceImpl::UriToHandlerMap::notFoundHandler = nullptr;


/**
 * HttpServerInterfaceImpl constructor.
 */
HttpServerInterfaceImpl::HttpServerInterfaceImpl() :
    m_server(nullptr),
    m_client(nullptr)
{
    m_clientRequest.clear();
    m_uriHandlerMap.clear();
    m_responseHeaders.clear();
    m_storagePath = CHARPTR_WRAP("/var/www/static/"); // Default storage path for static files
}

/**
 * HttpServerInterfaceImpl destructor.
 */
HttpServerInterfaceImpl::~HttpServerInterfaceImpl(){
    if( nullptr != m_server ){
        delete m_server;
        m_server = nullptr;
    }
}

/**
 * begin with provided port
 */
void HttpServerInterfaceImpl::begin(uint16_t port){

    if( nullptr == m_server ){
        m_server = __i_instance.getNewTcpServerInstance();
    }

    if( nullptr != m_server ){
        m_server->begin(port);
    }

    #ifdef ENABLE_STORAGE_SERVICE
    __i_instance.getFileSystemInstance().createDirectory(m_storagePath.c_str());
    #endif
}

/**
 * handleClient if any connects
 */
void HttpServerInterfaceImpl::handleClient(){

    if (!m_server) {
        return; // Server not initialized
    }

    // Check if there is a new client connection
    if (!m_client && m_server->hasClient()) {
        m_client = m_server->accept();
    }

    if (m_client && m_client->connected()) {

        if( m_client->available() ){

            // Parse the incoming request
            parseRequest();

            // Handle the request based on the URI
            bool uriFound = false;
            for (uint16_t i = 0; i < m_uriHandlerMap.size(); i++) {

                if (m_clientRequest.uri == m_uriHandlerMap[i].uri) {
                    // Call the registered handler for the URI
                    if (m_uriHandlerMap[i].urihandler) {
                        m_uriHandlerMap[i].urihandler();
                    }
                    uriFound = true;
                    break; // Exit the loop after handling the request
                }
            }

            if (!uriFound) {
                // If no handler was found, call the notFoundHandler
                if (UriToHandlerMap::notFoundHandler) {
                    UriToHandlerMap::notFoundHandler();
                }
            }

            if(!m_clientRequest.isPending){
                m_clientRequest.clear();
            }
            
            // Flush the client buffer
            m_client->flush();
        }
    } else if (m_client && !m_client->connected()) {
        closeClient();
    }
}

/**
 * close
 */
void HttpServerInterfaceImpl::close(){
    closeClient();
    if( nullptr != m_server ){
        m_server->close();
    }
}

/**
 * on uri find call registered handler
 */
void HttpServerInterfaceImpl::on(const pdiutil::string &uri, CallBackVoidArgFn handler){
    m_uriHandlerMap.push_back({uri, handler});
}

/**
 * onNotFound
 * called when handler is not assigned
 */
void HttpServerInterfaceImpl::onNotFound(CallBackVoidArgFn fn){
    UriToHandlerMap::notFoundHandler = fn;
}

/**
 * arg
 * get request argument value by name
 */
pdiutil::string HttpServerInterfaceImpl::arg(const pdiutil::string &name) const {
    // Check if the query/form name exists in the request
    for (uint32_t j = 0; j < m_clientRequest.queries.size(); j++){
        if (m_clientRequest.queries[j].isKeyMatch(name.c_str())) {
            return m_clientRequest.queries[j].value ? m_clientRequest.queries[j].value : "";
        }
    }
    for (uint32_t j = 0; j < m_clientRequest.formdata.size(); j++){
        if (m_clientRequest.formdata[j].isKeyMatch(name.c_str())) {
            return m_clientRequest.formdata[j].value ? m_clientRequest.formdata[j].value : "";
        }
    }
    for (uint32_t j = 0; j < m_clientRequest.files.size(); j++){
        if (m_clientRequest.files[j].isKeyMatch(name.c_str())) {
            return m_clientRequest.files[j].value ? m_clientRequest.files[j].value : "";
        }
    }
    return "";
}

/**
 * hasArg
 * check if argument exists
 */
bool HttpServerInterfaceImpl::hasArg(const pdiutil::string &name) const{
    return !arg(name).empty();
}

/**
 * collectHeaders
 * set the request headers to collect
 */
void HttpServerInterfaceImpl::collectHeaders(const char *headerKeys[], const size_t headerKeysCount){
    m_clientRequest.headers.clear();

    // Add default headers to collect list
    m_clientRequest.headers.push_back({HTTP_HEADER_KEY_HOST, nullptr});
    m_clientRequest.headers.push_back({HTTP_HEADER_KEY_AUTHORIZATION, nullptr});
    m_clientRequest.headers.push_back({HTTP_HEADER_KEY_CONNECTION, nullptr});
    m_clientRequest.headers.push_back({HTTP_HEADER_KEY_CONTENT_TYPE, nullptr});
    m_clientRequest.headers.push_back({HTTP_HEADER_KEY_CONTENT_LENGTH, nullptr});

    // Collect headers from the provided header keys
    for (size_t i = 0; i < headerKeysCount; ++i) {
        
        if (headerKeys[i] != nullptr && strlen(headerKeys[i]) > 0) {
            bool isHeaderCollected = false;

            // Check if the header already exists in the request
            for (size_t j = 0; j < m_clientRequest.headers.size(); j++){
                if (m_clientRequest.headers[j].isKeyMatch(headerKeys[i])) {
                    isHeaderCollected = true;
                    break;
                }
            }

            if (!isHeaderCollected) {
                m_clientRequest.headers.push_back({headerKeys[i], nullptr});
            }
        }
    }
}

/**
 * header
 * get request header value by name
 */
pdiutil::string HttpServerInterfaceImpl::header(const pdiutil::string &name) const {
    // Check if the header name exists in the request
    for (uint32_t j = 0; j < m_clientRequest.headers.size(); j++){
        if (m_clientRequest.headers[j].isKeyMatch(name.c_str())) {
            return m_clientRequest.headers[j].value ? m_clientRequest.headers[j].value : "";
        }
    }
    return "";
}

/**
 * hasHeader
 * check if header exists
 */
bool HttpServerInterfaceImpl::hasHeader(const pdiutil::string &name) const{    
    return !header(name).empty();
}

/**
 * add Header
 */
void HttpServerInterfaceImpl::addHeader(const pdiutil::string &name, const pdiutil::string &value){
  m_responseHeaders += name;
  m_responseHeaders += ": ";
  m_responseHeaders += value;
  m_responseHeaders += "\r\n";
}

/**
 * setStoragePath
 * set storage path for static files
 */
void HttpServerInterfaceImpl::setStoragePath(const pdiutil::string &storagepath) {
    m_storagePath = storagepath;
    if (m_storagePath[m_storagePath.length() - 1] != FILE_SEPARATOR[0]) {
        m_storagePath += FILE_SEPARATOR;
    }
    #ifdef ENABLE_STORAGE_SERVICE
    __i_instance.getFileSystemInstance().createDirectory(m_storagePath.c_str());
    #endif
}

/**
 * send
 */
void HttpServerInterfaceImpl::send(int code, mimetype_t content_type, const char *content){
    if( MIME_TYPE_MAX != content_type ){
        sendResponse(code, content_type, content);
    }else{
        sendResponse(code, MIME_TYPE_TEXT_HTML, "");
    }
}

/**
 * @brief parse incoming HTTP request.
 */
void HttpServerInterfaceImpl::parseRequest(){

    if (!m_server || !m_client) {
        return; // Client/Server not initialized
    }

    int32_t max_timeout = HTTP_CLIENT_MAX_READ_MS;
    uint32_t start = __i_instance.getUtilityInstance().millis_now();
    uint32_t now = start;
    pdiutil::string request_line;
    CallBackVoidArgFn readLineYield = [&]() {
        __i_instance.getUtilityInstance().yield();
    };

    // Read the request line
    m_client->readLine(request_line, readLineYield);

    if (request_line.empty()) {
        return; // No request line received
    }

    // Parse the request line
    // Example: GET /index.html HTTP/1.1
    pdiutil::string method;
    pdiutil::string uri;
    pdiutil::string version;

    // Split the request line into components
    pdiutil::string::size_type pos1 = request_line.find(' ');
    pdiutil::string::size_type pos2 = request_line.find(' ', pos1 + 1);

    if (pos1 != pdiutil::string::npos && pos2 != pdiutil::string::npos) {
        method = request_line.substr(0, pos1);
        uri = request_line.substr(pos1 + 1, pos2 - pos1 - 1);
        version = request_line.substr(pos2 + 1);
    }

    // Set the parsed values to the request structure
    m_clientRequest.uri = uri;
    m_clientRequest.method = method;
    m_clientRequest.version = version;

    // Read headers
    pdiutil::string header_line;
    bool isForm = false; // Indicates form data
    bool isEncoded = false; // Indicates encoded data
    pdiutil::string boundaryStr; // Boundary for multipart/form-data
    uint32_t contentLength = 0; // Content length

    while (1) {

        m_client->readLine(header_line, readLineYield);
        if(header_line.empty()) break; // Exit if no more headers

        // Split header line into key and value
        pdiutil::string::size_type colon_pos = header_line.find(':');
        if (colon_pos != pdiutil::string::npos) {
            pdiutil::string key = header_line.substr(0, colon_pos);
            pdiutil::string value = header_line.substr(colon_pos + 1);
            // Trim leading spaces from value
            value.erase(0, value.find_first_not_of(' '));

            // Collect header if it is in the list of headers to collect
            for (size_t i = 0; i < m_clientRequest.headers.size(); i++){
                m_clientRequest.headers[i].setvalue(key.c_str(), value.c_str());
            }

            if( key == HTTP_HEADER_KEY_CONTENT_TYPE ){

                if( value.find(getMimeTypeString(MIME_TYPE_APPLICATION_X_WWW_FORM_URLENCODED)) != pdiutil::string::npos ){
                    isForm = false;
                    isEncoded = true;
                } else if( value.find(ROPTR_WRAP("multipart/")) != pdiutil::string::npos ){
                    isForm = true;
                    boundaryStr = value.substr(value.find('=') + 1);
                    boundaryStr.replace("\"","");
                }
            }else if( key == HTTP_HEADER_KEY_CONTENT_LENGTH ){
                contentLength = StringToUint32(value.c_str());
            }
        }
    }

    __i_instance.getUtilityInstance().yield(); // Yield to allow other tasks to run

    // read body if content length is specified
    if(!isForm && contentLength > 0) {

        char* body = new char[contentLength + 1];
        if( body ){
            memset(body, 0, contentLength + 1);
            uint16_t readlen = readPacket(  m_client,
                                            (uint8_t *)body,
                                            contentLength,
                                            max_timeout,
                                            0);

            if (readlen > 0) {
                body[readlen] = '\0'; // Null-terminate the body
                m_clientRequest.body = pdiutil::string(body, readlen);
            } else {
                m_clientRequest.body.clear(); // Clear body if read failed
            }
            delete[] body;
        }
    }

    __i_instance.getUtilityInstance().yield(); // Yield to allow other tasks to run

    if(!isForm && m_clientRequest.body.length() < contentLength) {
        return;
    }

    // Parse query parameters from the URI
    pdiutil::string::size_type query_pos = m_clientRequest.uri.find('?');
    if (query_pos != pdiutil::string::npos || (isEncoded && m_clientRequest.body.length() > 0)) {

        pdiutil::string query_string;
        
        if(query_pos != pdiutil::string::npos){
            query_string = m_clientRequest.uri.substr(query_pos + 1);
            m_clientRequest.uri = m_clientRequest.uri.substr(0, query_pos); // Update URI to exclude query string
        }

        // If the request is a form submission, append the body to the query string
        // This is typically used for application/x-www-form-urlencoded data
        if(isEncoded && m_clientRequest.body.length() > 0){
            query_string += '&';
            query_string += m_clientRequest.body;
        }
        
        pdiutil::string::size_type param_start = 0;
        pdiutil::string::size_type param_end = 0;

        // Parse each query parameter
        while (param_start < query_string.length()) {

            param_end = query_string.find('&', param_start);
            if (param_end == pdiutil::string::npos) {
                param_end = query_string.length(); // Last parameter
            }
            pdiutil::string param = query_string.substr(param_start, param_end - param_start);
            pdiutil::string::size_type equal_pos = param.find('=');
            if (equal_pos != pdiutil::string::npos) {
                pdiutil::string key = param.substr(0, equal_pos);
                pdiutil::string value = param.substr(equal_pos + 1);
                urlDecode(key);
                urlDecode(value);
                m_clientRequest.queries.push_back({key.c_str(), value.c_str()});
            }
            param_start = param_end + 1;
        }
    }

    if(isForm){
        // Handle multipart/form-data parsing
        pdiutil::string boundary = "--" + boundaryStr;
        pdiutil::string end_boundary = boundary + "--";
        pdiutil::string part;
        bool found_end = false;

        // Read until the end boundary is found
        while (1) {

            m_client->readLine(part, readLineYield);

            if( part == end_boundary ){
                found_end = true;
            }

            if (found_end) {
                break;
            }            

            if (part.find(boundary) != pdiutil::string::npos) {
                // Start of a new part
            } else {
                // Process Content-Disposition header
                if (part.find(ROPTR_WRAP("Content-Disposition")) != pdiutil::string::npos) {

                    pdiutil::string argname;
                    pdiutil::string argfilename;
                    pdiutil::string argvalue;
                    pdiutil::string argtype = (char*)getMimeTypeString(MIME_TYPE_TEXT_PLAIN);

                    pdiutil::string::size_type nameStart = part.find("=\"");
                    if (nameStart != pdiutil::string::npos) {
                        pdiutil::string::size_type nameEnd = part.find("\"", nameStart + 2);
                        argname = part.substr(nameStart + 2, nameEnd - nameStart - 3);

                        pdiutil::string::size_type filenameStart = part.find(ROPTR_WRAP("filename=\""), nameEnd + 1);
                        if (filenameStart != pdiutil::string::npos) {
                            pdiutil::string::size_type filenameEnd = part.find("\"", filenameStart + 10);
                            argfilename = part.substr(filenameStart + 10, filenameEnd - filenameStart - 11);
                        }
                    }

                    // Continue reading lines until we find enpty line after Content-Disposition
                    while (true){
                        m_client->readLine(part, readLineYield); // Read the next line after Content-Disposition
                        if (!part.empty()) {
                            pdiutil::string::size_type argtypeStart = part.find(ROPTR_WRAP("Content-Type: "));
                            if (argtypeStart != pdiutil::string::npos) {
                                argtype = part.substr(argtypeStart + 14);
                            }
                        }else{
                            // skip empty line after Content-Disposition
                            break;
                        }
                    }

                    if( argfilename.empty() ){
                        // Read the argument value
                        m_client->readLine(argvalue, readLineYield, 128);

                        // Store the argument in the request
                        m_clientRequest.formdata.push_back({argname.c_str(), argvalue.c_str()});
                    }else{

                        // Read the file content
                        #ifdef ENABLE_STORAGE_SERVICE
                        const char *tempdir = __i_instance.getFileSystemInstance().getTempDirectory();
                        pdiutil::string tempFilePath = pdiutil::string(tempdir) + argfilename;
                        if(__i_instance.getFileSystemInstance().isFileExist(tempFilePath.c_str())){
                            // If the file already exists, delete it
                            __i_instance.getFileSystemInstance().deleteFile(tempFilePath.c_str());
                        }
                        uint8_t filewritecounter = 0;
                        #else
                        pdiutil::string tempFilePath = argfilename;
                        #endif
                        
                        bool found_boundary = false;
                        part.clear();
                        argvalue.clear();
                        pdiutil::string lastread;
                        while (1) {

                            m_client->readStringUntil(part, 0, readLineYield, 64);

                            lastread += part;

                            if(lastread.find(end_boundary) != pdiutil::string::npos){
                                lastread = lastread.substr(0, lastread.find(end_boundary));
                                found_end = true;
                            }
                                
                            if(lastread.find(boundary) != pdiutil::string::npos){
                                lastread = lastread.substr(0, lastread.find(boundary));
                                found_boundary = true;
                            }

                            #ifdef ENABLE_STORAGE_SERVICE
                            // Write the file content to a temporary file
                            if(++filewritecounter == 2){
                                // If we have read two parts, write the last part to the file
                                __i_instance.getFileSystemInstance().writeFile(tempFilePath.c_str(), lastread.c_str(), lastread.length(), true);
                                filewritecounter = 0;
                            }
                            #else
                            // without file system support
                            if(argvalue.length() < 500){
                                argvalue += part;
                            }
                            #endif

                            lastread = part; // Store the last read part
                            part.clear();

                            // If we found a boundary, we can stop reading further
                            if (found_boundary || found_end) {
                                break;
                            }
                        }

                        // Store the file data in the request
                        #ifdef ENABLE_STORAGE_SERVICE
                        m_clientRequest.files.push_back({argname.c_str(), tempFilePath.c_str()});
                        #else
                        argvalue = argfilename + ':' + argvalue;
                        m_clientRequest.files.push_back({argname.c_str(), argvalue.c_str()});
                        #endif

                    }
                }
            }
            __i_instance.getUtilityInstance().wait(1);
        }

        __i_instance.getUtilityInstance().yield(); // Yield to allow other tasks to run
    }
}

/**
 * @brief Prepare the header for response
 */
void HttpServerInterfaceImpl::prepareResponseHeader(pdiutil::string& _header, int code, const char *content_type, uint32_t content_length){

    _header.clear();

    _header += m_clientRequest.version;
    _header += ' ';
    _header += pdiutil::to_string(code);
    _header += ' ';
    _header += getHttpStatusString(code);
    _header += "\r\n";

    addHeader(HTTP_HEADER_KEY_CONTENT_TYPE, content_type);
    addHeader(HTTP_HEADER_KEY_CONTENT_LENGTH, pdiutil::to_string(content_length));

    pdiutil::string keepalive = header(HTTP_HEADER_KEY_CONNECTION);
    if (keepalive.empty() || keepalive == "close") {
        addHeader(HTTP_HEADER_KEY_CONNECTION, "close");
    } else {
        addHeader(HTTP_HEADER_KEY_CONNECTION, CHARPTR_WRAP("keep-alive"));
        addHeader(HTTP_HEADER_KEY_KEEP_ALIVE, CHARPTR_WRAP("timeout=30 max=100"));
    }

    addHeader(HTTP_HEADER_KEY_ACCESS_CONTROL_ALLOW_ORIGIN, "*"); // Allow CORS

    _header += m_responseHeaders;
    _header += "\r\n";
    m_responseHeaders = "";
}

/**
 * @brief Send response to client
 */
void HttpServerInterfaceImpl::sendResponse(int code, mimetype_t content_type, const char *content){

    if (!m_client || !content) {
        return; // Client not initialized
    }

    pdiutil::string response;
    prepareResponseHeader(response, code, getMimeTypeString(content_type), strlen(content));

    // Send the response headers
    sendPacket(m_client, (uint8_t *)response.c_str(), response.length());

    // Send the response body
    sendPacket(m_client, (uint8_t *)content, strlen(content), 400);

    // Make sure data has been sent
    __task_scheduler.setTimeout( [&]() {
        m_client->write((const uint8_t*)"", 0);
    }, 1, __i_instance.getUtilityInstance().millis_now());
}

/**
 * @brief handle file request.
 */
bool HttpServerInterfaceImpl::handleStaticFileRequest(){

    bool bStatus = false;

    #ifdef ENABLE_STORAGE_SERVICE

    if (m_clientRequest.method == "GET" || m_clientRequest.method == "HEAD") {

        pdiutil::string filePath = m_storagePath + (m_clientRequest.uri[0] == '/' ? m_clientRequest.uri.substr(1) : m_clientRequest.uri);
        mimetype_t filetype = __i_instance.getFileSystemInstance().getFileMimeType(filePath.c_str());

        // Check if the request URI is a static file
        if (__i_instance.getFileSystemInstance().isFileExist(filePath.c_str())) {

            if (filetype == MIME_TYPE_MAX) {
                filetype = MIME_TYPE_TEXT_PLAIN;
            }

            // Set the content type based on the file type
            // Add Content-Disposition header to force download
            m_responseHeaders.clear();
            pdiutil::string content_disposition_value = CHARPTR_WRAP("attachment; filename=\"") + __i_instance.getFileSystemInstance().basename(filePath.c_str()) + "\"";
            addHeader(HTTP_HEADER_KEY_CONTENT_DISPOSITION, content_disposition_value);

            pdiutil::string response;
            prepareResponseHeader(response, HTTP_RESP_OK, getMimeTypeString(filetype), __i_instance.getFileSystemInstance().getFileSize(filePath.c_str()));

            // Send the response headers
            sendPacket(m_client, (uint8_t *)response.c_str(), response.length());

            // Send the response body
            int iStatus = __i_instance.getFileSystemInstance().readFile(filePath.c_str(), 400, [&](char* data, uint32_t size)->bool{
                sendPacket(m_client, (uint8_t *)data, size, 400);
                // return true to continue reading
                return true;
            });

            // Make sure data has been sent
            __task_scheduler.setTimeout( [&]() {
                m_client->write((const uint8_t*)"", 0);
            }, 1, __i_instance.getUtilityInstance().millis_now());

            bStatus = true;
        }
    }

    #endif

    return bStatus;
}

/**
 * @brief close current client.
 */
void HttpServerInterfaceImpl::closeClient() {
    if (m_client) {
        m_client->close();
        delete m_client;
        m_client = nullptr;
    }
}

#endif