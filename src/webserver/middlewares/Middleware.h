/******************************** Middleware **********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `Middleware` class provides a mechanism to process HTTP requests before
they reach their intended handlers. It allows for authentication checks,
API-level validations, and request redirection based on specific conditions.
The middleware integrates with the session handler to manage user sessions.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _MIDDLEWARE_PROVIDER_
#define _MIDDLEWARE_PROVIDER_

#include <webserver/resources/WebResource.h>
#include <webserver/handlers/SessionHandler.h>

/**
 * @enum middlewares
 * @brief Defines the levels of middleware processing.
 *
 * - `AUTH_MIDDLEWARE`: Middleware for authentication checks.
 * - `API_MIDDLEWARE`: Middleware for API-level validations.
 * - `NO_MIDDLEWARE`: No middleware processing.
 */
enum middlwares {
  AUTH_MIDDLEWARE,
  API_MIDDLEWARE,
  NO_MIDDLEWARE
};

/**
 * @class Middleware
 * @brief Handles middleware processing for HTTP requests.
 *
 * The `Middleware` class processes incoming HTTP requests to enforce
 * authentication, validate API requests, or redirect unauthorized users.
 * It extends the `EwSessionHandler` class to manage user sessions.
 */
class Middleware : public EwSessionHandler {

  public:

    /**
     * @brief Constructor for the `Middleware` class.
     *
     * Initializes the middleware with default settings.
     */
    Middleware(void) {
    }

    /**
     * @brief Destructor for the `Middleware` class.
     *
     * Cleans up resources used by the middleware.
     */
    ~Middleware() {
    }

    /**
     * @brief Processes an HTTP request through the middleware.
     *
     * This method checks the request against the specified middleware level
     * and performs actions such as authentication checks or redirection.
     *
     * @param _middleware_level The middleware level to apply (e.g., `AUTH_MIDDLEWARE`).
     * @param _redirect_uri The URI to redirect to if the request is unauthorized.
     * @return `true` if the request passes the middleware checks, `false` otherwise.
     */
    bool handle_middleware(middlwares _middleware_level, const char* _redirect_uri) {

      LogI("checking through middleware\n");

      if (_middleware_level == AUTH_MIDDLEWARE) {

        if (!this->has_active_session()) {

          if (nullptr != __web_resource.m_server) {

            __web_resource.m_server->addHeader("Location", _redirect_uri);
            __web_resource.m_server->addHeader("Cache-Control", "no-cache");
            __web_resource.m_server->send(HTTP_RESP_MOVED_PERMANENTLY);
          }
          return false;
        }
        return true;
      } else if (_middleware_level == API_MIDDLEWARE) {

        return true;

      } else {

        return true;
      }
    }
};

#endif
