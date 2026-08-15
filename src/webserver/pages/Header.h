/***************************** Header HTML Page *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `Header.h` file defines the HTML content for the header section of the web
server pages. This header includes the document preamble, the stylesheet shared
by every page, the theme switch, and the opening of the content container. It is
split into program-memory parts so no part exceeds the page buffer, and they are
emitted through `concat_header_html()` which flushes a chunk between them.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_HEADER_HTML_H_
#define _WEB_SERVER_HEADER_HTML_H_

#include <interface/pdi.h>

/**
 * @brief Document preamble and theme tokens.
 *
 * Colours are declared once as tokens. The scheme follows the operating system
 * until the visitor picks one, after which `data-theme` on the root element
 * wins in both directions. The inline script applies a stored choice before the
 * body renders so the page never flashes the wrong scheme.
 */
static const char WEB_SERVER_HEADER_HTML[] PROG_RODT_ATTR = "\
<!DOCTYPE html>\
<html lang='en'>\
<head>\
<meta charset='utf-8'>\
<meta name='viewport' content='width=device-width,initial-scale=1'>\
<title>Device Manager</title>\
<script>\
function tgTh(){\
var d=document.documentElement,c=d.getAttribute('data-theme');\
if(!c)c=matchMedia('(prefers-color-scheme:dark)').matches?'dark':'light';\
var n=c=='dark'?'light':'dark';\
d.setAttribute('data-theme',n);\
try{localStorage.setItem('th',n);}catch(e){}\
}\
try{\
var t=localStorage.getItem('th');\
if(t)document.documentElement.setAttribute('data-theme',t);\
}catch(e){}\
</script>\
<style>\
:root{\
--bg:#f4f6f8;\
--fg:#1c1e21;\
--cd:#ffffff;\
--ln:#dfe3e8;\
--pr:#0062af;\
--bt:#337ab7;\
--mt:#6b7280;\
--rd:8px;\
}\
@media(prefers-color-scheme:dark){\
:root:not([data-theme='light']){\
--bg:#16181c;\
--fg:#e6e8eb;\
--cd:#1f2226;\
--ln:#343a40;\
--pr:#4da3e8;\
--bt:#3d8fd0;\
--mt:#9aa3ad;\
}\
}\
:root[data-theme='dark']{\
--bg:#16181c;\
--fg:#e6e8eb;\
--cd:#1f2226;\
--ln:#343a40;\
--pr:#4da3e8;\
--bt:#3d8fd0;\
--mt:#9aa3ad;\
}\
</style>";

/**
 * @brief Base layout rules.
 */
static const char WEB_SERVER_HEADER_HTML_LAYOUT[] PROG_RODT_ATTR = "\
<style>\
*{box-sizing:border-box;}\
body{\
margin:0;\
padding:16px 12px;\
background:var(--bg);\
color:var(--fg);\
font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Helvetica,Arial,sans-serif;\
font-size:15px;\
line-height:1.5;\
}\
#cntnr{\
position:relative;\
background:var(--cd);\
border:1px solid var(--ln);\
border-radius:12px;\
max-width:380px;\
margin:auto;\
text-align:center;\
padding:18px 16px;\
}\
h1{color:var(--pr);font-size:22px;margin:0 0 14px;}\
h2{font-size:17px;margin:0 0 12px;}\
h4{font-size:14px;color:var(--mt);font-weight:500;}\
a{text-decoration:none;color:inherit;}\
#thm{\
position:absolute;\
top:12px;\
right:12px;\
width:38px;\
height:38px;\
padding:0;\
display:flex;\
align-items:center;\
justify-content:center;\
border:1px solid var(--ln);\
border-radius:50%;\
background:none;\
color:var(--mt);\
cursor:pointer;\
}\
#thm:hover{color:var(--bt);border-color:var(--bt);}\
#thm svg{display:block;margin:0;}\
</style>";

/**
 * @brief Control, table and menu rules, then the opening of the document body.
 *
 * Inputs and buttons are sized for a touch target and use a 16px font so a
 * phone browser does not zoom when a field takes focus.
 */
