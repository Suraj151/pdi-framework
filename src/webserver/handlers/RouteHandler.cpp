/****************************** Route Handler *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "RouteHandler.h"

/**
 * RouteHandler constructor
 */
RouteHandler::RouteHandler(){
}

/**
 * RouteHandler destructor
 */
RouteHandler::~RouteHandler(){
}

/**
 * register route, callbacks, middlware level etc.
 *
 * @param const char* _uri
 * @param CallBackVoidArgFn _fn
 * @param middlwares|NO_MIDDLEWARE _middleware_level
 * @param const char*|WEB_SERVER_LOGIN_ROUTE _redirect_uri
 */
void RouteHandler::register_route( const char* _uri, CallBackVoidArgFn _fn, middlwares _middleware_level, const char* _redirect_uri ){

  if( nullptr != __web_resource.m_server ){

    __web_resource.m_server->on( _uri, [=]() {
      if( this->handle_middleware(_middleware_level,_redirect_uri) )
      _fn();
    } );
  }
}

/**
 * register not found callback
 *
 * @param CallBackVoidArgFn _fn
 */
void RouteHandler::register_not_found_fn( CallBackVoidArgFn _fn ){

  if( nullptr != __web_resource.m_server ){

    __web_resource.m_server->onNotFound( _fn );
  }
}

RouteHandler __web_route_handler;
