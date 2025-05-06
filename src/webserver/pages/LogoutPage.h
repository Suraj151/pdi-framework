/**************************** Logout HTML Page ********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `LogoutPage.h` file defines the HTML content for the logout page of the web
server. This page is displayed to users after they have successfully logged out.
It provides a message confirming the logout and a link to navigate back to the
login page. The HTML content is stored in program memory (PROG_RODT_ATTR) to optimize
memory usage on embedded systems.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_LOGOUT_PAGE_H_
#define _WEB_SERVER_LOGOUT_PAGE_H_

#include <interface/pdi.h>

/**
 * @brief HTML content for the logout page.
 *
 * This static HTML content is used to render the logout page of the web server.
 * It includes a message confirming the logout and a button linking to the login
 * page for reauthentication.
 */
static const char WEB_SERVER_LOGOUT_PAGE[] PROG_RODT_ATTR = "\
<h3>Logged Out Successfully</h3>\
<div>\
<a href='/login'>\
<button class='btn'>\
Click Here\
</button>\
</a>\
to login again\
</div>";

#endif
