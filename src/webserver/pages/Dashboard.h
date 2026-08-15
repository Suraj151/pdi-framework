/************************** Dashboard HTML Page *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `Dashboard.h` file defines the HTML content for the dashboard page of the
web server. The page is a grid of cards covering link state, storage, uptime and
tasks, active sessions, live gpio and connected stations. Each part is a
separate program memory string so no part exceeds the page buffer, and the
controller flushes a chunk between them.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_DASHBOARD_PAGE_H_
#define _WEB_SERVER_DASHBOARD_PAGE_H_

#include <interface/pdi.h>

/**
 * @brief Dashboard layout rules.
 *
 * The card grid is a single column on a phone and two columns once the
 * container widens, so a card never has to scroll sideways.
 */
static const char WEB_SERVER_DASHBOARD_STYLE[] PROG_RODT_ATTR = "\
<style>\
#dhd{display:flex;align-items:center;justify-content:space-between;gap:8px;margin:0 44px 12px 0;}\
#dhd h2{margin:0;}\
#dst{display:flex;align-items:center;gap:6px;}\
.chp{display:flex;align-items:center;gap:4px;padding:3px 8px;border:1px solid var(--ln);border-radius:20px;font-size:12px;color:var(--mt);}\
.chp svg{display:block;margin:0;}\
.dim{opacity:.22;}\
#grd{display:grid;grid-template-columns:1fr;gap:10px;text-align:left;}\
.crd{border:1px solid var(--ln);border-radius:var(--rd);padding:10px 12px;min-width:0;}\
.crd h4{margin:0 0 8px;font-size:11px;font-weight:600;text-transform:uppercase;letter-spacing:.4px;color:var(--mt);}\
.kv{display:flex;justify-content:space-between;gap:10px;font-size:13px;padding:2px 0;}\
.kv b{font-weight:500;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;}\
.kv span{color:var(--mt);flex:none;}\
.big{font-size:19px;font-weight:600;}\
.gauge{display:flex;align-items:center;gap:14px;}\
.gauge svg{flex:none;margin:0;}\
.dot{width:8px;height:8px;border-radius:50%;background:var(--ln);flex:none;}\
.dot.on{background:#2ea043;}\
@media(min-width:640px){#grd{grid-template-columns:repeat(2,1fr);}.wd{grid-column:1/-1;}}\
</style>";

/**
 * @brief Page heading with the link state chips.
 *
 * The chips carry the whole radio state as symbols. The wifi arcs are dimmed
 * from the bottom up by signal strength, and the globe dims when the internet
 * probe last failed.
 */
static const char WEB_SERVER_DASHBOARD_HEAD[] PROG_RODT_ATTR = "\
<div id='dhd'>\
<h2>Dashboard</h2>\
<div id='dst'>\
<span class='chp' id='wch' title='Wifi signal'>\
<svg width='16' height='16' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round'>\
<path id='wa3' d='M2 8.8a15 15 0 0120 0'/>\
<path id='wa2' d='M5.3 12.4a10 10 0 0113.4 0'/>\
<path id='wa1' d='M8.6 16a5 5 0 016.8 0'/>\
<circle cx='12' cy='19.5' r='1.2' fill='currentColor' stroke='none'/>\
</svg>\
<b id='rsv'>-</b>\
</span>\
<span class='chp' id='ich' title='Internet'>\
<svg width='15' height='15' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'>\
<circle cx='12' cy='12' r='9'/>\
<path d='M3 12h18'/>\
<path d='M12 3a15 15 0 010 18a15 15 0 010-18'/>\
</svg>\
</span>\
</div>\
</div>\
<div id='grd'>";

/**
 * @brief Storage and heap gauges.
 *
 * The ring radius makes its circumference exactly 100 units, so the dash length
 * is the used percentage and no arc arithmetic is needed on the client.
 */
