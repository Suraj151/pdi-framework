/***************************** Header HTML Page *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `Header.h` file defines the HTML content for the header section of the web
server pages. This header includes the basic structure of an HTML document,
such as the `<html>` and `<head>` tags, along with styling for the web interface.
It also includes a title and a container for the main content of the page. The
HTML content is stored in program memory (PROGMEM) to optimize memory usage on
embedded systems.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_HEADER_HTML_H_
#define _WEB_SERVER_HEADER_HTML_H_

#include <Arduino.h>

/**
 * @brief HTML content for the header section of the web server pages.
 *
 * This static HTML content defines the structure and styling for the header
 * section of the web server pages. It includes:
 * - A `<title>` tag for the page title ("Device Manager").
 * - A `<meta>` tag for responsive design.
 * - A `<style>` block for CSS styling, including styles for buttons, inputs,
 *   containers, and other UI elements.
 * - A `<div>` container for the main content of the page.
 */
static const char WEB_SERVER_HEADER_HTML[] PROGMEM = "\
<html>\
<head>\
<title>Device Manager</title>\
<meta name='viewport' content='width=device-width, initial-scale=1.0, user-scalable=yes'>\
<style>\
#cntnr{\
border:1px solid #d0d0d0;\
border-radius:10px;\
max-width:350px;\
margin:auto;\
text-align:center;\
padding:15px;\
}\
a{\
text-decoration:none;\
}\
input, select{\
padding:4px;\
border:1px solid #ccc;\
border-radius:4px;\
background:white;\
}\
select{\
width:100%;\
min-width:40px;\
padding-left:0px !important;\
}\
.msg{\
padding:3px;\
border-radius:4px;\
}\
h1{\
color:#0062af;\
}\
svg,table{\
margin:auto;\
}\
svg{\
display:block;\
}\
.mnwdth125{\
min-width:125px;\
}\
#mncntr a{\
margin:auto;\
color:#2f2f2f;\
}\
#mncntr>div{\
display:inline-flex;\
min-width:100px;\
margin:10px 3px;\
padding:10px 0px;\
border:1px solid #d0d0d0;\
border-radius:4px;\
font-size:12px;\
}\
.btn,.btnd{\
padding:6px;\
border-radius:5px;\
border:1px solid #ccc;\
color:white;\
background:#337ab7;\
margin:0px 2px;\
cursor:pointer;\
text-align:center;\
}\
.btnd{\
background:none;\
color:black;\
}\
.ldr{\
border:4px solid #fff;\
border-radius:50%;\
border-top:4px solid #2196F3;\
width:30px;\
height:30px;\
animation:spin 1s ease-in infinite;\
margin:10px auto;\
}\
@keyframes spin{\
0%{transform:rotate(0deg);}\
100%{transform:rotate(360deg);}\
}\
</style>\
</head>\
<body>\
<div id='cntnr'>\
<a href='/'><h1>Device Manager</h1></a>";

#endif
