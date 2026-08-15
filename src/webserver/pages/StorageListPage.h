/*************************** Storage List HTML Page ****************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_STORAGE_LIST_PAGE_H_
#define _WEB_SERVER_STORAGE_LIST_PAGE_H_

#include <interface/pdi.h>

/**
 * @brief HTML content for the Storage listing page.
 *
 * This static HTML content is used to render the top section of the storage
 * listing page on the web server. 
 */
static const char WEB_SERVER_STORAGE_LIST_PAGE_TOP[] PROG_RODT_ATTR = "\
<h2>Storage</h2>\
<style>\
#strg-tbl{width:100%;font-size:13px;}\
#strg-tbl td{padding:6px;border-bottom:1px solid var(--ln);white-space:nowrap;}\
#strg-tbl .hdr td{\
font-size:11px;\
font-weight:600;\
color:var(--mt);\
text-transform:uppercase;\
letter-spacing:.4px;\
}\
#strg-tbl .num{text-align:right;}\
#strg-tbl .pth td{border-bottom:0;color:var(--mt);font-size:12px;}\
#strg-tbl code{font-family:ui-monospace,Menlo,Consolas,monospace;font-size:12px;color:var(--mt);}\
#strg-tbl tr:hover td{background:rgba(127,127,127,.07);}\
#strg-tbl svg{fill:var(--bt);}\
.del{\
display:flex;\
margin:auto;\
padding:4px;\
border:0;\
background:none;\
color:var(--mt);\
cursor:pointer;\
}\
.del:hover{color:#d9534f;}\
</style>\
";

static const char WEB_SERVER_STORAGE_LIST_PAGE_BOTTOM_SCRIPT1[] PROG_RODT_ATTR = "\
<script>\
var rq=new XMLHttpRequest();\
function fsz(b){\
b=+b;\
if(b<1024)return b+' B';\
if(b<1048576)return (b/1024).toFixed(1)+' K';\
return (b/1048576).toFixed(1)+' M';\
}\
function rql(){\
if(this.status!=200){location.href='/login';return;}\
var r=JSON.parse(this.responseText);\
const tbl=document.getElementById('strg-tbl');\
tbl.deleteRow(tbl.rows.length-1);\
const hd=tbl.insertRow();\
hd.className='hdr';\
hd.innerHTML='<td></td><td>Name</td><td>Permissions</td><td>Owner</td><td>Group</td><td class=\"num\">Size</td><td></td>';\
for(let i=0;i<r.lst.length;i++){\
const it=r.lst[i];\
const ntr=tbl.insertRow();\
ntr.insertCell().innerHTML=it.t==\"D\"?r.dsvg:r.fsvg;\
ntr.insertCell().innerHTML=`<a href=\"${it.l}\">${it.n.substring(0,28)}</a>`;\
ntr.insertCell().innerHTML=`<code>${it.p||''}</code>`;\
ntr.insertCell().textContent=it.o||'';\
ntr.insertCell().textContent=it.g||'';\
const sc=ntr.insertCell();\
sc.className='num';\
sc.textContent=it.t==\"D\"?'-':fsz(it.s);\
ntr.insertCell().innerHTML=it.n==\"..\"?'':`<form method='POST' action='/storage-filedel' style='margin:0;'>\
<input type='hidden' name='df' value=\"${it.l.replace('/storage?cp=','')}\">\
<input type='hidden' name='loc' value=\"${location.pathname+location.search}\">\
<input type='hidden' name='csrf' value=\"${r.csrf}\">\
<button class='del' type='submit' title='Delete' aria-label=\"Delete ${it.n}\">\
${r.tsvg}\
</button>\
</form>`;\
}\
}\
rq.addEventListener('load',rql);\
setTimeout(function(){\
rq.open('GET','/storage-filelist'+location.search);\
rq.send();\
},2000);\
</script>\
";

static const char WEB_SERVER_STORAGE_LIST_PAGE_BOTTOM_FORMS1[] PROG_RODT_ATTR = "\
<style>\
form{\
text-align:left;\
margin:0px 0px 3px 15px;\
}\
input{max-width:185px;}\
.btn{min-width:100px;}\
</style>\
<form id='frm1' action='/storage-fileupload' enctype='multipart/form-data' method='POST'>\
";

static const char WEB_SERVER_STORAGE_LIST_PAGE_BOTTOM_FORMS2[] PROG_RODT_ATTR = "\
<input name='nf' type='file'>\
<input id='nfl' name='loc' type='text' style='display:none'>\
<button class='btn' type='submit'>\
Upload File\
</button>\
<progress id='pBr' value='0' max='100' style='display:none'></progress>\
</form>\
<form action='/storage-fileupload' enctype='multipart/form-data' method='POST'>\
";

static const char WEB_SERVER_STORAGE_LIST_PAGE_BOTTOM_FORMS3[] PROG_RODT_ATTR = "\
<input name='nd' type='text' placeholder='Enter folder name'>\
<input id='ndl' name='loc' type='text' style='display:none'>\
<button class='btn' type='submit'>\
Create Folder\
</button>\
</form>\
";

static const char WEB_SERVER_STORAGE_LIST_PAGE_BOTTOM_SCRIPT2[] PROG_RODT_ATTR = "\
<script>\
document.getElementById('frm1').addEventListener('submit',function(e1){\
e1.preventDefault();\
var frq=new XMLHttpRequest();\
var fDt=new FormData(document.getElementById('frm1'));\
frq.upload.addEventListener('progress',function(e2){\
if(e2.lengthComputable){\
var prc=(e2.loaded/e2.total)*100;\
let pelm=document.getElementById('pBr');\
pelm.value=prc;\
pelm.style.display='block';\
}\
});\
frq.open('POST','/storage-fileupload');\
frq.send(fDt);\
frq.onload=function(){\
if(frq.responseURL){\
location.href=frq.responseURL;\
}\
};\
});\
</script>\
";

#endif
