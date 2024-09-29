/*********************************** Controller *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "Controller.h"

/**
 * @var	std::vector<struct_controllers>	m_controllers
 */
std::vector<struct_controllers> Controller::m_controllers;

/**
 * Controller constructor
 */
Controller::Controller():
  m_controller_name("controller"),
  m_web_resource(&__web_resource),
  m_route_handler(&__web_route_handler)
{
  this->register_controller(this);
}

/**
 * Controller constructor
 */
Controller::Controller(const char* _controller_name):
  m_controller_name(_controller_name),
  m_web_resource(&__web_resource),
  m_route_handler(&__web_route_handler)
{
  this->register_controller(this);
}

/**
 * Controller destructor
 */
Controller::~Controller(){
  this->m_controller_name = nullptr;
  this->m_web_resource = nullptr;
  this->m_route_handler = nullptr;
}

/**
 * Register Controller
 */
void Controller::register_controller(Controller* that){
  struct_controllers _c(that);
  this->m_controllers.push_back(_c);
}