static const char WEB_SERVER_HEADER_HTML_CONTROLS[] PROG_RODT_ATTR = "\
<style>\
input,select,button{font:inherit;color:inherit;}\
input:not([type=checkbox]):not([type=radio]),select{\
width:100%;\
padding:5px 9px;\
border:1px solid var(--ln);\
border-radius:var(--rd);\
background:var(--cd);\
font-size:16px;\
line-height:1.35;\
}\
input:focus-visible,select:focus-visible,button:focus-visible{\
outline:2px solid var(--bt);\
outline-offset:1px;\
}\
table{margin:auto;width:100%;border-collapse:collapse;}\
td{padding:5px 4px;text-align:left;}\
svg{display:block;margin:auto;}\
.btn{\
padding:7px 16px;\
border-radius:var(--rd);\
border:1px solid transparent;\
color:#fff;\
background:var(--bt);\
margin:3px 2px;\
font-size:14px;\
cursor:pointer;\
}\
.btn:hover{filter:brightness(1.08);}\
.btn:active{transform:translateY(1px);}\
.btnd{\
padding:6px;\
border-radius:var(--rd);\
border:1px solid var(--ln);\
background:none;\
color:var(--fg);\
text-align:center;\
}\
.msg{padding:9px;border-radius:var(--rd);margin:10px 0;font-size:14px;}\
.mnwdth125{min-width:125px;}\
</style>";

/**
 * @brief Menu grid, progress indicator and the responsive breakpoint.
 */
static const char WEB_SERVER_HEADER_HTML_MENU[] PROG_RODT_ATTR = "\
<style>\
#mncntr{display:grid;grid-template-columns:repeat(2,1fr);gap:8px;margin-top:6px;}\
#mncntr>div{\
display:flex;\
align-items:center;\
justify-content:center;\
min-height:54px;\
padding:8px;\
border:1px solid var(--ln);\
border-radius:var(--rd);\
font-size:13px;\
text-transform:capitalize;\
}\
#mncntr>div:hover{border-color:var(--bt);}\
#mncntr a{display:flex;flex-direction:column;align-items:center;gap:5px;width:100%;}\
#mncntr svg{fill:var(--bt);}\
.ldr{\
border:3px solid var(--ln);\
border-radius:50%;\
border-top:3px solid var(--bt);\
width:28px;\
height:28px;\
animation:spin 1s linear infinite;\
margin:10px auto;\
}\
@keyframes spin{to{transform:rotate(360deg);}}\
@media(min-width:640px){\
#cntnr.wide{max-width:760px;}\
#mncntr{grid-template-columns:repeat(3,1fr);}\
}\
@media(pointer:coarse){\
input:not([type=checkbox]):not([type=radio]),select{padding:9px;}\
.btn{padding:11px 18px;}\
}\
</style>\
</head>\
<body>";

/**
 * @brief Opening of the content container.
 *
 * Emitted separately so a page that shows a table or a dashboard can widen the
 * container on a larger screen while forms stay in the narrow column.
 */
static const char WEB_SERVER_HEADER_CNTNR_OPEN[] PROG_RODT_ATTR = "<div id='cntnr'";

/**
 * @brief Modifier that widens the container above the mobile breakpoint.
 */
static const char WEB_SERVER_HEADER_CNTNR_WIDE[] PROG_RODT_ATTR = " class='wide'";

/**
 * @brief Theme switch and the portal title, which links back to the menu.
 */
static const char WEB_SERVER_HEADER_TITLE[] PROG_RODT_ATTR = "\
>\
<button id='thm' type='button' onclick='tgTh()' aria-label='Switch theme' title='Switch theme'>\
<svg width='17' height='17' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'>\
<circle cx='12' cy='12' r='9'/>\
<path d='M12 3v18a9 9 0 000-18z' fill='currentColor'/>\
</svg>\
</button>\
<a href='/'><h1>Device Manager</h1></a>";

#endif