static const char WEB_SERVER_DASHBOARD_STORAGE[] PROG_RODT_ATTR = "\
<div class='crd'>\
<h4>Storage</h4>\
<div class='gauge'>\
<svg width='68' height='68' viewBox='0 0 36 36'>\
<circle cx='18' cy='18' r='15.9' fill='none' stroke='var(--ln)' stroke-width='3.2'/>\
<circle id='fsr' cx='18' cy='18' r='15.9' fill='none' stroke='var(--bt)' stroke-width='3.2' stroke-dasharray='0 100' stroke-linecap='round' transform='rotate(-90 18 18)'/>\
<text id='fsp' x='18' y='20' text-anchor='middle' font-size='8' fill='currentColor'>-</text>\
</svg>\
<div style='flex:1;min-width:0;'>\
<div class='kv'><span>Used</span><b id='fsu'>-</b></div>\
<div class='kv'><span>Free</span><b id='fsf'>-</b></div>\
<div class='kv'><span>Total</span><b id='fst'>-</b></div>\
</div>\
</div>\
</div>\
<div class='crd'>\
<h4>Memory</h4>\
<div class='gauge'>\
<svg width='68' height='68' viewBox='0 0 36 36'>\
<circle cx='18' cy='18' r='15.9' fill='none' stroke='var(--ln)' stroke-width='3.2'/>\
<circle id='hpr' cx='18' cy='18' r='15.9' fill='none' stroke='var(--bt)' stroke-width='3.2' stroke-dasharray='0 100' stroke-linecap='round' transform='rotate(-90 18 18)'/>\
<text id='hpp' x='18' y='20' text-anchor='middle' font-size='8' fill='currentColor'>-</text>\
</svg>\
<div style='flex:1;min-width:0;'>\
<div class='kv'><span>Free heap</span><b id='hpf'>-</b></div>\
<div class='kv'><span>Largest block</span><b id='hpb'>-</b></div>\
<div class='kv'><span>Tasks</span><b id='tkc'>-</b></div>\
</div>\
</div>\
</div>";

/**
 * @brief Uptime with the busiest tasks, in the column layout of the ps view.
 */
static const char WEB_SERVER_DASHBOARD_TASKS[] PROG_RODT_ATTR = "\
<div class='crd'>\
<h4>Uptime</h4>\
<div class='big' id='upt'>-</div>\
<div class='kv' style='margin-top:6px;'><span>Network time</span><b id='nwt'>-</b></div>\
<table style='margin-top:8px;font-size:12px;'>\
<thead style='color:var(--mt);'>\
<tr><td>PID</td><td>NAME</td><td>ST</td><td style='text-align:right;'>%CPU</td></tr>\
</thead>\
<tbody id='pst'></tbody>\
</table>\
</div>";

/**
 * @brief Active sessions across every terminal the device serves.
 */
static const char WEB_SERVER_DASHBOARD_SESSIONS[] PROG_RODT_ATTR = "\
<div class='crd'>\
<h4>Active Sessions</h4>\
<table style='font-size:12px;'>\
<thead style='color:var(--mt);'>\
<tr><td>USER</td><td>VIA</td><td style='text-align:right;'>LOGIN</td><td style='text-align:right;'>IDLE</td></tr>\
</thead>\
<tbody id='ses'></tbody>\
</table>\
</div>";

/**
 * @brief Network detail and the stations attached to the soft ap.
 */
static const char WEB_SERVER_DASHBOARD_NETWORK[] PROG_RODT_ATTR = "\
<div class='crd'>\
<h4>Network</h4>\
<div class='kv'><span>SSID</span><b id='stnm'>-</b></div>\
<div class='kv'><span>IP</span><b id='stip'>-</b></div>\
<div class='kv'><span>MAC</span><b id='stmc'>-</b></div>\
<div class='kv'><span>Status</span><b id='stst'>-</b></div>\
<table id='cndl' style='margin-top:6px;font-size:12px;'></table>\
</div>";

/**
 * @brief Live gpio card.
 *
 * Digital pins render as state dots and the analog pin feeds a rolling trace
 * kept on the client, so the device only ever sends the current readings.
 */
