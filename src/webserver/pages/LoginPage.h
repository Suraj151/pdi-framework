/**************************** Login HTML Page *********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `LoginPage.h` file defines the HTML content for the login page of the web
server. This page allows users to log in by providing their username and
password. The HTML content is stored in program memory (PROG_RODT_ATTR) to optimize
memory usage on embedded systems.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_LOGIN_PAGE_H_
#define _WEB_SERVER_LOGIN_PAGE_H_

#include <interface/pdi.h>

/**
 * @brief HTML content for the login page.
 *
 * This static HTML content is used to render the login page of the web server.
 * It includes a form with fields for entering a username and password, and a
 * button to submit the login credentials.
 */
static const char WEB_SERVER_LOGIN_PAGE[] PROG_RODT_ATTR = "\
<h2>Login to Continue !</h2>\
<form action='/login' method='POST'>\
<table>\
<tr>\
<td>Username:</td>\
<td><input type='text' name='username' maxlength='20'/></td>\
</tr>\
<tr>\
<td>Password:</td>\
<td><input type='password' name='password' maxlength='20'/></td>\
</tr>\
<tr>\
<td></td>\
<td>\
<button class='btn' type='submit'>\
Login\
</button>\
</td>\
</tr>\
</table>\
</form>";

#endif
