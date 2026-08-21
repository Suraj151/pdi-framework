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
    m_client(nullptr),
    m_currentclient_lastactivity_timestamp(0),
    m_handlingclientfromcb(false)
#ifdef ENABLE_TLS_SERVICE
    , m_secure(false)
#endif
#if defined(ENABLE_HTTPS_SERVER) && defined(ENABLE_TLS_SERVICE)
    , m_insecure_server(nullptr)
#endif
{
    m_clientRequest.clear();
    m_uriHandlerMap.clear();
    m_responseHeaders.clear();
    m_storagePath = CHARPTR_WRAP_RO(HTTP_SERVER_DEFAULT_STATIC_PATH); // Default storage path for static files
}

/**
 * HttpServerInterfaceImpl destructor.
 */
HttpServerInterfaceImpl::~HttpServerInterfaceImpl(){
    if( nullptr != m_server ){
        pdiutil::safe_delete(m_server);
        m_server = nullptr;
    }
    #if defined(ENABLE_HTTPS_SERVER) && defined(ENABLE_TLS_SERVICE)
    pdiutil::safe_delete(m_insecure_server);
    #endif
}

/**
 * begin with provided port
 */
void HttpServerInterfaceImpl::begin(uint16_t port, bool secure){

    if( nullptr == m_server ){
        #ifdef ENABLE_TLS_SERVICE
        if(secure){
            iTlsServerInterface* tls = __i_instance.getNewTlsServerInstance();
            if(tls){
                tls->setServerCertificatePath(m_serverCertPath.c_str());
                tls->setServerPrivateKeyPath(m_serverKeyPath.c_str());
                if(!m_clientCaPath.empty()){
                    tls->setClientCertificateAuthorityPath(m_clientCaPath.c_str());
                }
            }
            m_server = tls;
            m_secure = true;
        } else {
            m_server = __i_instance.getNewTcpServerInstance();
            m_secure = false;
        }
        #else
        (void)secure;
        m_server = __i_instance.getNewTcpServerInstance();
        #endif
    }

    if( nullptr != m_server ){
        if (m_server->begin(port) == 0) {

            m_server->setOnAcceptClientEventCallback([](void* arg){
                HttpServerInterfaceImpl *ihttpserver = reinterpret_cast<HttpServerInterfaceImpl*>(arg);

                if(ihttpserver && !ihttpserver->m_handlingclientfromcb && !ihttpserver->m_client){

                    ihttpserver->m_handlingclientfromcb = true;

                    ihttpserver->m_client = ihttpserver->m_server->accept();
                    ihttpserver->m_currentclient_lastactivity_timestamp = __i_instance.getUtilityInstance().millis_now();

                    ihttpserver->m_handlingclientfromcb = false;
                }
            }, this);
        }
    }

    #if defined(ENABLE_HTTPS_SERVER) && defined(ENABLE_TLS_SERVICE)
    if( secure && nullptr == m_insecure_server ){
        m_insecure_server = __i_instance.getNewTcpServerInstance();
        if( nullptr != m_insecure_server ){
            m_insecure_server->begin(HTTP_DEFAULT_PORT);
        }
    }
    #endif

    #ifdef ENABLE_STORAGE_SERVICE
    __i_instance.getFileSystemInstance().createDirectory(m_storagePath.c_str());
    #endif
}

#ifdef ENABLE_TLS_SERVICE
void HttpServerInterfaceImpl::setServerCertificatePath(const char* path){
    m_serverCertPath = path ? path : "";
}

void HttpServerInterfaceImpl::setServerPrivateKeyPath(const char* path){
    m_serverKeyPath = path ? path : "";
}

void HttpServerInterfaceImpl::setClientCertificateAuthorityPath(const char* path){
    m_clientCaPath = path ? path : "";
}
#endif

/**
 * handleClient if any connects
 */
