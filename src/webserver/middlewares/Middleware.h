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

            __web_resource.m_server->addHeader(CHARPTR_WRAP(HTTP_HEADER_KEY_LOCATION), _redirect_uri);
            __web_resource.m_server->addHeader(CHARPTR_WRAP_RO(HTTP_HEADER_KEY_CACHE_CONTROL), CHARPTR_WRAP_RO(HTTP_HEADER_VALUE_NO_CACHE));
            __web_resource.m_server->send(HTTP_RESP_MOVED_PERMANENTLY);
          }
          return false;
        }
        return this->guard_state_change(_redirect_uri, false);
      } else if (_middleware_level == API_MIDDLEWARE) {

        if (!this->has_active_session()) {

          if (nullptr != __web_resource.m_server) {
            __web_resource.m_server->addHeader(CHARPTR_WRAP_RO(HTTP_HEADER_KEY_CACHE_CONTROL), CHARPTR_WRAP_RO(HTTP_HEADER_VALUE_NO_CACHE));
            __web_resource.m_server->send(HTTP_RESP_UNAUTHORIZED);
          }
          return false;
        }
        return this->guard_state_change(_redirect_uri, true);

      } else {

        return true;
      }
    }

    /**
     * @brief Rejects a state changing request that carries no valid csrf token.
     *
     * Every POST behind an authenticated route must present the csrf token of
     * its session, so a third party page cannot drive the portal on behalf of
     * a logged in browser.
     *
     * @param _redirect_uri The URI to redirect to when the token is missing.
     * @param _is_api Whether to answer with a status code instead of a redirect.
     * @return `true` if the request may proceed, `false` otherwise.
     */
    bool guard_state_change(const char* _redirect_uri, bool _is_api) {

      if (nullptr == __web_resource.m_server) {
        return false;
      }

      if (!__web_resource.m_server->isPostRequest()) {
        return true;
      }

      if (this->has_valid_csrf_token()) {
        return true;
      }

      LogW("csrf token rejected\n");

      __web_resource.m_server->addHeader(CHARPTR_WRAP_RO(HTTP_HEADER_KEY_CACHE_CONTROL), CHARPTR_WRAP_RO(HTTP_HEADER_VALUE_NO_CACHE));

      if (_is_api) {
        __web_resource.m_server->send(HTTP_RESP_FORBIDDEN);
      } else {
        __web_resource.m_server->addHeader(CHARPTR_WRAP(HTTP_HEADER_KEY_LOCATION), _redirect_uri);
        __web_resource.m_server->send(HTTP_RESP_MOVED_PERMANENTLY);
      }

      return false;
    }
};

#endif
