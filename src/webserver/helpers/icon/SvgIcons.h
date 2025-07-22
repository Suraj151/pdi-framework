/********************************* SVG Icons **********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `SvgIcons.h` file defines a collection of SVG path data for various icons
used in the web server's user interface. These icons are stored in program
memory (PROG_RODT_ATTR) to optimize memory usage on embedded systems. The icons include
representations for account, WiFi, cloud download, send, memory, mail, dashboard,
power, and more.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _EW_SVG_ICON_PAGE_H_
#define _EW_SVG_ICON_PAGE_H_

#include <webserver/helpers/HtmlTagsAndAttr.h>

/**
 * @brief HTML wrapper for menu card page (top section).
 *
 * This static HTML content is used to render the top section of the menu card
 * page on the web server.
 */
static const char WEB_SERVER_MENU_CARD_PAGE_WRAP_TOP[] PROG_RODT_ATTR = "\
<div id='mncntr'>";

/**
 * @brief HTML wrapper for menu card page (bottom section).
 *
 * This static HTML content is used to close the menu card page wrapper.
 */
static const char* WEB_SERVER_MENU_CARD_PAGE_WRAP_BOTTOM = HTML_DIV_CLOSE_TAG;

/**
 * @brief SVG path data for the "Account Circle" icon.
 */
static const char SVG_ICON48_PATH_ACCOUNT_CIRCLE[] PROG_RODT_ATTR = "\
<path d='M24 4C12.95 4 4 12.95 4 24s8.95 20 20 20 20-8.95 20-20S35.05 4 24 4zm0 6c3.31 0 6 2.69 6 6 0 3.32-2.69 6-6 6s-6-2.68-6-6c0-3.31 2.69-6 6-6zm0 28.4c-5.01 0-9.41-2.56-12-6.44.05-3.97 8.01-6.16 12-6.16s11.94 2.19 12 6.16c-2.59 3.88-6.99 6.44-12 6.44z'/>";

/**
 * @brief SVG path data for the "WiFi" icon.
 */
static const char SVG_ICON48_PATH_WIFI[] PROG_RODT_ATTR = "\
<path d='M2 18l4 4c9.94-9.94 26.06-9.94 36 0l4-4C33.85 5.85 14.15 5.85 2 18zm16 16l6 6 6-6c-3.31-3.31-8.69-3.31-12 0zm-8-8l4 4c5.52-5.52 14.48-5.52 20 0l4-4c-7.73-7.73-20.27-7.73-28 0z'/>";

/**
 * @brief SVG path data for the "Cloud Download" icon.
 */
static const char SVG_ICON48_PATH_CLOUD_DOWNLOAD[] PROG_RODT_ATTR = "\
<path d='M38.71 20.07C37.35 13.19 31.28 8 24 8c-5.78 0-10.79 3.28-13.3 8.07C4.69 16.72 0 21.81 0 28c0 6.63 5.37 12 12 12h26c5.52 0 10-4.48 10-10 0-5.28-4.11-9.56-9.29-9.93zM34 26L24 36 14 26h6v-8h8v8h6z'/>";

/**
 * @brief SVG path data for the "Send" icon.
 */
static const char SVG_ICON48_PATH_SEND[] PROG_RODT_ATTR = "\
<path d='M4.02 42L46 24 4.02 6 4 20l30 4-30 4z'/>";

/**
 * @brief SVG path data for the "Memory" icon.
 */
static const char SVG_ICON48_PATH_MEMORY[] PROG_RODT_ATTR = "\
<path d='M30 18H18v12h12V18zm-4 8h-4v-4h4v4zm16-4v-4h-4v-4c0-2.21-1.79-4-4-4h-4V6h-4v4h-4V6h-4v4h-4c-2.21 0-4 1.79-4 4v4H6v4h4v4H6v4h4v4c0 2.21 1.79 4 4 4h4v4h4v-4h4v4h4v-4h4c2.21 0 4-1.79 4-4v-4h4v-4h-4v-4h4zm-8 12H14V14h20v20z'/>";

/**
 * @brief SVG path data for the "Mail" icon.
 */
static const char SVG_ICON48_PATH_MAIL[] PROG_RODT_ATTR = "\
<path d='M40 8H8c-2.21 0-3.98 1.79-3.98 4L4 36c0 2.21 1.79 4 4 4h32c2.21 0 4-1.79 4-4V12c0-2.21-1.79-4-4-4zm0 8L24 26 8 16v-4l16 10 16-10v4z'/>";

