/************************** Dashboard html page *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_DASHBOARD_PAGE_H_
#define _WEB_SERVER_DASHBOARD_PAGE_H_

#include <Arduino.h>

static const char WEB_SERVER_DASHBOARD_PAGE[] PROGMEM = "\
<h2>Dashboard</h2>\
\
<style>\
#stnm,#stip,#strs,#stst,#stmc{\
text-align:left;\
}\
td{\
border:1px solid #d0d0d0;\
padding:4px;\
min-width:100px;\
}\
</style>\
\
<table>\
<tr>\
<td>station ssid</td>\
<td id='stnm'></td>\
</tr>\
<tr>\
<td>ip</td>\
<td id='stip'></td>\
</tr>\
<tr>\
<td>rssi</td>\
<td id='strs'></td>\
</tr>\
<tr>\
<td>status</td>\
<td id='stst'></td>\
</tr>\
<tr>\
<td>mac</td>\
<td id='stmc'></td>\
</tr>\
<tr>\
<td>internet</td>\
<td id='inet'></td>\
</tr>\
<tr>\
<td>n/w time</td>\
<td id='nwt'></td>\
</tr>\
</table>\
\
<h2>Connected Devices</h2>\
\
<table id='cndl'>\
</table>";
#endif