static const char WEB_SERVER_DASHBOARD_GPIO[] PROG_RODT_ATTR = "\
<style>\
.pin{border:1px solid var(--ln);border-radius:var(--rd);padding:6px 10px;}\
.pinh{display:flex;align-items:center;gap:6px;font-size:13px;font-weight:600;}\
.pinm{font-size:11px;color:var(--mt);margin-top:1px;}\
#gpa{font-size:12px;color:var(--mt);margin-top:10px;}\
#gpa b{color:var(--fg);font-size:15px;margin-left:8px;}\
#gpg .ax{stroke:var(--ln);}\
#gpg .gl{stroke:var(--ln);stroke-dasharray:3 3;}\
.gyl,.gxl{font-size:10px;color:var(--mt);}\
.gyl{display:flex;flex-direction:column;justify-content:space-between;width:30px;text-align:right;}\
.gxl{display:flex;justify-content:space-between;}\
</style>\
<div class='crd wd' id='gpc' style='display:none;'>\
<h4>GPIO</h4>\
<div id='gpd' style='display:flex;flex-wrap:wrap;gap:8px;'></div>\
<div id='gpa' style='display:none;'>\
<span id='gpn'>-</span><b id='gpv'>-</b>\
</div>\
<div id='gpw' style='display:none;margin-top:4px;'>\
<div style='display:flex;gap:6px;'>\
<div class='gyl'><span id='gy2'>-</span><span id='gy1'>-</span><span>0</span></div>\
<svg id='gpg' width='100%' height='78' viewBox='0 0 300 78' preserveAspectRatio='none' style='flex:1;min-width:0;'>\
<line class='gl' x1='0' y1='1' x2='300' y2='1'/>\
<line class='gl' x1='0' y1='39' x2='300' y2='39'/>\
<line class='ax' x1='0' y1='77' x2='300' y2='77'/>\
<polyline id='gpl' fill='none' stroke='var(--bt)' stroke-width='1.5' stroke-linejoin='round' vector-effect='non-scaling-stroke' points=''/>\
</svg>\
</div>\
<div class='gxl' style='margin-left:36px;'><span id='gx0'>-</span><span>now</span></div>\
</div>\
</div>\
</div>";

/**
 * @brief Formatting helpers shared by the card renderers.
 */
static const char WEB_SERVER_DASHBOARD_SCRIPT1[] PROG_RODT_ATTR = "\
<script>\
var rq=new XMLHttpRequest(),gh=[],GN=60,GI=3,\
MD=['OFF','DOUT','DIN','BLINK','AOUT','AIN'];\
function bs(b){\
b=+b;\
if(b<1024)return b+' B';\
if(b<1048576)return (b/1024).toFixed(1)+' K';\
return (b/1048576).toFixed(2)+' M';\
}\
function du(s){\
s=Math.floor(s);\
var d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);\
return (d?d+'d ':'')+(d||h?h+'h ':'')+m+'m '+(s%60)+'s';\
}\
function gi(i){return document.getElementById(i);}\
function tx(i,v){var e=gi(i);if(e)e.textContent=v;}\
function dm(i,c){var e=gi(i);if(e)e.classList.toggle('dim',!c);}\
function rg(r,t,p){\
var e=gi(r);\
if(e)e.setAttribute('stroke-dasharray',p+' 100');\
tx(t,p+'%');\
}\
function rw(b,rows,al){\
var t=gi(b);\
if(!t)return;\
t.innerHTML='';\
for(var i=0;i<rows.length;i++){\
var tr=t.insertRow();\
for(var j=0;j<rows[i].length;j++){\
var c=tr.insertCell();\
c.textContent=rows[i][j];\
if(al.indexOf(j)>=0)c.style.textAlign='right';\
}\
}\
}\
</script>";

/**
 * @brief Renders one poll response into the cards.
 */
