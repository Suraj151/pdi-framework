/*************************** email config html page ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_EMAIL_CONFIG_PAGE_H_
#define _WEB_SERVER_EMAIL_CONFIG_PAGE_H_

#include <Arduino.h>

static const char WEB_SERVER_EMAIL_CONFIG_PAGE_TOP[] PROGMEM = "\
<h2>Email Configuration</h2>\
<form action='/email-config' method='POST'>\
<table>";

#endif
