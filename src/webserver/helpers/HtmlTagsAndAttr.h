/*********************** HTML Tags and Attributes *****************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `HtmlTagsAndAttr.h` file defines constants for commonly used HTML tags,
attributes, and default values. These constants are stored in program memory
(PROG_RODT_ATTR) to optimize memory usage on embedded systems. The file also includes
default configurations for input fields, SVG elements, and flash messages.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _HTML_TAGS_AND_ATTRIBUTES_H_
#define _HTML_TAGS_AND_ATTRIBUTES_H_

#include <interface/pdi.h>

/**
 * @section HTML Open and Close Tags
 * Defines constants for opening and closing HTML tags.
 */
static const char HTML_TAG_OPEN_BRACKET   []PROG_RODT_ATTR = "<";
static const char HTML_TAG_CLOSE_BRACKET  []PROG_RODT_ATTR = ">";
static const char HTML_TABLE_OPEN_TAG     []PROG_RODT_ATTR = "<table ";
static const char HTML_TABLE_CLOSE_TAG    []PROG_RODT_ATTR = "</table>";
static const char HTML_TH_OPEN_TAG        []PROG_RODT_ATTR = "<th ";
static const char HTML_TH_CLOSE_TAG       []PROG_RODT_ATTR = "</th>";
static const char HTML_TR_OPEN_TAG        []PROG_RODT_ATTR = "<tr ";
static const char HTML_TR_CLOSE_TAG       []PROG_RODT_ATTR = "</tr>";
static const char HTML_TD_OPEN_TAG        []PROG_RODT_ATTR = "<td ";
static const char HTML_TD_CLOSE_TAG       []PROG_RODT_ATTR = "</td>";
static const char HTML_INPUT_OPEN         []PROG_RODT_ATTR = "<input ";
static const char HTML_SELECT_OPEN        []PROG_RODT_ATTR = "<select ";
static const char HTML_SELECT_CLOSE       []PROG_RODT_ATTR = "</select>";
static const char HTML_OPTION_OPEN        []PROG_RODT_ATTR = "<option ";
static const char HTML_OPTION_CLOSE       []PROG_RODT_ATTR = "</option>";
static const char HTML_DIV_OPEN_TAG       []PROG_RODT_ATTR = "<div";
static const char HTML_DIV_CLOSE_TAG      []PROG_RODT_ATTR = "</div>";
static const char HTML_H1_OPEN_TAG        []PROG_RODT_ATTR = "<h1 ";
static const char HTML_H1_CLOSE_TAG       []PROG_RODT_ATTR = "</h1>";
static const char HTML_H2_OPEN_TAG        []PROG_RODT_ATTR = "<h2 ";
static const char HTML_H2_CLOSE_TAG       []PROG_RODT_ATTR = "</h2>";
static const char HTML_H3_OPEN_TAG        []PROG_RODT_ATTR = "<h3 ";
static const char HTML_H3_CLOSE_TAG       []PROG_RODT_ATTR = "</h3>";
static const char HTML_H4_OPEN_TAG        []PROG_RODT_ATTR = "<h4 ";
static const char HTML_H4_CLOSE_TAG       []PROG_RODT_ATTR = "</h4>";
static const char HTML_SVG_OPEN_TAG       []PROG_RODT_ATTR = "<svg ";
static const char HTML_SVG_CLOSE_TAG      []PROG_RODT_ATTR = "</svg>";
static const char HTML_LINK_OPEN_TAG      []PROG_RODT_ATTR = "<a ";
static const char HTML_LINK_CLOSE_TAG     []PROG_RODT_ATTR = "</a>";
static const char HTML_SPAN_OPEN_TAG      []PROG_RODT_ATTR = "<span";
static const char HTML_SPAN_CLOSE_TAG     []PROG_RODT_ATTR = "</span>";

/**
 * @section HTML Attributes
 * Defines constants for commonly used HTML attributes.
 */
static const char HTML_MAXLEN_ATTR        []PROG_RODT_ATTR = " maxlength=";
static const char HTML_TYPE_ATTR          []PROG_RODT_ATTR = " type=";
static const char HTML_NAME_ATTR          []PROG_RODT_ATTR = " name=";
static const char HTML_VALUE_ATTR         []PROG_RODT_ATTR = " value=";
static const char HTML_ID_ATTR            []PROG_RODT_ATTR = " id=";
static const char HTML_STYLE_ATTR         []PROG_RODT_ATTR = " style=";
static const char HTML_COLSPAN_ATTR       []PROG_RODT_ATTR = " colspan=";
static const char HTML_CLASS_ATTR         []PROG_RODT_ATTR = " class=";
static const char HTML_SELECTED_ATTR      []PROG_RODT_ATTR = " selected='selected' ";
static const char HTML_CHECKED_ATTR       []PROG_RODT_ATTR = " checked='checked' ";
static const char HTML_DISABLED_ATTR      []PROG_RODT_ATTR = " disabled ";
static const char HTML_MIN_RANGE_ATTR     []PROG_RODT_ATTR = " min=";
static const char HTML_MAX_RANGE_ATTR     []PROG_RODT_ATTR = " max=";
static const char HTML_WIDTH_ATTR         []PROG_RODT_ATTR = " width=";
static const char HTML_HEIGHT_ATTR        []PROG_RODT_ATTR = " height=";
static const char HTML_FILL_ATTR          []PROG_RODT_ATTR = " fill=";
static const char HTML_HREF_ATTR          []PROG_RODT_ATTR = " href=";
static const char HTML_VIEWBOX_ATTR       []PROG_RODT_ATTR = " viewBox=";

/**
 * @section Flash Messages
 * Defines constants for default flash messages displayed on the web server.
 */
static const char HTML_SUCCESS_FLASH      []PROG_RODT_ATTR = "Config saved Successfully";
static const char HTML_EMAIL_SUCCESS_FLASH[]PROG_RODT_ATTR = "Config saved & test mail sent (depends on internet and correct mail configs)";

/**
 * @section HTML Tag Types and Defaults
 * Defines constants for input tag types and default configurations.
 */
#define HTML_INPUT_TEXT_TAG_TYPE              "text"
#define HTML_INPUT_FILE_TAG_TYPE              "file"
#define HTML_INPUT_RANGE_TAG_TYPE             "range"
#define HTML_INPUT_CHECKBOX_TAG_TYPE          "checkbox"
#define HTML_INPUT_TAG_DEFAULT_MAXLENGTH      20
#define HTML_INPUT_RANGE_DEFAULT_MIN          0
#define HTML_INPUT_RANGE_DEFAULT_MAX          1024

/**
 * @section SVG Defaults
 * Defines default configurations for SVG elements.
 */
#define HTML_SVG_DEFAULT_WIDTH                48
#define HTML_SVG_DEFAULT_HEIGHT               HTML_SVG_DEFAULT_WIDTH
#define HTML_SVG_DEFAULT_FILL                 "#0062af"

#endif
