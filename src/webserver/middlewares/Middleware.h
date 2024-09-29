/******************************** Middleware **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _MIDDLEWARE_PROVIDER_
#define _MIDDLEWARE_PROVIDER_

#include <webserver/resources/WebResource.h>
#include <webserver/handlers/SessionHandler.h>

/**
 * @enum middlewares
 */
enum middlwares {
  AUTH_MIDDLEWARE,
  API_MIDDLEWARE,
  NO_MIDDLEWARE
};

/**
 * Middleware class
 * @parent  EwSessionHandler|protected
 */
class Middleware  : public EwSessionHandler{

  public:

    /**
		 * Middleware constructor
		 */
    Middleware( void ){
    }

    /**
		 * Middleware destructor
		 */
    ~Middleware(){
    }

    /**
		 * each request go through this middelware to check requests by parameters.
     * this method allow to redirect or unautherise any request based on conditions.
     *
     * @param   middlwares _middleware_level
     * @param   const char* _redirect_uri
     * @return  bool
		 */
    bool handle_middleware( middlwares _middleware_level, const char* _redirect_uri ){

      LogI("checking through middleware\n");

      if ( _middleware_level == AUTH_MIDDLEWARE ) {

        if ( !this->has_active_session() ) {

          if( nullptr != __web_resource.m_server ){

            __web_resource.m_server->sendHeader("Location", _redirect_uri);
            __web_resource.m_server->sendHeader("Cache-Control", "no-cache");
            __web_resource.m_server->send(HTTP_REDIRECT);
          }
          return false;
        }
        return true;
      }else if ( _middleware_level == API_MIDDLEWARE ) {

        return true;

      }else{

        return true;
      }
    }
};

#endif
