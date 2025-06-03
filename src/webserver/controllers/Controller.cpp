/*********************************** Controller *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `Controller.cpp` file implements the `Controller` class, which serves as a
base class for managing web server controllers. It provides functionality for
registering controllers, initializing resources, and defining routes.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_HTTP_SERVER)

#include "Controller.h"

/**
 * @var pdiutil::vector<struct_controllers> Controller::m_controllers
 * @brief Static vector to store registered controllers.
 */
pdiutil::vector<struct_controllers> Controller::m_controllers;

/**
 * @brief Default constructor for the `Controller` class.
 *
 * Initializes the controller with default values and registers it in the
 * global list of controllers.
 */
Controller::Controller():
  m_controller_name("controller"),
  m_web_resource(&__web_resource),
  m_route_handler(&__web_route_handler)
{
  this->register_controller(this);
}

/**
 * @brief Parameterized constructor for the `Controller` class.
 *
 * Initializes the controller with the specified name and registers it in the
 * global list of controllers.
 *
 * @param _controller_name The name of the controller.
 */
Controller::Controller(const char* _controller_name):
  m_controller_name(_controller_name),
  m_web_resource(&__web_resource),
  m_route_handler(&__web_route_handler)
{
  this->register_controller(this);
}

/**
 * @brief Destructor for the `Controller` class.
 *
 * Cleans up resources used by the controller, including resetting pointers
 * to the controller name, web resource, and route handler.
 */
Controller::~Controller() {
  this->m_controller_name = nullptr;
  this->m_web_resource = nullptr;
  this->m_route_handler = nullptr;
}

/**
 * @brief Registers a controller.
 *
 * Adds the specified controller to the global list of registered controllers.
 *
 * @param that Pointer to the controller to register.
 */
void Controller::register_controller(Controller* that) {
  struct_controllers _c(that);
  this->m_controllers.push_back(_c);
}

#endif