/**
 * @brief SVG path data for the "Dashboard" icon.
 */
static const char SVG_ICON48_PATH_DASHBOARD[] PROG_RODT_ATTR = "\
<path d='M6 26h16V6H6v20zm0 16h16V30H6v12zm20 0h16V22H26v20zm0-36v12h16V6H26z'/>";

/**
 * @brief SVG path data for the "Power" icon.
 */
static const char SVG_ICON48_PATH_POWER[] PROG_RODT_ATTR = "\
<path d='M26 6h-4v20h4V6zm9.67 4.33l-2.83 2.83C35.98 15.73 38 19.62 38 24c0 7.73-6.27 14-14 14s-14-6.27-14-14c0-4.38 2.02-8.27 5.16-10.84l-2.83-2.83C8.47 13.63 6 18.52 6 24c0 9.94 8.06 18 18 18s18-8.06 18-18c0-5.48-2.47-10.37-6.33-13.67z'/>";

/**
 * @brief SVG path data for the "Swap Calls" icon.
 */
static const char SVG_ICON48_PATH_SWAP_CALLS[] PROG_RODT_ATTR = "\
<path d='M36 8l-8 8h6v14c0 2.21-1.79 4-4 4s-4-1.79-4-4V16c0-4.41-3.59-8-8-8s-8 3.59-8 8v14H4l8 8 8-8h-6V16c0-2.21 1.79-4 4-4s4 1.79 4 4v14c0 4.41 3.59 8 8 8s8-3.59 8-8V16h6l-8-8z'/>";

/**
 * @brief SVG path data for the "Game Asset" icon.
 */
static const char SVG_ICON48_PATH_GAME_ASSET[] PROG_RODT_ATTR = "\
<path d='M42 12H6c-2.21 0-4 1.79-4 4v16c0 2.21 1.79 4 4 4h36c2.21 0 4-1.79 4-4V16c0-2.21-1.79-4-4-4zM22 26h-6v6h-4v-6H6v-4h6v-6h4v6h6v4zm9 4c-1.66 0-3-1.34-3-3s1.34-3 3-3 3 1.34 3 3-1.34 3-3 3zm8-6c-1.66 0-3-1.34-3-3s1.34-3 3-3 3 1.34 3 3-1.34 3-3 3z'/>";

/**
 * @brief SVG path data for the "Computer" icon.
 */
static const char SVG_ICON48_PATH_COMPUTER[] PROG_RODT_ATTR = "\
<path d='M40 36c2.21 0 3.98-1.79 3.98-4L44 12c0-2.21-1.79-4-4-4H8c-2.21 0-4 1.79-4 4v20c0 2.21 1.79 4 4 4H0v4h48v-4h-8zM8 12h32v20H8V12z'/>";

/**
 * @brief SVG path data for the "Eye" icon.
 */
static const char SVG_ICON48_PATH_EYE[] PROG_RODT_ATTR = "\
<path d='M24 9C14 9 5.46 15.22 2 24c3.46 8.78 12 15 22 15s18.54-6.22 22-15C42.54 15.22 34.01 9 24 9zm0 25c-5.52 0-10-4.48-10-10s4.48-10 10-10 10 4.48 10 10-4.48 10-10 10zm0-16c-3.31 0-6 2.69-6 6s2.69 6 6 6 6-2.69 6-6-2.69-6-6-6z'/>";

/**
 * @brief SVG path data for the "Tune" icon.
 */
static const char SVG_ICON48_PATH_TUNE[] PROG_RODT_ATTR = "\
<path d='M6 34v4h12v-4H6zm0-24v4h20v-4H6zm20 32v-4h16v-4H26v-4h-4v12h4zM14 18v4H6v4h8v4h4V18h-4zm28 8v-4H22v4h20zm-12-8h4v-4h8v-4h-8V6h-4v12z'/>";

/**
 * @brief SVG path data for the "Import Export" icon.
 */
static const char SVG_ICON48_PATH_IMPORT_EXPORT[] PROG_RODT_ATTR = "\
<path d='M18 6l-8 7.98h6V28h4V13.98h6L18 6zm14 28.02V20h-4v14.02h-6L30 42l8-7.98h-6z'/>";

