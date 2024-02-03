/*********************************** Controller *******************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _SERVER_CONTROLLER_
#define _SERVER_CONTROLLER_

#include <webserver/handlers/RouteHandler.h>

class Controller;
/**
 * define controller structure here
 */
struct struct_controllers
{
	Controller *controller;
	struct_controllers(Controller *_c) : controller(_c) {}
};

/**
 * Controller class
 */
class Controller
{

public:
	/**
	 * Controller constructor
	 */
	Controller();
	/**
	 * Controller constructor
	 */
	Controller(const char *_controller_name);
	/**
	 * Controller destructor
	 */
	~Controller();

	/**
	 * Register Controller
	 */
	void register_controller(Controller *that);

	/**
	 * override boot
	 */
	virtual void boot() = 0;

	/**
	 * @var	std::vector<struct_controllers>	m_controllers
	 */
	static std::vector<struct_controllers> m_controllers;

	/**
	 * @var	const char*	m_controller_name
	 */
	const char *m_controller_name;

protected:
	/**
	 * @var	WebResourceProvider*	m_web_resource
	 */
	WebResourceProvider *m_web_resource;
	/**
	 * @var	RouteHandler*	m_route_handler
	 */
	RouteHandler *m_route_handler;
};

#endif