void HttpServerInterfaceImpl::handleClient(){

    if (!m_server) {
        return; // Server not initialized
    }

    // Skip if the lwIP callback is currently mid-accept; avoids racing on m_client.
    if (m_handlingclientfromcb) {
        return;
    }

    // A connection that arrived while the previous client was still being served
    // stays waiting in the server, the accept callback only runs for a new one.
    // Picking it up here is what keeps it from being reset by the next accept.
    if (!m_client && m_server->hasClient()) {

        m_handlingclientfromcb = true;
        m_client = m_server->accept();
        m_currentclient_lastactivity_timestamp = __i_instance.getUtilityInstance().millis_now();
        m_handlingclientfromcb = false;
    }

    #if defined(ENABLE_HTTPS_SERVER) && defined(ENABLE_TLS_SERVICE)
    if (!m_client && nullptr != m_insecure_server && m_insecure_server->hasClient()) {

        m_handlingclientfromcb = true;
        m_client = m_insecure_server->accept();
        m_currentclient_lastactivity_timestamp = __i_instance.getUtilityInstance().millis_now();
        m_handlingclientfromcb = false;
    }
    #endif

    if (m_client && m_client->connected()) {

        if( m_client->available() ){

            // Parse the incoming request
            parseRequest();

            #if defined(ENABLE_HTTPS_SERVER) && defined(ENABLE_TLS_SERVICE)
            if( m_client && !m_client->isSecure() ){
                sendHttpsRedirect();
                if( !m_clientRequest.isPending ){
                    m_clientRequest.clear();
                }
                m_client->flush(FLUSH_ALL);
                // closeClient();
                // m_currentclient_lastactivity_timestamp = 0;
                return;
            }
            #endif

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
            m_client->flush(FLUSH_ALL);

            // Update current client liast activity timestamp
            m_currentclient_lastactivity_timestamp = __i_instance.getUtilityInstance().millis_now();
        }

        // Only one client is served at a time, so an idle keep-alive connection
        // is given up as soon as another one is waiting. Holding it would leave
        // the waiting connection to be reset by the accept that follows it.
        else if( !m_clientRequest.isPending && m_server->hasClient() ){

            closeClient();
            m_currentclient_lastactivity_timestamp = 0;
            return;
        }

        // Check for keep-alive timeout
        if( m_currentclient_lastactivity_timestamp != 0 &&
            (__i_instance.getUtilityInstance().millis_now() - m_currentclient_lastactivity_timestamp) > HTTP_DEFAULT_KEEP_ALIVE_MS ){
            closeClient();
            m_currentclient_lastactivity_timestamp = 0;
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
    #if defined(ENABLE_HTTPS_SERVER) && defined(ENABLE_TLS_SERVICE)
    if( nullptr != m_insecure_server ){
        m_insecure_server->close();
    }
    #endif
}

#if defined(ENABLE_HTTPS_SERVER) && defined(ENABLE_TLS_SERVICE)
void HttpServerInterfaceImpl::sendHttpsRedirect(){

    pdiutil::string host = header(CHARPTR_WRAP(HTTP_HEADER_KEY_HOST));
    pdiutil::string::size_type port_sep = host.find(':');
    if( port_sep != pdiutil::string::npos ){
        host = host.substr(0, port_sep);
    }

    pdiutil::string location = CHARPTR_WRAP("https://");
    location += host;
    location += m_clientRequest.uri;

    addHeader(CHARPTR_WRAP(HTTP_HEADER_KEY_LOCATION), location);
    send(HTTP_RESP_MOVED_PERMANENTLY);
}
#endif

/**
 * on uri find call registered handler
 */
void HttpServerInterfaceImpl::on(const pdiutil::string &uri, CallBackVoidArgFn handler){
    uint32_t before = m_uriHandlerMap.size();
    m_uriHandlerMap.push_back({uri, handler});

    if (m_uriHandlerMap.size() == before) {
        SysLogE("HTTP: route %s not registered, out of memory\n", uri.c_str());
    }
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
 * check if argument exists. an argument submitted with an empty value still
 * exists, so a form clearing a field is not mistaken for a form never sent.
 */
bool HttpServerInterfaceImpl::hasArg(const pdiutil::string &name) const{
    for (uint32_t j = 0; j < m_clientRequest.queries.size(); j++){
        if (m_clientRequest.queries[j].isKeyMatch(name.c_str())) {
            return true;
        }
    }
    for (uint32_t j = 0; j < m_clientRequest.formdata.size(); j++){
        if (m_clientRequest.formdata[j].isKeyMatch(name.c_str())) {
            return true;
        }
    }
    for (uint32_t j = 0; j < m_clientRequest.files.size(); j++){
        if (m_clientRequest.files[j].isKeyMatch(name.c_str())) {
            return true;
        }
    }
    return false;
}

/**
 * isPostRequest
 * check if the request method is POST
 */
bool HttpServerInterfaceImpl::isPostRequest() const{
    pdiutil::string method_post = CHARPTR_WRAP("POST");
    return (m_clientRequest.method == method_post);
}

/**
 * collectHeaders
 * set the request headers to collect
 */
void HttpServerInterfaceImpl::collectHeaders(const char *headerKeys[], const size_t headerKeysCount){
    m_clientRequest.headers.clear();

    // Add default headers to collect list
    pdiutil::string host_key = CHARPTR_WRAP(HTTP_HEADER_KEY_HOST);
    pdiutil::string authorization_key = CHARPTR_WRAP(HTTP_HEADER_KEY_AUTHORIZATION);
    pdiutil::string connection_key = CHARPTR_WRAP(HTTP_HEADER_KEY_CONNECTION);
    pdiutil::string content_type_key = CHARPTR_WRAP(HTTP_HEADER_KEY_CONTENT_TYPE);
    pdiutil::string content_length_key = CHARPTR_WRAP(HTTP_HEADER_KEY_CONTENT_LENGTH);
    m_clientRequest.headers.push_back({host_key.c_str(), nullptr});
    m_clientRequest.headers.push_back({authorization_key.c_str(), nullptr});
    m_clientRequest.headers.push_back({connection_key.c_str(), nullptr});
    m_clientRequest.headers.push_back({content_type_key.c_str(), nullptr});
    m_clientRequest.headers.push_back({content_length_key.c_str(), nullptr});

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
    #ifdef ENABLE_STORAGE_SERVICE
    m_storagePath = storagepath.length() > 0 ? storagepath : CHARPTR_WRAP_RO(HTTP_SERVER_DEFAULT_STATIC_PATH);
    __i_instance.getFileSystemInstance().appendFileSeparator(m_storagePath);
    __i_instance.getFileSystemInstance().createDirectory(m_storagePath.c_str());
    #endif
}

/**
 * send
 */
void HttpServerInterfaceImpl::send(int code, mimetype_t content_type, const char *content, bool send_in_chunks){
    if( MIME_TYPE_MAX != content_type ){
        sendResponse(code, content_type, content, send_in_chunks);
    }else{
        sendResponse(code, MIME_TYPE_TEXT_HTML, "", send_in_chunks);
    }
}

/**
 * send content in chunk
 */
void HttpServerInterfaceImpl::sendChunk(const char *chunk){

    int32_t chunklen = strlen(chunk);
    
    if( chunk && chunklen >= 0 ){
    
        // Send the chunk in response
        char temp[20]; memset(temp, 0, 20);
        __snprintf(temp, 20, "%X\r\n", chunklen);

        sendPacket(m_client, (uint8_t *)temp, strlen(temp));
        __i_instance.getUtilityInstance().yield();
        sendPacket(m_client, (uint8_t *)chunk, chunklen, 400, 5000);
        __i_instance.getUtilityInstance().yield();
        sendPacket(m_client, (uint8_t *)"\r\n", 2);
        __i_instance.getUtilityInstance().yield();
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
    pdiutil::string content_type_key = CHARPTR_WRAP(HTTP_HEADER_KEY_CONTENT_TYPE);
    pdiutil::string content_length_key = CHARPTR_WRAP(HTTP_HEADER_KEY_CONTENT_LENGTH);

    while (1) {

        m_client->readLine(header_line, readLineYield, 256);
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

            if( key == content_type_key ){

                if( value.find(getMimeTypeString(MIME_TYPE_APPLICATION_X_WWW_FORM_URLENCODED)) != pdiutil::string::npos ){
                    isForm = false;
                    isEncoded = true;
                } else if( value.find(ROPTR_WRAP("multipart/")) != pdiutil::string::npos ){
                    isForm = true;
                    boundaryStr = value.substr(value.find('=') + 1);
                    boundaryStr.replace("\"","");
                }
            }else if( key == content_length_key ){
                contentLength = StringToUint32(value.c_str());
            }
        }
    }

    __i_instance.getUtilityInstance().yield(); // Yield to allow other tasks to run

    // read body if content length is specified
    if(!isForm && contentLength > 0) {

        char* body = pdiutil::safe_new_array<char>(contentLength + 1);
        if( body ){
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
            pdiutil::safe_delete_array(body);
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
        bool parthaslastread = false;
        bool found_end = false;
        bool upload_aborted = false;

        // Read until the end boundary is found
        uint32_t partprogressat = __i_instance.getUtilityInstance().millis_now();
        while (1) {

            pdiutil::string line;
            m_client->readLine(line, readLineYield);

            // The same rule as the body reader: a socket that yields nothing is
            // only worth waiting on while the peer is still there and the wait
            // is short, otherwise this loop would never end.
            if( line.empty() ){
                if( !m_client->connected() ||
                    (__i_instance.getUtilityInstance().millis_now() - partprogressat) > HTTP_UPLOAD_STALL_TIMEOUT_MS ){
                    LogE("HTTP upload: part stalled, connected=%d\n", (int)m_client->connected());
                    upload_aborted = true;
                    break;
                }
            }else{
                partprogressat = __i_instance.getUtilityInstance().millis_now();
            }

            if(parthaslastread){
                part += line;
                parthaslastread = false;
            }else{
                part = line;
            }

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
                        argname = part.substr(nameStart + 2, nameEnd - nameStart - 2);

                        pdiutil::string::size_type filenameStart = part.find(ROPTR_WRAP("filename=\""), nameEnd + 1);
                        if (filenameStart != pdiutil::string::npos) {
                            pdiutil::string::size_type filenameEnd = part.find("\"", filenameStart + 10);
                            argfilename = part.substr(filenameStart + 10, filenameEnd - filenameStart - 10);
                            #ifdef ENABLE_STORAGE_SERVICE
                            __i_instance.getFileSystemInstance().applyFileSizeLimit(argfilename);
                            #endif
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
                        if(!__i_instance.getFileSystemInstance().isDirExist(tempdir)){
                            __i_instance.getFileSystemInstance().createDirectory(tempdir);
                        }
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
                        pdiutil::string held;
                        uint32_t maxreadinonecall = HTTP_UPLOAD_READ_BLOCK_SIZE;
                        uint32_t lastprogressat = __i_instance.getUtilityInstance().millis_now();
                        uint32_t uploadedbytes = 0;
                        uint32_t loopcount = 0;

                        LogI("HTTP upload: begin, heap=%u\n",
                            (unsigned)__i_instance.getUtilityInstance().get_free_heap());

                        while (1) {

                            m_client->readStringUntil(part, '\r', true, readLineYield, maxreadinonecall);
                            m_client->readStringUntil(part, '\n', true, readLineYield, maxreadinonecall);

                            uint32_t heldbefore = lastread.length();
                            lastread += part;
                            uploadedbytes += part.length();

                            // rxQ is what lwip has handed over and not yet been
                            // drained, so a backlog pinned at its cap alongside a
                            // falling heap tells the two apart
                            if( (++loopcount % HTTP_UPLOAD_LOG_EVERY_N_BLOCKS) == 0 ){
                                LogI("HTTP upload: %u bytes, rxQ=%u, heap=%u\n",
                                    (unsigned)uploadedbytes,
                                    (unsigned)m_client->available(),
                                    (unsigned)__i_instance.getUtilityInstance().get_free_heap());
                            }

                            // A short append means the heap could not hold the
                            // block. Carrying on would spin without ever meeting
                            // the boundary, so the transfer is given up instead.
                            if( lastread.length() != heldbefore + part.length() ){
                                SysLogE("HTTP upload: out of memory at %u bytes, heap=%u\n",
                                    (unsigned)uploadedbytes,
                                    (unsigned)__i_instance.getUtilityInstance().get_free_heap());
                                upload_aborted = true;
                                break;
                            }

                            if( part.length() > 0 ){
                                lastprogressat = __i_instance.getUtilityInstance().millis_now();
                            }else if( !m_client->connected() ||
                                      (__i_instance.getUtilityInstance().millis_now() - lastprogressat) > HTTP_UPLOAD_STALL_TIMEOUT_MS ){
                                SysLogE("HTTP upload: stalled at %u bytes, connected=%d, rxQ=%u, heap=%u\n",
                                    (unsigned)uploadedbytes,
                                    (int)m_client->connected(),
                                    (unsigned)m_client->available(),
                                    (unsigned)__i_instance.getUtilityInstance().get_free_heap());
                                upload_aborted = true;
                                break;
                            }

                            pdiutil::string::size_type _endboundaryfound = lastread.find(end_boundary);
                            if(_endboundaryfound != pdiutil::string::npos){
                                lastread = lastread.substr(0, _endboundaryfound);
                                found_end = true;

                                // check for last write
                                if( filewritecounter == 0 && _endboundaryfound > maxreadinonecall ){
                                    lastread = lastread.substr(maxreadinonecall);
                                    filewritecounter = 1;
                                }
                            }
                            
                            pdiutil::string::size_type _boundaryfound = lastread.find(boundary);
                            if(_boundaryfound != pdiutil::string::npos){
                                // the two bytes skipped past the boundary are its
                                // trailing CRLF, which may not have arrived yet
                                // when the boundary ends the block
                                pdiutil::string::size_type _partat = _boundaryfound + boundary.length() + 2;
                                if( _partat > lastread.length() ){
                                    _partat = lastread.length();
                                }
                                part = lastread.substr(_partat);
                                lastread = lastread.substr(0, _boundaryfound);
                                found_boundary = true;
                                parthaslastread = true;

                                // check for last write
                                if( filewritecounter == 0 && _boundaryfound > maxreadinonecall ){
                                    lastread = lastread.substr(maxreadinonecall);
                                    filewritecounter = 1;
                                }
                            }

                            #ifdef ENABLE_STORAGE_SERVICE
                            if(++filewritecounter == 2){
                                if( !held.empty() ){
                                    __i_instance.getFileSystemInstance().writeFile(tempFilePath.c_str(), held.c_str(), held.length(), true);
                                    held.clear();
                                }
                                uint32_t llen = lastread.length();
                                if( llen >= 2 ){
                                    held += lastread[llen - 2];
                                    held += lastread[llen - 1];
                                    __i_instance.getFileSystemInstance().writeFile(tempFilePath.c_str(), lastread.c_str(), llen - 2, true);
                                }else{
                                    held += lastread;
                                }
                                filewritecounter = 0;
                            }
                            #else
                            // without file system support
                            if(argvalue.length() < 500 && ++filewritecounter == 2){
                                argvalue += lastread;
                            }
                            #endif

                            // If we found a boundary, we can stop reading further
                            if (found_boundary || found_end) {
                                break;
                            }

                            lastread = part; // Store the last read part
                            part.clear();

                            __i_instance.getUtilityInstance().yield();
                        }

                        LogI("HTTP upload: end, %u bytes, aborted=%d, heap=%u\n",
                            (unsigned)uploadedbytes, (int)upload_aborted,
                            (unsigned)__i_instance.getUtilityInstance().get_free_heap());

                        // An abandoned transfer leaves a partial file, which must
                        // not be handed on as if it were the uploaded one.
                        #ifdef ENABLE_STORAGE_SERVICE
                        if( upload_aborted ){
                            __i_instance.getFileSystemInstance().deleteFile(tempFilePath.c_str());
                        }else{
                            m_clientRequest.files.push_back({argname.c_str(), tempFilePath.c_str()});
                        }
                        #else
                        if( !upload_aborted ){
                            argvalue = argfilename + ':' + argvalue;
                            m_clientRequest.files.push_back({argname.c_str(), argvalue.c_str()});
                        }
                        #endif

                    }
                }
            }

            // Giving up on a part means the body can no longer be trusted, and
            // the peer has nothing more to send, so the part loop stops too
            // rather than reading an empty socket forever.
            if( upload_aborted ){
                break;
            }

            __i_instance.getUtilityInstance().wait(1);
        }

        __i_instance.getUtilityInstance().yield(); // Yield to allow other tasks to run
    }
}

/**
 * @brief Prepare the header for response
 */
void HttpServerInterfaceImpl::prepareResponseHeader(pdiutil::string& _header, int code, const char *content_type, uint32_t content_length, bool chunk_encoding){

    _header.clear();

    _header += m_clientRequest.version;
    _header += ' ';
    _header += pdiutil::to_string(code);
    _header += ' ';
    _header += getHttpStatusString(code);
    _header += "\r\n";

    addHeader(CHARPTR_WRAP(HTTP_HEADER_KEY_CONTENT_TYPE), content_type);

    if(chunk_encoding)
        addHeader(CHARPTR_WRAP(HTTP_HEADER_KEY_TRANSFER_ENCODING), CHARPTR_WRAP("chunked"));
    else
        addHeader(CHARPTR_WRAP(HTTP_HEADER_KEY_CONTENT_LENGTH), pdiutil::to_string(content_length));

    pdiutil::string connection_key = CHARPTR_WRAP(HTTP_HEADER_KEY_CONNECTION);
    pdiutil::string conn_close = CHARPTR_WRAP("close");
    pdiutil::string keepalive = header(connection_key);
    if (keepalive.empty() || keepalive == conn_close) {
        addHeader(connection_key, CHARPTR_WRAP("close"));
    } else {
        addHeader(connection_key, CHARPTR_WRAP("keep-alive"));

        keepalive = CHARPTR_WRAP("timeout=");
        keepalive += pdiutil::to_string((int)(HTTP_DEFAULT_KEEP_ALIVE_MS));
        addHeader(CHARPTR_WRAP(HTTP_HEADER_KEY_KEEP_ALIVE), keepalive);
        // (static_cast<iTcpClientInterface*>(m_client))->setKeepAlive((HTTP_DEFAULT_KEEP_ALIVE_MS/1000), 10, 3); // Enable keep-alive for the client
    }

    addHeader(CHARPTR_WRAP(HTTP_HEADER_KEY_ACCESS_CONTROL_ALLOW_ORIGIN), "*"); // Allow CORS

    #ifdef ENABLE_TLS_SERVICE
    if(m_secure && HTTPS_HSTS_MAX_AGE_SECONDS > 0){
        pdiutil::string hsts = CHARPTR_WRAP("max-age=");
        hsts += pdiutil::to_string((int)(HTTPS_HSTS_MAX_AGE_SECONDS));
        addHeader(CHARPTR_WRAP(HTTP_HEADER_KEY_STRICT_TRANSPORT_SECURITY), hsts);
    }
    #endif

    _header += m_responseHeaders;
    _header += "\r\n";
    m_responseHeaders = "";
}

/**
 * @brief Send response to client
 */
void HttpServerInterfaceImpl::sendResponse(int code, mimetype_t content_type, const char *content, bool chunk_encoding){

    if (!m_client || !content) {
        return; // Client not initialized
    }

    pdiutil::string response;
    prepareResponseHeader(response, code, getMimeTypeString(content_type), strlen(content), chunk_encoding);

    // Send the response headers
    sendPacket(m_client, (uint8_t *)response.c_str(), response.length());

    // Send the response body
    if(chunk_encoding){

        if(strlen(content))
            sendChunk(content);
    }else{
        sendPacket(m_client, (uint8_t *)content, strlen(content), 400, 3000);
    }

    // Make sure data has been sent
    // m_client->write((const uint8_t*)"", 0);
}

/**
 * @brief handle file request.
 */
bool HttpServerInterfaceImpl::handleStaticFileRequest(){

    bool bStatus = false;

    #ifdef ENABLE_STORAGE_SERVICE

    pdiutil::string method_get = CHARPTR_WRAP("GET");
    pdiutil::string method_head = CHARPTR_WRAP("HEAD");
    if (m_clientRequest.method == method_get || m_clientRequest.method == method_head) {

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
            addHeader(CHARPTR_WRAP(HTTP_HEADER_KEY_CONTENT_DISPOSITION), content_disposition_value);

            pdiutil::string response;
            prepareResponseHeader(response, HTTP_RESP_OK, getMimeTypeString(filetype), __i_instance.getFileSystemInstance().getFileSize(filePath.c_str()));

            // Send the response headers
            sendPacket(m_client, (uint8_t *)response.c_str(), response.length());

            // Send the response body
            int iStatus = __i_instance.getFileSystemInstance().readFile(filePath.c_str(), 400, [&](char* data, uint32_t size)->bool{
                sendPacket(m_client, (uint8_t *)data, size, 400, 3000);
                // return true to continue reading
                return true;
            });

            // Make sure data has been sent
            // m_client->write((const uint8_t*)"", 0);

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
        pdiutil::safe_delete(m_client);
        m_client = nullptr;
    }
}

#endif