/**************************** OTA Config HTML Page ****************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `OtaConfigPage.h` file defines the HTML content for the OTA (Over-The-Air)
configuration page of the web server. This page allows users to configure OTA
update settings through a web interface. The HTML content is stored in program
memory (PROG_RODT_ATTR) to optimize memory usage on embedded systems.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_OTA_CONFIG_PAGE_H_
#define _WEB_SERVER_OTA_CONFIG_PAGE_H_

#include <interface/pdi.h>

/**
 * @brief HTML content for the OTA configuration page.
 *
 * This static HTML content is used to render the top section of the OTA
 * configuration page on the web server. It includes a form for submitting OTA
 * update settings.
 */
static const char WEB_SERVER_OTA_CONFIG_PAGE_TOP[] PROG_RODT_ATTR = "\
<h2>OTA Configuration</h2>\
<form action='/ota-config' method='POST'>\
<table>";

#ifdef ENABLE_STORAGE_SERVICE

/**
 * @brief Opening of the local image flashing form.
 *
 * Offers the firmware images already present on the filesystem, so a build can
 * be uploaded through the storage page and flashed without an update server.
 */
static const char WEB_SERVER_OTA_LOCAL_FLASH_TOP[] PROG_RODT_ATTR = "\
<hr style='border:0;border-top:1px solid var(--ln);margin:18px 0;'>\
<h2>Flash From Storage</h2>\
<form action='/ota-config' method='POST'>\
<table>";

/**
 * @brief Submit row of the local image flashing form.
 */
static const char WEB_SERVER_OTA_LOCAL_FLASH_BOTTOM[] PROG_RODT_ATTR = "\
<tr>\
<td></td>\
<td>\
<button class='btn' type='submit' onclick='return confirm(\"Flash this image and restart?\")'>\
Flash\
</button>\
</td>\
</tr>\
</table>\
</form>";

/**
 * @brief Shown when the filesystem holds no firmware image.
 */
static const char WEB_SERVER_OTA_NO_LOCAL_IMAGE[] PROG_RODT_ATTR = "\
</table>\
</form>\
<h4>No firmware image on storage. Upload a .bin file first from storage menu.</h4>";

#endif

#endif
