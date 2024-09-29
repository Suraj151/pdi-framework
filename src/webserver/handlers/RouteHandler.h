/****************************** Route Handler *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _EW_SERVER_ROUTE_HANDLER_
#define _EW_SERVER_ROUTE_HANDLER_

#include <webserver/middlewares/Middleware.h>

/**
 * RouteHandler class
 */
class RouteHandler : public Middleware{

  public:

    /**
		 * RouteHandler constructor
		 */
    RouteHandler();
    /**
		 * RouteHandler destructor
		 */
    ~RouteHandler();

    void register_route( const char* _uri, CallBackVoidArgFn _fn, middlwares _middleware_level=NO_MIDDLEWARE, const char* _redirect_uri=EW_SERVER_LOGIN_ROUTE );
    void register_not_found_fn( CallBackVoidArgFn _fn );
};

extern RouteHandler __web_route_handler;

#endif
