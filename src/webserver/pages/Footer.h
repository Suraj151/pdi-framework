/**************************** Footer HTML Page ********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `Footer.h` file defines various HTML footer templates for the web server.
These templates include JavaScript code for dynamic updates and monitoring
features, such as analog monitoring, dashboard updates, client listening, and
OTP requests for IoT devices. The HTML content is stored in program memory
(PROG_RODT_ATTR) to optimize memory usage on embedded systems.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_FOOTER_HTML_H_
#define _WEB_SERVER_FOOTER_HTML_H_

#include <interface/pdi.h>

/**
 * @brief Basic footer HTML template.
 *
 * This template includes a script to hide messages after a timeout.
 */
static const char WEB_SERVER_FOOTER_HTML[] PROG_RODT_ATTR = "\
</div>\
<script>\
setTimeout(function(){\
let msgelm=document.getElementsByClassName('msg');\
if(msgelm[0])msgelm[0].style.display='none';\
let nfl=document.querySelector('#nfl');\
if(nfl)nfl.value=(location.pathname+location.search);\
let ndl=document.querySelector('#ndl');\
if(ndl)ndl.value=(location.pathname+location.search);\
},3000);\
</script>\
</body>\
</html>";

/**
 * @brief Footer HTML template with analog monitoring.
 *
 * This template includes JavaScript code to dynamically update analog monitoring
 * data and render it on the page using SVG elements.
 */
static const char WEB_SERVER_FOOTER_WITH_ANALOG_MONITOR_HTML[] PROG_RODT_ATTR = "\
</div>\
<script>\
var rq=new XMLHttpRequest();\
function rql(){\
var r=JSON.parse(this.responseText);\
var sv=document.getElementById('svga0');\
var ln=document.createElementNS('http://www.w3.org/2000/svg','line');\
if(r.d.data){\
var ks=Object.keys(r.d.data);\
for(var i=0;i<ks.length;i++){\
document.getElementsByTagName('table')[0].rows[i+1].innerHTML=\
`<td class='btnd'>${ks[i]}</td><td class='btnd'>${r.md[r.d.data[ks[i]].mode]}</td><td class='btnd'>${r.d.data[ks[i]].val}</td>`;\
}\
}\
if(r.r)location.href='/';\
ln.setAttribute('x1',r.x1);\
ln.setAttribute('y1',r.y1);\
ln.setAttribute('x2',r.x2);\
ln.setAttribute('y2',r.y2);\
ln.setAttribute('stroke','red');\
var nc=sv.childElementCount;\
if(nc>32){\
for(var i=3;i<nc;i++){\
var cn=sv.childNodes[i];\
cn.setAttribute('x1',cn.getAttribute('x1')-10);\
cn.setAttribute('x2',cn.getAttribute('x2')-10);\
}\
sv.removeChild(sv.childNodes[3]);\
}\
sv.appendChild(ln)\
}\
rq.addEventListener('load',rql);\
setInterval(function(){\
rq.open('GET','/listen-monitor');\
rq.send();\
},1000);\
</script>\
</body>\
</html>\
";

/**
 * @brief Footer HTML template with dashboard monitoring.
 *
 * This template includes JavaScript code to dynamically update dashboard data,
 * such as network status, IP address, RSSI, and connected devices.
 */
static const char WEB_SERVER_FOOTER_WITH_DASHBOARD_MONITOR_HTML[] PROG_RODT_ATTR = "\
</div>\
<script>\
var rq=new XMLHttpRequest();\
function rql(){\
var r=JSON.parse(this.responseText);\
document.getElementById('stnm').innerHTML=r.nm;\
document.getElementById('stip').innerHTML=r.ip;\
document.getElementById('strs').innerHTML=r.rs;\
document.getElementById('stst').innerHTML=r.st;\
document.getElementById('stmc').innerHTML=r.mc;\
document.getElementById('inet').innerHTML=r.nt;\
document.getElementById('nwt').innerHTML=r.nwt;\
document.getElementById('cndl').innerHTML=r.dl;\
if(r.r)location.href='/';\
}\
rq.addEventListener('load',rql);\
setInterval(function(){\
rq.open('GET','/listen-dashboard');\
rq.send();\
},3000);\
</script>\
</body>\
</html>\
";

/**
 * @brief Footer HTML template with client listening.
 *
 * This template includes JavaScript code to dynamically update client-related
 * data by polling the server at regular intervals.
 */
static const char WEB_SERVER_FOOTER_WITH_CLIENT_LISTEN_HTML[] PROG_RODT_ATTR = "\
</div>\
<script>\
setTimeout(function(){\
document.getElementsByClassName('msg')[0].style.display='none';\
},4000);\
var rq=new XMLHttpRequest();\
function rql(){\
var r=JSON.parse(this.responseText);\
if(r.rpc){\
document.getElementsByClassName('cntnr')[0].innerHTML=r.bdy;\
}\
}\
rq.addEventListener('load',rql);\
setInterval(function(){\
rq.open('GET','/listen-client');\
rq.send();\
},15000);\
</script>\
</body>\
</html>";

#ifdef ENABLE_DEVICE_IOT

/**
 * @brief Footer HTML template with OTP monitoring for IoT devices.
 *
 * This template includes JavaScript code to handle OTP requests and display
 * the OTP or related status messages dynamically.
 */
static const char WEB_SERVER_FOOTER_WITH_OTP_MONITOR_HTML[] PROG_RODT_ATTR = "\
<tr>\
<td></td>\
<td>\
<button id='otpbtn' class='btn' type='button' onclick='gotp()'>\
Request OTP\
</button>\
</td>\
</tr>\
</table>\
</form>\
</div>\
<script>\
var isldr=0,\
rq=new XMLHttpRequest(),\
el=document.getElementById('cntnr'),\
eldr=document.createElement('div');\
function rql(){\
var r=JSON.parse(this.responseText);\
console.log('r',r);\
if(r){\
eldr.setAttribute('class','msg');\
eldr.style.background=r.status?'#a6eaa8':'#f9dc87';\
eldr.innerHTML=r.status?'OTP is <strong>'+r.otp+'</strong> : '+r.remark:r.remark;\
isldr=0;\
}\
}\
function gotp(){\
if(!isldr){\
eldr.setAttribute('class','ldr');\
eldr.innerHTML='';\
eldr.style.background='';\
el.appendChild(eldr);\
}\
rq.addEventListener('load',rql);\
rq.open('POST','/device-register-config');\
var fd=new FormData(document.getElementById('drcf'));\
rq.send(fd);\
isldr=1;\
}\
</script>\
</body>\
</html>";

#endif

#endif
