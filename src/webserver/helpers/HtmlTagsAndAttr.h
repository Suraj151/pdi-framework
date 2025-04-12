/*********************** HTML Tags and Attributes *****************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `HtmlTagsAndAttr.h` file defines constants for commonly used HTML tags,
attributes, and default values. These constants are stored in program memory
(PROGMEM) to optimize memory usage on embedded systems. The file also includes
default configurations for input fields, SVG elements, and flash messages.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _HTML_TAGS_AND_ATTRIBUTES_H_
#define _HTML_TAGS_AND_ATTRIBUTES_H_

#include <Arduino.h>
#include <config/Config.h>

/**
 * @section HTML Open and Close Tags
 * Defines constants for opening and closing HTML tags.
 */
static const char HTML_TAG_OPEN_BRACKET   []PROGMEM = "<";
static const char HTML_TAG_CLOSE_BRACKET  []PROGMEM = ">";
static const char HTML_TABLE_OPEN_TAG     []PROGMEM = "<table ";
static const char HTML_TABLE_CLOSE_TAG    []PROGMEM = "</table>";
static const char HTML_TH_OPEN_TAG        []PROGMEM = "<th ";
static const char HTML_TH_CLOSE_TAG       []PROGMEM = "</th>";
static const char HTML_TR_OPEN_TAG        []PROGMEM = "<tr ";
static const char HTML_TR_CLOSE_TAG       []PROGMEM = "</tr>";
static const char HTML_TD_OPEN_TAG        []PROGMEM = "<td ";
static const char HTML_TD_CLOSE_TAG       []PROGMEM = "</td>";
static const char HTML_INPUT_OPEN         []PROGMEM = "<input ";
static const char HTML_SELECT_OPEN        []PROGMEM = "<select ";
static const char HTML_SELECT_CLOSE       []PROGMEM = "</select>";
static const char HTML_OPTION_OPEN        []PROGMEM = "<option ";
static const char HTML_OPTION_CLOSE       []PROGMEM = "</option>";
static const char HTML_DIV_OPEN_TAG       []PROGMEM = "<div";
static const char HTML_DIV_CLOSE_TAG      []PROGMEM = "</div>";
static const char HTML_H1_OPEN_TAG        []PROGMEM = "<h1 ";
static const char HTML_H1_CLOSE_TAG       []PROGMEM = "</h1>";
static const char HTML_H2_OPEN_TAG        []PROGMEM = "<h2 ";
static const char HTML_H2_CLOSE_TAG       []PROGMEM = "</h2>";
static const char HTML_H3_OPEN_TAG        []PROGMEM = "<h3 ";
static const char HTML_H3_CLOSE_TAG       []PROGMEM = "</h3>";
static const char HTML_H4_OPEN_TAG        []PROGMEM = "<h4 ";
static const char HTML_H4_CLOSE_TAG       []PROGMEM = "</h4>";
static const char HTML_SVG_OPEN_TAG       []PROGMEM = "<svg ";
static const char HTML_SVG_CLOSE_TAG      []PROGMEM = "</svg>";
static const char HTML_LINK_OPEN_TAG      []PROGMEM = "<a ";
static const char HTML_LINK_CLOSE_TAG     []PROGMEM = "</a>";
static const char HTML_SPAN_OPEN_TAG      []PROGMEM = "<span";
static const char HTML_SPAN_CLOSE_TAG     []PROGMEM = "</span>";

/**
 * @section HTML Attributes
 * Defines constants for commonly used HTML attributes.
 */
static const char HTML_MAXLEN_ATTR        []PROGMEM = " maxlength=";
static const char HTML_TYPE_ATTR          []PROGMEM = " type=";
static const char HTML_NAME_ATTR          []PROGMEM = " name=";
static const char HTML_VALUE_ATTR         []PROGMEM = " value=";
static const char HTML_ID_ATTR            []PROGMEM = " id=";
static const char HTML_STYLE_ATTR         []PROGMEM = " style=";
static const char HTML_COLSPAN_ATTR       []PROGMEM = " colspan=";
static const char HTML_CLASS_ATTR         []PROGMEM = " class=";
static const char HTML_SELECTED_ATTR      []PROGMEM = " selected='selected' ";
static const char HTML_CHECKED_ATTR       []PROGMEM = " checked='checked' ";
static const char HTML_DISABLED_ATTR      []PROGMEM = " disabled ";
static const char HTML_MIN_RANGE_ATTR     []PROGMEM = " min=";
static const char HTML_MAX_RANGE_ATTR     []PROGMEM = " max=";
static const char HTML_WIDTH_ATTR         []PROGMEM = " width=";
static const char HTML_HEIGHT_ATTR        []PROGMEM = " height=";
static const char HTML_FILL_ATTR          []PROGMEM = " fill=";
static const char HTML_HREF_ATTR          []PROGMEM = " href=";

/**
 * @section Flash Messages
 * Defines constants for default flash messages displayed on the web server.
 */
static const char HTML_SUCCESS_FLASH      []PROGMEM = "Config saved Successfully";
static const char HTML_EMAIL_SUCCESS_FLASH[]PROGMEM = "Config saved & test mail sent (depends on internet and correct mail configs)";

/**
 * @section HTML Tag Types and Defaults
 * Defines constants for input tag types and default configurations.
 */
#define HTML_INPUT_TEXT_TAG_TYPE              "text"
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
