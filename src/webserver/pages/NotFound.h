/************************** 404 Not Found HTML Page ****************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `NotFound.h` file defines the HTML content for the "404 Not Found" page of
the web server. This page is displayed when a user attempts to access a route
or resource that does not exist. It provides a message indicating the page was
not found and includes a link to navigate back to the home page. The HTML content
is stored in program memory (PROGMEM) to optimize memory usage on embedded systems.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_404_PAGE_H_
#define _WEB_SERVER_404_PAGE_H_

#include <Arduino.h>

/**
 * @brief HTML content for the "404 Not Found" page.
 *
 * This static HTML content is used to render the "404 Not Found" page of the
 * web server. It includes a message indicating the page was not found and a
 * button linking to the home page.
 */
static const char WEB_SERVER_404_PAGE[] PROGMEM = "\
<h3>Page Not Found</h3>\
<div>\
Go to\
<a href='/'>\
<button class='btn'>\
Home\
</button>\
</a>\
</div>";

#endif
