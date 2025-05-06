/************************** Login Config HTML Page ****************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `LoginConfigPage.h` file defines the HTML content for the login configuration
page of the web server. This page allows users to configure login credentials,
such as username and password, through a web interface. The HTML content is
stored in program memory (PROG_RODT_ATTR) to optimize memory usage on embedded systems.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_LOGIN_CONFIG_PAGE_H_
#define _WEB_SERVER_LOGIN_CONFIG_PAGE_H_

#include <interface/pdi.h>

/**
 * @brief HTML content for the login configuration page.
 *
 * This static HTML content is used to render the top section of the login
 * configuration page on the web server. It includes a form for submitting
 * login credentials.
 */
static const char WEB_SERVER_LOGIN_CONFIG_PAGE_TOP[] PROG_RODT_ATTR = "\
<h2>Login Configuration</h2>\
<form action='/login-config' method='POST'>\
<table>";

#endif
