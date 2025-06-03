/*********************************** Controller *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `Controller.h` file defines the `Controller` class, which serves as a base
class for managing web server controllers. It provides functionality for
registering controllers, initializing resources, and defining routes. The file
also includes a structure for managing a collection of controllers.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _SERVER_CONTROLLER_
#define _SERVER_CONTROLLER_

#include <webserver/handlers/RouteHandler.h>

/**
 * @class Controller
 * @brief Forward Declaration of Base class for web server controllers.
 */
class Controller;

/**
 * @struct struct_controllers
 * @brief Represents a structure for managing controllers.
 *
 * This structure holds a pointer to a `Controller` instance and provides
 * a constructor for initializing it.
 */
struct struct_controllers {
    Controller *controller; /**< Pointer to the associated controller. */

    /**
     * @brief Constructor for `struct_controllers`.
     *
     * Initializes the structure with the given controller.
     *
     * @param _c Pointer to the controller instance.
     */
    struct_controllers(Controller *_c) : controller(_c) {}
};

/**
 * @class Controller
 * @brief Base class for web server controllers.
 *
 * The `Controller` class provides a foundation for managing web server
 * controllers. It includes methods for registering controllers, initializing
 * resources, and defining routes. Derived classes must override the `boot`
 * method to implement specific functionality.
 */
class Controller {

public:
    /**
     * @brief Default constructor for the `Controller` class.
     *
     * Initializes the controller with default values.
     */
    Controller();

    /**
     * @brief Parameterized constructor for the `Controller` class.
     *
     * Initializes the controller with the specified name.
     *
     * @param _controller_name The name of the controller.
     */
    Controller(const char *_controller_name);

    /**
     * @brief Destructor for the `Controller` class.
     *
     * Cleans up resources used by the controller.
     */
    ~Controller();

    /**
     * @brief Registers a controller.
     *
     * Adds the specified controller to the list of registered controllers.
     *
     * @param that Pointer to the controller to register.
     */
    void register_controller(Controller *that);

    /**
     * @brief Abstract method to initialize the controller.
     *
     * Derived classes must override this method to implement specific
     * initialization logic.
     */
    virtual void boot() = 0;

    /**
     * @var pdiutil::vector<struct_controllers> m_controllers
     * @brief Static vector to store registered controllers.
     */
    static pdiutil::vector<struct_controllers> m_controllers;

    /**
     * @var const char* m_controller_name
     * @brief Name of the controller.
     */
    const char *m_controller_name;

protected:
    /**
     * @var WebResourceProvider* m_web_resource
     * @brief Pointer to the web resource provider.
     */
    WebResourceProvider *m_web_resource;

    /**
     * @var RouteHandler* m_route_handler
     * @brief Pointer to the route handler.
     */
    RouteHandler *m_route_handler;
};

#endif
