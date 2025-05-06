/************************** Dashboard HTML Page *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `Dashboard.h` file defines the HTML content for the dashboard page of the
web server. The dashboard provides information about the device's network
status, connected devices, and other relevant details. The HTML content is
stored in program memory (PROG_RODT_ATTR) to optimize memory usage on embedded systems.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_DASHBOARD_PAGE_H_
#define _WEB_SERVER_DASHBOARD_PAGE_H_

#include <interface/pdi.h>

/**
 * @brief HTML content for the dashboard page.
 *
 * This static HTML content is used to render the dashboard page on the web
 * server. It includes a table displaying network information such as SSID, IP
 * address, RSSI, status, MAC address, internet connectivity, and network time.
 * Additionally, it provides a section for listing connected devices.
 */
static const char WEB_SERVER_DASHBOARD_PAGE[] PROG_RODT_ATTR = "\
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
