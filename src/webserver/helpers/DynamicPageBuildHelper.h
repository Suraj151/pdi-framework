/************************ Dynamic html tag builder ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _EW_HTML_PAGE_HELPER_H_
#define _EW_HTML_PAGE_HELPER_H_

#include <webserver/helpers/icon/SvgIcons.h>
#include <webserver/helpers/HtmlTagsAndAttr.h>
#include <utility/Utility.h>


/**
 * @var flash message enum
 */
enum FLASH_MSG_TYPE {
  ALERT_DANGER,
  ALERT_SUCCESS,
  ALERT_WARNING
};

void concat_input_html_tag(
  char *_page,
  const char *_name,
  char *_value,
  int _maxlength=HTML_INPUT_TAG_DEFAULT_MAXLENGTH,
  char *_type=HTML_INPUT_TEXT_TAG_TYPE,
  bool _checked=false,
  bool _disabled=false,
  int _min=HTML_INPUT_RANGE_DEFAULT_MIN,
  int _max=HTML_INPUT_RANGE_DEFAULT_MAX
);
void concat_input_html_tag(
  char *_page,
  char *_name,
  char *_value,
  int _maxlength=HTML_INPUT_TAG_DEFAULT_MAXLENGTH,
  char *_type=HTML_INPUT_TEXT_TAG_TYPE,
  bool _checked=false,
  bool _disabled=false,
  int _min=HTML_INPUT_RANGE_DEFAULT_MIN,
  int _max=HTML_INPUT_RANGE_DEFAULT_MAX
);

void concat_td_input_html_tags(
  char *_page,
  const char *_label,
  const char *_name,
  char *_value,
  int _maxlength=HTML_INPUT_TAG_DEFAULT_MAXLENGTH,
  char *_type=HTML_INPUT_TEXT_TAG_TYPE,
  bool _checked=false,
  bool _disabled=false,
  int _min=HTML_INPUT_RANGE_DEFAULT_MIN,
  int _max=HTML_INPUT_RANGE_DEFAULT_MAX
);
void concat_td_input_html_tags(
  char *_page,
  char *_label,
  char *_name,
  char *_value,
  int _maxlength=HTML_INPUT_TAG_DEFAULT_MAXLENGTH,
  char *_type=HTML_INPUT_TEXT_TAG_TYPE,
  bool _checked=false,
  bool _disabled=false,
  int _min=HTML_INPUT_RANGE_DEFAULT_MIN,
  int _max=HTML_INPUT_RANGE_DEFAULT_MAX
);

void concat_tr_input_html_tags(
  char *_page,
  const char *_label,
  const char *_name,
  char *_value,
  int _maxlength=HTML_INPUT_TAG_DEFAULT_MAXLENGTH,
  char *_type=HTML_INPUT_TEXT_TAG_TYPE,
  bool _checked=false,
  bool _disabled=false,
  int _min=HTML_INPUT_RANGE_DEFAULT_MIN,
  int _max=HTML_INPUT_RANGE_DEFAULT_MAX
);
void concat_tr_input_html_tags(
  char *_page,
  char *_label,
  char *_name,
  char *_value,
  int _maxlength=HTML_INPUT_TAG_DEFAULT_MAXLENGTH,
  char *_type=HTML_INPUT_TEXT_TAG_TYPE,
  bool _checked=false,
  bool _disabled=false,
  int _min=HTML_INPUT_RANGE_DEFAULT_MIN,
  int _max=HTML_INPUT_RANGE_DEFAULT_MAX
);

void concat_class_attribute( char *_page, const char *_class );
void concat_class_attribute( char *_page, char *_class );

void concat_style_attribute( char *_page, const char *_style );
void concat_style_attribute( char *_page, char *_style );

void concat_id_attribute( char *_page, const char *_id );
void concat_id_attribute( char *_page, char *_id );

void concat_colspan_attribute( char *_page, const char *_colspan );

void concat_heading_html_tag( char *_page, const char *_heading, uint8_t _heading_level=1, const char *_class_attr=nullptr, const char *_style_attr=nullptr );

void concat_select_html_tag( char *_page, char *_name, char** _options, int _size, int _selected, int _exception=-1, bool _disabled=false );
void concat_select_html_tag( char *_page, const char *_name, char** _options, int _size, int _selected, int _exception=-1, bool _disabled=false );

void concat_td_select_html_tags( char *_page, char *_label, char *_name, char** _options, int _size, int _selected, int _exception=-1, bool _disabled=false );
void concat_td_select_html_tags( char *_page, const char *_label, const char *_name, char** _options, int _size, int _selected, int _exception=-1, bool _disabled=false );
void concat_tr_select_html_tags( char *_page, char *_label, char *_name, char** _options, int _size, int _selected, int _exception=-1, bool _disabled=false );
void concat_tr_select_html_tags( char *_page, const char *_label, const char *_name, char** _options, int _size, int _selected, int _exception=-1, bool _disabled=false );
void concat_tr_heading_html_tags( char *_page, const char *_heading, uint8_t _header_level=1, const char *_colspan_attr=nullptr, const char *_class_attr=nullptr, const char *_style_attr=nullptr );

void concat_flash_message_div( char *_page, const char *_message, int _status );
void concat_flash_message_div( char *_page, char *_message, int _status );

void concat_graph_axis_title_div( char *_page, char *_title, char *_style="" );
void concat_svg_tag( char *_page, const char *_path, int _width=HTML_SVG_DEFAULT_WIDTH, int _height=HTML_SVG_DEFAULT_HEIGHT, char *_fill=HTML_SVG_DEFAULT_FILL );
void concat_svg_menu_card( char *_page, const char *_menu_title, const char *_svg_path, char *_menu_link );

void concat_table_heading_row( char *_page, char** _headings, int _size, const char *_row_class, const char *_row_style, const char *_head_class, const char *_head_style );
void concat_table_data_row( char *_page, char** _data_items, int _size, const char *_row_class, const char *_row_style, const char *_data_class, const char *_data_style );

#endif