static const char WEB_SERVER_DASHBOARD_SCRIPT2[] PROG_RODT_ATTR = "\
<script>\
function rnw(w,nwt){\
tx('rsv',w.c?w.rs:'--');\
dm('wa1',w.c);\
dm('wa2',w.c&&w.rs>-75);\
dm('wa3',w.c&&w.rs>-65);\
dm('ich',w.nt);\
tx('stnm',w.nm||'-');\
tx('stip',w.ip||'-');\
tx('stmc',w.mc||'-');\
tx('stst',w.c?'connected':'disconnected');\
tx('nwt',nwt?new Date(nwt*1000).toLocaleString():'-');\
}\
function rfs(f){\
if(!f)return;\
rg('fsr','fsp',f.t?Math.round(f.u*100/f.t):0);\
tx('fsu',bs(f.u));\
tx('fsf',bs(f.t-f.u));\
tx('fst',bs(f.t));\
}\
function rmm(r){\
rg('hpr','hpp',r.hp?Math.round(r.hb*100/r.hp):0);\
tx('hpf',bs(r.hp));\
tx('hpb',bs(r.hb));\
tx('tkc',r.tc);\
}\
function rgp(g,mx){\
var c=gi('gpc');\
if(!g||!g.length){c.style.display='none';return;}\
c.style.display='';\
var d=gi('gpd'),h='',a=null,an='',am=0;\
for(var i=0;i<g.length;i++){\
var p=g[i],md=MD[p[1]]||'?';\
if(p[3]){a=p[2];an=p[0];am=p[1];continue;}\
var st=(p[1]>=4)?p[2]:(p[2]?'HIGH':'LOW');\
h+=\"<div class='pin'><div class='pinh'><i class='dot\"+(p[2]?' on':'')+\"'></i>\"+p[0]+\
\"</div><div class='pinm'>\"+md+' &middot; '+st+'</div></div>';\
}\
d.innerHTML=h;\
if(a===null){gi('gpa').style.display='none';gi('gpw').style.display='none';return;}\
mx=mx||1023;\
gi('gpa').style.display='';\
gi('gpw').style.display='';\
gi('gpn').innerHTML=an+' &middot; '+(MD[am]||'?');\
tx('gpv',a);\
tx('gy2',mx);\
tx('gy1',Math.round(mx/2));\
tx('gx0','-'+((GN-1)*GI)+'s');\
gh.push(a);\
if(gh.length>GN)gh.shift();\
var s=300/(GN-1),q='';\
for(var i=0;i<gh.length;i++){\
q+=(300-(gh.length-1-i)*s).toFixed(1)+','+(77-gh[i]*76/mx).toFixed(1)+' ';\
}\
gi('gpl').setAttribute('points',q);\
}\
</script>";

/**
 * @brief Poll loop. A response other than 200 means the session went away.
 */
static const char WEB_SERVER_DASHBOARD_SCRIPT3[] PROG_RODT_ATTR = "\
<script>\
function rql(){\
if(this.status!=200){location.href='/login';return;}\
var r=JSON.parse(this.responseText);\
rnw(r.w,r.nwt);\
rfs(r.fs);\
rmm(r);\
tx('upt',du(r.up));\
rw('pst',r.ps,[3]);\
var s=[];\
for(var i=0;i<r.se.length;i++){\
s.push([r.se[i][0],r.se[i][1],du(r.se[i][2]),du(r.se[i][3])]);\
}\
rw('ses',s,[2,3]);\
rgp(r.gp,r.gm);\
var t=gi('cndl');\
t.innerHTML='';\
for(var i=0;i<r.dv.length;i++){\
var tr=t.insertRow();\
tr.insertCell().textContent=r.dv[i][0];\
tr.insertCell().textContent=r.dv[i][1];\
}\
}\
rq.addEventListener('load',rql);\
function pol(){\
rq.open('GET','/listen-dashboard');\
rq.send();\
}\
pol();\
setInterval(pol,GI*1000);\
</script>";

#endif
