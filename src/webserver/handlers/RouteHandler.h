/****************************** Route Handler *********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `RouteHandler.h` file defines the `RouteHandler` class, which is responsible
for managing HTTP routes in the web server. It provides methods to register
routes, associate them with callback functions, and handle middleware processing
for each route. The class also allows for defining a custom "not found" handler
for unregistered routes.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_ROUTE_HANDLER_
#define _WEB_SERVER_ROUTE_HANDLER_

#include <webserver/middlewares/Middleware.h>

/**
 * @class RouteHandler
 * @brief Handles HTTP route registration and processing.
 *
 * The `RouteHandler` class extends the `Middleware` class to provide
 * functionality for managing HTTP routes in the web server. It allows
 * developers to register routes, associate them with callback functions,
 * and apply middleware for authentication or validation. It also supports
 * defining a custom handler for unregistered routes.
 */
class RouteHandler : public Middleware {

  public:

    /**
     * @brief Constructor for the `RouteHandler` class.
     *
     * Initializes the route handler.
     */
    RouteHandler();

    /**
     * @brief Destructor for the `RouteHandler` class.
     *
     * Cleans up resources used by the route handler.
     */
    ~RouteHandler();

    /**
     * @brief Registers a new HTTP route.
     *
     * Associates a URI with a callback function and an optional middleware
     * level. If middleware is applied, the route will enforce authentication
     * or validation before executing the callback.
     *
     * @param _uri The URI of the route to register.
     * @param _fn The callback function to execute when the route is accessed.
     * @param _middleware_level The middleware level to apply (default: NO_MIDDLEWARE).
     * @param _redirect_uri The URI to redirect to if middleware validation fails
     *                      (default: WEB_SERVER_LOGIN_ROUTE).
     */
    void register_route(const char* _uri, CallBackVoidArgFn _fn, middlwares _middleware_level = NO_MIDDLEWARE, const char* _redirect_uri = WEB_SERVER_LOGIN_ROUTE);

    /**
     * @brief Registers a "not found" handler.
     *
     * Defines a callback function to execute when a requested route is not
     * found in the registered routes.
     *
     * @param _fn The callback function to execute for unregistered routes.
     */
    void register_not_found_fn(CallBackVoidArgFn _fn);
};

/**
 * @brief Global instance of the `RouteHandler` class.
 *
 * This instance is used to manage routes across the web server.
 */
extern RouteHandler __web_route_handler;

#endif
