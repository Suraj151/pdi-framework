/************************** login config html page ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _EW_SERVER_LOGIN_CONFIG_PAGE_H_
#define _EW_SERVER_LOGIN_CONFIG_PAGE_H_

#include <Arduino.h>

static const char EW_SERVER_LOGIN_CONFIG_PAGE_TOP[] PROGMEM = "\
<h2>Login Configuration</h2>\
<form action='/login-config' method='POST'>\
<table>";

#endif
