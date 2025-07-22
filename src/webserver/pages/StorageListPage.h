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
";

static const char WEB_SERVER_STORAGE_LIST_PAGE_BOTTOM[] PROG_RODT_ATTR = "\
<script>\
var rq=new XMLHttpRequest();\
function rql(){\
var r=JSON.parse(this.responseText);\
const tbl=document.getElementById('strg-tbl');\
tbl.deleteRow(tbl.rows.length-1);\
for(let i=0;i<r.lst.length;i++){\
const ntr=tbl.insertRow();\
const td1=ntr.insertCell();\
const td2=ntr.insertCell();\
const td3=ntr.insertCell();\
const td4=ntr.insertCell();\
td1.innerHTML=r.lst[i].t==\"D\"?r.dsvg:r.fsvg;\
td2.innerHTML=`<a href=\"${r.lst[i].l}\">${r.lst[i].n.substring(0,25)}</a>`;\
td3.innerHTML=r.lst[i].s;\
td4.innerHTML=r.lst[i].n==\"..\"?'':`<a style='display:table;' href=\"/storage-filedel?df=${r.lst[i].l.replace('/storage?cp=','')+'&loc='+(location.pathname+location.search)}\">\
${r.tsvg}\
</a>`;\
}\
}\
rq.addEventListener('load',rql);\
setTimeout(function(){\
rq.open('GET','/storage-filelist'+location.search);\
rq.send();\
},400);\
</script>\
<style>\
form{\
text-align:left;\
margin:0px 0px 3px 15px;\
}\
input{max-width:185px;}\
.btn{min-width:100px;}\
</style>\
<form action='/storage-fileupload' enctype='multipart/form-data' method='POST'>\
<input name='nf' type='file'>\
<input id='nfl' name='loc' type='text' style='display:none'>\
<button class='btn' type='submit'>\
Upload File\
</button>\
</form>\
<form action='/storage-fileupload' enctype='multipart/form-data' method='POST'>\
<input name='nd' type='text' placeholder='Enter folder name'>\
<input id='ndl' name='loc' type='text' style='display:none'>\
<button class='btn' type='submit'>\
Create Folder\
</button>\
</form>\
";

#endif
