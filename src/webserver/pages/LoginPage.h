/**************************** login html page *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_LOGIN_PAGE_H_
#define _WEB_SERVER_LOGIN_PAGE_H_

#include <Arduino.h>

static const char WEB_SERVER_LOGIN_PAGE[] PROGMEM = "\
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
