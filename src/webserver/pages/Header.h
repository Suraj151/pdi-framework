/***************************** header html page ********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _EW_SERVER_HEADER_HTML_H_
#define _EW_SERVER_HEADER_HTML_H_

#include <Arduino.h>

static const char EW_SERVER_HEADER_HTML[] PROGMEM = "\
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
