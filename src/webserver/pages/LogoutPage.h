/**************************** logout html page ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _EW_SERVER_LOGOUT_PAGE_H_
#define _EW_SERVER_LOGOUT_PAGE_H_

#include <Arduino.h>

static const char EW_SERVER_LOGOUT_PAGE[] PROGMEM = "\
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