/**
 * @brief SVG path data for the "Beenhere" icon.
 */
static const char SVG_ICON48_PATH_BEENHERE[] PROG_RODT_ATTR = "\
<path d='M38 2H10C7.79 2 6.02 3.79 6.02 6L6 31.87c0 1.38.7 2.6 1.76 3.32L23.99 46l16.23-10.81c1.06-.72 1.76-1.94 1.76-3.32L42 6c0-2.21-1.79-4-4-4zM20 32L10 22l2.83-2.83L20 26.34l15.17-15.17L38 14 20 32z'/>";

/**
 * @brief SVG path data for the "Settings" icon.
 */
static const char SVG_ICON48_PATH_SETTINGS[] PROG_RODT_ATTR = "\
<path d='M38.86 25.95c.08-.64.14-1.29.14-1.95s-.06-1.31-.14-1.95l4.23-3.31c.38-.3.49-.84.24-1.28l-4-6.93c-.25-.43-.77-.61-1.22-.43l-4.98 2.01c-1.03-.79-2.16-1.46-3.38-1.97L29 4.84c-.09-.47-.5-.84-1-.84h-8c-.5 0-.91.37-.99.84l-.75 5.3c-1.22.51-2.35 1.17-3.38 1.97L9.9 10.1c-.45-.17-.97 0-1.22.43l-4 6.93c-.25.43-.14.97.24 1.28l4.22 3.31C9.06 22.69 9 23.34 9 24s.06 1.31.14 1.95l-4.22 3.31c-.38.3-.49.84-.24 1.28l4 6.93c.25.43.77.61 1.22.43l4.98-2.01c1.03.79 2.16 1.46 3.38 1.97l.75 5.3c.08.47.49.84.99.84h8c.5 0 .91-.37.99-.84l.75-5.3c1.22-.51 2.35-1.17 3.38-1.97l4.98 2.01c.45.17.97 0 1.22-.43l4-6.93c.25-.43.14-.97-.24-1.28l-4.22-3.31zM24 31c-3.87 0-7-3.13-7-7s3.13-7 7-7 7 3.13 7 7-3.13 7-7 7z'/>";

/**
 * @brief SVG path data for the "Notification" icon.
 */
static const char SVG_ICON48_PATH_NOTIFICATION[] PROG_RODT_ATTR = "\
<path d='M24 44c2.21 0 4-1.79 4-4h-8c0 2.21 1.79 4 4 4zm12-12V22c0-6.15-3.27-11.28-9-12.64V8c0-1.66-1.34-3-3-3s-3 1.34-3 3v1.36c-5.73 1.36-9 6.49-9 12.64v10l-4 4v2h32v-2l-4-4z'/>";

/**
 * @brief SVG path data for the "File" icon.
 */
static const char SVG_ICON48_1216_PATH_FILE[] PROG_RODT_ATTR = "\
<path d='M6 5H2V4h4v1zM2 8h7V7H2v1zm0 2h7V9H2v1zm0 2h7v-1H2v1zm10-7.5V14c0 .55-.45 1-1 1H1c-.55 0-1-.45-1-1V2c0-.55.45-1 1-1h7.5L12 4.5zM11 5L8 2H1v12h10V5z'/>";

/**
 * @brief SVG path data for the "Folder" icon.
 */
static const char SVG_ICON48_1416_PATH_FOLDER[] PROG_RODT_ATTR = "\
<path d='M13 4H7V3c0-.66-.31-1-1-1H1c-.55 0-1 .45-1 1v10c0 .55.45 1 1 1h12c.55 0 1-.45 1-1V5c0-.55-.45-1-1-1zM6 4H1V3h5v1z'/>";

/**
 * @brief SVG path data for the "trash" icon.
 */
static const char SVG_ICON48_1616_PATH_TRASH[] PROG_RODT_ATTR = "\
<path d='M14.5 3a1 1 0 0 1-1 1H13v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V4h-.5a1 1 0 0 1-1-1V2a1 1 0 0 1 1-1H6a1 1 0 0 1 1-1h2a1 1 0 0 1 1 1h3.5a1 1 0 0 1 1 1zM4.118 4 4 4.059V13a1 1 0 0 0 1 1h6a1 1 0 0 0 1-1V4.059L11.882 4zM2.5 3h11V2h-11z'/>";

#endif
