/************************ Dynamic html tag builder ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_HTTP_SERVER)

#include "DynamicPageBuildHelper.h"

/**
 * @brief Appends a style attribute to the provided buffer.
 *
 * Generates a style attribute and appends it to the provided buffer.
 *
 * @param _page The buffer to which the style attribute is appended.
 * @param _style The style attribute value.
 */
void concat_style_attribute( char *_page, const char *_style ){

  if( _style ){
    strcat_P( _page, HTML_STYLE_ATTR );
    strcat( _page, "'" );
    strcat_P( _page, _style );
    strcat( _page, "'" );
  }
}

void concat_style_attribute( char *_page, char *_style ){

  if( _style ){
    strcat_P( _page, HTML_STYLE_ATTR );
    strcat( _page, "'" );
    strcat( _page, _style );
    strcat( _page, "'" );
  }
}

/**
 * @brief Appends a class attribute to the provided buffer.
 *
 * Generates a class attribute and appends it to the provided buffer.
 *
 * @param _page The buffer to which the class attribute is appended.
 * @param _class The class attribute value.
 */
void concat_class_attribute( char *_page, const char *_class ){

  if( _class ){
    strcat_P( _page, HTML_CLASS_ATTR );
    strcat( _page, "'" );
    strcat_P( _page, _class );
    strcat( _page, "'" );
  }
}

void concat_class_attribute( char *_page, char *_class ){

  if( _class ){
    strcat_P( _page, HTML_CLASS_ATTR );
    strcat( _page, "'" );
    strcat( _page, _class );
    strcat( _page, "'" );
  }
}

/**
 * @brief Appends an ID attribute to the provided buffer.
 *
 * Generates an ID attribute and appends it to the provided buffer.
 *
 * @param _page The buffer to which the ID attribute is appended.
 * @param _id The ID attribute value.
 */
void concat_id_attribute( char *_page, const char *_id ){

  if( _id ){
    strcat_P( _page, HTML_ID_ATTR );
    strcat( _page, "'" );
    strcat_P( _page, _id );
    strcat( _page, "'" );
  }
}

void concat_id_attribute( char *_page, char *_id ){

  if( _id ){
    strcat_P( _page, HTML_ID_ATTR );
    strcat( _page, "'" );
    strcat( _page, _id );
    strcat( _page, "'" );
  }
}

/**
 * @brief Appends a colspan attribute to the provided buffer.
 *
 * Generates a colspan attribute and appends it to the provided buffer.
 *
 * @param _page The buffer to which the colspan attribute is appended.
 * @param _colspan The colspan attribute value.
 */
void concat_colspan_attribute( char *_page, const char *_colspan ){

  if( _colspan ){
    strcat_P( _page, HTML_COLSPAN_ATTR );
    strcat( _page, "'" );
    strcat_P( _page, _colspan );
    strcat( _page, "'" );
  }
}

/**
 * @brief Appends an HTML heading tag to the provided buffer.
 *
 * Generates an HTML heading tag with the specified attributes and appends it
 * to the provided buffer.
 *
 * @param _page The buffer to which the heading tag is appended.
 * @param _heading The heading text.
 * @param _heading_level The level of the heading tag (default: 1).
 * @param _class_attr The class attribute for the heading tag (optional).
 * @param _style_attr The style attribute for the heading tag (optional).
 */
void concat_heading_html_tag( char *_page, const char *_heading, uint8_t _heading_level, const char *_class_attr, const char *_style_attr ){

  strcat_P( _page,
    _heading_level == 1 ? HTML_H1_OPEN_TAG :
    _heading_level == 2 ? HTML_H2_OPEN_TAG :
    _heading_level == 3 ? HTML_H3_OPEN_TAG :
    HTML_H4_OPEN_TAG
  );
  concat_class_attribute( _page, _class_attr );
  concat_style_attribute( _page, _style_attr );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
  strcat_P( _page, _heading );
  strcat_P( _page,
    _heading_level == 1 ? HTML_H1_CLOSE_TAG :
    _heading_level == 2 ? HTML_H2_CLOSE_TAG :
    _heading_level == 3 ? HTML_H3_CLOSE_TAG :
    HTML_H4_CLOSE_TAG
  );
}

/**
 * @brief Appends an HTML input tag to the provided buffer.
 *
 * Generates an HTML input tag with the specified attributes and appends it to
 * the provided buffer.
 *
 * @param _page The buffer to which the HTML input tag is appended.
 * @param _name The name attribute of the input tag.
 * @param _value The value attribute of the input tag.
 * @param _maxlength The maximum length of the input value (default: HTML_INPUT_TAG_DEFAULT_MAXLENGTH).
 * @param _type The type of the input tag (default: HTML_INPUT_TEXT_TAG_TYPE).
 * @param _checked Whether the input tag is checked (default: false).
 * @param _disabled Whether the input tag is disabled (default: false).
 * @param _min The minimum value for range inputs (default: HTML_INPUT_RANGE_DEFAULT_MIN).
 * @param _max The maximum value for range inputs (default: HTML_INPUT_RANGE_DEFAULT_MAX).
 */
void concat_input_html_tag(
  char *_page,
  const char *_name,
  char *_value,
  int _maxlength,
  char *_type,
  bool _checked,
  bool _disabled,
  int _min,
  int _max ){

  bool _is_checkbox = false;
  bool _is_range = false;

  if( 0 <= __strstr( _type, HTML_INPUT_CHECKBOX_TAG_TYPE, 10 ) )
    _is_checkbox = true;

  if( 0 <= __strstr( _type, HTML_INPUT_RANGE_TAG_TYPE, 10 ) )
    _is_range = true;

  strcat_P( _page, HTML_INPUT_OPEN );
  strcat_P( _page, HTML_TYPE_ATTR );
  strcat( _page, "'" );
  strcat( _page, _type );
  strcat( _page, "'" );

  if( !_is_checkbox && !_is_range ){
    char _maxlen[7]; memset(_maxlen, 0, 7);
    itoa( _maxlength, _maxlen, 10 );

    strcat_P( _page, HTML_MAXLEN_ATTR );
    strcat( _page, "'" );
    strcat( _page, _maxlen );
    strcat( _page, "'" );
  }

  if( _checked ){
    strcat_P( _page, HTML_CHECKED_ATTR );
  }

  if( _is_range ){
    char _minbuff[7]; char _maxbuff[7];
    memset(_minbuff, 0, 7); memset(_maxbuff, 0, 7);
    itoa( _min, _minbuff, 10 ); itoa( _max, _maxbuff, 10 );

    strcat_P( _page, HTML_MIN_RANGE_ATTR );
    strcat( _page, "'" );
    strcat( _page, _minbuff );
    strcat( _page, "'" );
    strcat_P( _page, HTML_MAX_RANGE_ATTR );
    strcat( _page, "'" );
    strcat( _page, _maxbuff );
    strcat( _page, "'" );
  }

  strcat_P( _page, HTML_NAME_ATTR );
  strcat( _page, "'" );
  strcat_P( _page, _name );
  strcat( _page, "'" );
  if(_disabled)strcat_P( _page, HTML_DISABLED_ATTR );
  strcat_P( _page, HTML_VALUE_ATTR );
  strcat( _page, "'" );
  strcat( _page, _value );
  strcat_P( _page, RODT_ATTR("'/>") );
}

void concat_input_html_tag(
  char *_page,
  char *_name,
  char *_value,
  int _maxlength,
  char *_type,
  bool _checked,
  bool _disabled,
  int _min,
  int _max ){

  bool _is_checkbox = false;
  bool _is_range = false;

  if( 0 <= __strstr( _type, HTML_INPUT_CHECKBOX_TAG_TYPE, 10 ) )
    _is_checkbox = true;

  if( 0 <= __strstr( _type, HTML_INPUT_RANGE_TAG_TYPE, 10 ) )
    _is_range = true;

  strcat_P( _page, HTML_INPUT_OPEN );
  strcat_P( _page, HTML_TYPE_ATTR );
  strcat( _page, "'" );
  strcat( _page, _type );
  strcat( _page, "'" );

  if( !_is_checkbox && !_is_range ){
    char _maxlen[7]; memset(_maxlen, 0, 7);
    itoa( _maxlength, _maxlen, 10 );

    strcat_P( _page, HTML_MAXLEN_ATTR );
    strcat( _page, "'" );
    strcat( _page, _maxlen );
    strcat( _page, "'" );
  }

  if( _checked ){
    strcat_P( _page, HTML_CHECKED_ATTR );
  }

  if( _is_range ){
    char _minbuff[7]; char _maxbuff[7];
    memset(_minbuff, 0, 7); memset(_maxbuff, 0, 7);
    itoa( _min, _minbuff, 10 ); itoa( _max, _maxbuff, 10 );

    strcat_P( _page, HTML_MIN_RANGE_ATTR );
    strcat( _page, "'" );
    strcat( _page, _minbuff );
    strcat( _page, "'" );
    strcat_P( _page, HTML_MAX_RANGE_ATTR );
    strcat( _page, "'" );
    strcat( _page, _maxbuff );
    strcat( _page, "'" );
  }

  strcat_P( _page, HTML_NAME_ATTR );
  strcat( _page, "'" );
  strcat( _page, _name );
  strcat( _page, "'" );
  if(_disabled)strcat_P( _page, HTML_DISABLED_ATTR );
  strcat_P( _page, HTML_VALUE_ATTR );
  strcat( _page, "'" );
  strcat( _page, _value );
  strcat_P( _page, RODT_ATTR("'/>") );
}

/**
 * @brief Appends an HTML table cell with input fields to the provided buffer.
 *
 * Generates an HTML table cell containing input fields with the specified
 * attributes and appends it to the provided buffer.
 *
 * @param _page The buffer to which the HTML table cell is appended.
 * @param _label The label for the input field.
 * @param _name The name attribute of the input field.
 * @param _value The value attribute of the input field.
 * @param _maxlength The maximum length of the input value (default: HTML_INPUT_TAG_DEFAULT_MAXLENGTH).
 * @param _type The type of the input field (default: HTML_INPUT_TEXT_TAG_TYPE).
 * @param _checked Whether the input field is checked (default: false).
 * @param _disabled Whether the input field is disabled (default: false).
 * @param _min The minimum value for range inputs (default: HTML_INPUT_RANGE_DEFAULT_MIN).
 * @param _max The maximum value for range inputs (default: HTML_INPUT_RANGE_DEFAULT_MAX).
 */
void concat_td_input_html_tags(
  char *_page,
  const char *_label,
  const char *_name,
  char *_value,
  int _maxlength,
  char *_type,
  bool _checked,
  bool _disabled,
  int _min,
  int _max ){

  strcat_P( _page, HTML_TD_OPEN_TAG );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
  strcat_P( _page, _label );
  strcat_P( _page, HTML_TD_CLOSE_TAG );
  strcat_P( _page, HTML_TD_OPEN_TAG );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
  concat_input_html_tag( _page, _name, _value, _maxlength, _type, _checked, _disabled, _min, _max );
  strcat_P( _page, HTML_TD_CLOSE_TAG );
}

void concat_td_input_html_tags(
  char *_page,
  char *_label,
  char *_name,
  char *_value,
  int _maxlength,
  char *_type,
  bool _checked,
  bool _disabled,
  int _min,
  int _max ){

  strcat_P( _page, HTML_TD_OPEN_TAG );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
  strcat( _page, _label );
  strcat_P( _page, HTML_TD_CLOSE_TAG );
  strcat_P( _page, HTML_TD_OPEN_TAG );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
  concat_input_html_tag( _page, _name, _value, _maxlength, _type, _checked, _disabled, _min, _max );
  strcat_P( _page, HTML_TD_CLOSE_TAG );
}

/**
 * @brief Appends an HTML table row with input fields to the provided buffer.
 *
 * Generates an HTML table row containing input fields with the specified
 * attributes and appends it to the provided buffer.
 *
 * @param _page The buffer to which the HTML table row is appended.
 * @param _label The label for the input field.
 * @param _name The name attribute of the input field.
 * @param _value The value attribute of the input field.
 * @param _maxlength The maximum length of the input value (default: HTML_INPUT_TAG_DEFAULT_MAXLENGTH).
 * @param _type The type of the input field (default: HTML_INPUT_TEXT_TAG_TYPE).
 * @param _checked Whether the input field is checked (default: false).
 * @param _disabled Whether the input field is disabled (default: false).
 * @param _min The minimum value for range inputs (default: HTML_INPUT_RANGE_DEFAULT_MIN).
 * @param _max The maximum value for range inputs (default: HTML_INPUT_RANGE_DEFAULT_MAX).
 */
void concat_tr_input_html_tags(
    char *_page,
    const char *_label,
    const char *_name,
    char *_value,
    int _maxlength,
    char *_type,
    bool _checked,
    bool _disabled,
    int _min,
    int _max ){
  
      strcat_P( _page, HTML_TR_OPEN_TAG );
      strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
      concat_td_input_html_tags(_page, _label, _name, _value, _maxlength, _type, _checked, _disabled, _min, _max);
      strcat_P( _page, HTML_TR_CLOSE_TAG );
  }
  
  
void concat_tr_input_html_tags(
  char *_page,
  char *_label,
  char *_name,
  char *_value,
  int _maxlength,
  char *_type,
  bool _checked,
  bool _disabled,
  int _min,
  int _max ){

    strcat_P( _page, HTML_TR_OPEN_TAG );
    strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
    concat_td_input_html_tags(_page, _label, _name, _value, _maxlength, _type, _checked, _disabled, _min, _max);
    strcat_P( _page, HTML_TR_CLOSE_TAG );
}

/**
 * @brief Appends an HTML select dropdown to the provided buffer.
 *
 * Generates an HTML select dropdown with the specified options and appends it
 * to the provided buffer.
 *
 * @param _page The buffer to which the select dropdown is appended.
 * @param _name The name attribute of the select dropdown.
 * @param _options The array of options for the dropdown.
 * @param _size The number of options in the dropdown.
 * @param _selected The index of the selected option.
 * @param _exception The index of an option to exclude (default: -1).
 * @param _disabled Whether the dropdown is disabled (default: false).
 */
void concat_select_html_tag( char *_page, char *_name, char** _options, int _size, int _selected, int _exception, bool _disabled ){

  strcat_P( _page, HTML_SELECT_OPEN );
  if(_disabled)strcat_P( _page, HTML_DISABLED_ATTR );
  strcat_P( _page, HTML_NAME_ATTR );
  strcat( _page, "'" );
  strcat( _page, _name );
  strcat( _page, "'>" );

  for (int i = 0; i < _size; i++) {

    if( strlen(_options[i]) > 0 && ( ( i ) != _exception ) ){
      char buf[3];
      memset( buf, 0, 3 );
      itoa( i, buf, 10 );
      strcat_P( _page, HTML_OPTION_OPEN );
      strcat_P( _page, HTML_VALUE_ATTR );
      strcat( _page, "'" );
      strcat( _page, buf );
      strcat( _page, "'" );
      if( _selected == i )
      strcat_P( _page, HTML_SELECTED_ATTR );
      strcat( _page, ">" );
      strcat( _page, _options[i] );
      strcat_P( _page, HTML_OPTION_CLOSE );
    }
  }
  strcat_P( _page, HTML_SELECT_CLOSE );
}

void concat_select_html_tag( char *_page, const char *_name, char** _options, int _size, int _selected, int _exception, bool _disabled ){

  strcat_P( _page, HTML_SELECT_OPEN );
  if(_disabled)strcat_P( _page, HTML_DISABLED_ATTR );
  strcat_P( _page, HTML_NAME_ATTR );
  strcat( _page, "'" );
  strcat_P( _page, _name );
  strcat( _page, "'>" );

  for (int i = 0; i < _size; i++) {

    if( strlen(_options[i]) > 0 && ( ( i ) != _exception ) ){
      char buf[3];
      memset( buf, 0, 3 );
      itoa( i, buf, 10 );
      strcat_P( _page, HTML_OPTION_OPEN );
      strcat_P( _page, HTML_VALUE_ATTR );
      strcat( _page, "'" );
      strcat( _page, buf );
      strcat( _page, "'" );
      if( _selected == i )
      strcat_P( _page, HTML_SELECTED_ATTR );
      strcat( _page, ">" );
      strcat( _page, _options[i] );
      strcat_P( _page, HTML_OPTION_CLOSE );
    }
  }
  strcat_P( _page, HTML_SELECT_CLOSE );
}

/**
 * @brief Appends an HTML table cell with select dropdowns to the provided buffer.
 *
 * Generates an HTML table cell containing select dropdowns with the specified
 * attributes and appends it to the provided buffer.
 *
 * @param _page The buffer to which the table cell is appended.
 * @param _label The label for the dropdown.
 * @param _name The name attribute of the dropdown.
 * @param _options The array of options for the dropdown.
 * @param _size The number of options in the dropdown.
 * @param _selected The index of the selected option.
 * @param _exception The index of an option to exclude (default: -1).
 * @param _disabled Whether the dropdown is disabled (default: false).
 */
void concat_td_select_html_tags( char *_page, char *_label, char *_name, char** _options, int _size, int _selected, int _exception, bool _disabled ){

  strcat_P( _page, HTML_TD_OPEN_TAG );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
  strcat( _page, _label );
  strcat_P( _page, HTML_TD_CLOSE_TAG );
  strcat_P( _page, HTML_TD_OPEN_TAG );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
  concat_select_html_tag( _page, _name, _options, _size, _selected, _exception, _disabled );
  strcat_P( _page, HTML_TD_CLOSE_TAG );
}

void concat_td_select_html_tags( char *_page, const char *_label, const char *_name, char** _options, int _size, int _selected, int _exception, bool _disabled ){

  strcat_P( _page, HTML_TD_OPEN_TAG );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
  strcat_P( _page, _label );
  strcat_P( _page, HTML_TD_CLOSE_TAG );
  strcat_P( _page, HTML_TD_OPEN_TAG );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
  concat_select_html_tag( _page, _name, _options, _size, _selected, _exception, _disabled );
  strcat_P( _page, HTML_TD_CLOSE_TAG );
}

/**
 * @brief Appends an HTML table row with select dropdowns to the provided buffer.
 *
 * Generates an HTML table row containing select dropdowns with the specified
 * attributes and appends it to the provided buffer.
 *
 * @param _page The buffer to which the table row is appended.
 * @param _label The label for the dropdown.
 * @param _name The name attribute of the dropdown.
 * @param _options The array of options for the dropdown.
 * @param _size The number of options in the dropdown.
 * @param _selected The index of the selected option.
 * @param _exception The index of an option to exclude (default: -1).
 * @param _disabled Whether the dropdown is disabled (default: false).
 */
void concat_tr_select_html_tags( char *_page, const char *_label, const char *_name, char** _options, int _size, int _selected, int _exception, bool _disabled ){

  strcat_P( _page, HTML_TR_OPEN_TAG );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
  concat_td_select_html_tags( _page, _label, _name, _options, _size, _selected, _exception, _disabled );
  strcat_P( _page, HTML_TR_CLOSE_TAG );
}

void concat_tr_select_html_tags( char *_page, char *_label, char *_name, char** _options, int _size, int _selected, int _exception, bool _disabled ){

  strcat_P( _page, HTML_TR_OPEN_TAG );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
  concat_td_select_html_tags( _page, _label, _name, _options, _size, _selected, _exception, _disabled );
  strcat_P( _page, HTML_TR_CLOSE_TAG );
}


/**
 * @brief Appends an HTML table head with select dropdowns to the provided buffer.
 *
 * Generates an HTML table head containing select dropdowns with the specified
 * attributes and appends it to the provided buffer.
 *
 * @param _page The buffer to which the table head is appended.
 * @param _heading The heading text for the table head.
 * @param _header_level The level of the heading (default: 1).
 * @param _colspan_attr The colspan attribute for the table head (optional).
 * @param _class_attr The class attribute for the table head (optional).
 * @param _style_attr The style attribute for the table head (optional).
 */
void concat_tr_heading_html_tags( char *_page, const char *_heading, uint8_t	_header_level, const char *_colspan_attr, const char *_class_attr, const char *_style_attr ){

  strcat_P( _page, HTML_TR_OPEN_TAG );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
  strcat_P( _page, HTML_TD_OPEN_TAG );
  concat_colspan_attribute( _page, _colspan_attr );
  concat_class_attribute( _page, _class_attr );
  concat_style_attribute( _page, _style_attr );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
  concat_heading_html_tag( _page, _heading, _header_level );
  strcat_P( _page, HTML_TD_CLOSE_TAG );
  strcat_P( _page, HTML_TR_CLOSE_TAG );
}

/**
 * @brief Appends a flash message div to the provided buffer.
 *
 * Generates an HTML div for displaying a flash message with the specified
 * status and appends it to the provided buffer.
 *
 * @param _page The buffer to which the flash message div is appended.
 * @param _message The message to display in the flash message.
 * @param _status The status of the flash message (e.g., ALERT_SUCCESS).
 */
void concat_flash_message_div( char *_page, const char *_message, int _status ){

  strcat_P( _page, HTML_DIV_OPEN_TAG );
  strcat_P( _page, HTML_CLASS_ATTR );
  strcat_P( _page, RODT_ATTR("'msg'") );
  strcat_P( _page, HTML_STYLE_ATTR );
  strcat_P( _page, RODT_ATTR("'background:") );
  strcat_P( _page, _status==ALERT_DANGER ? RODT_ATTR("#ffb2c7"): _status==ALERT_SUCCESS ? RODT_ATTR("#a6eaa8") : RODT_ATTR("#f9dc87") );
  strcat_P( _page, RODT_ATTR(";'>") );
  strcat_P( _page, _message );
  strcat_P( _page, HTML_DIV_CLOSE_TAG );
}

void concat_flash_message_div( char *_page, char *_message, int _status ){

  strcat_P( _page, HTML_DIV_OPEN_TAG );
  strcat_P( _page, HTML_CLASS_ATTR );
  strcat_P( _page, RODT_ATTR("'msg'") );
  strcat_P( _page, HTML_STYLE_ATTR );
  strcat_P( _page, RODT_ATTR("'background:") );
  strcat_P( _page, _status==ALERT_DANGER ? RODT_ATTR("#ffb2c7"): _status==ALERT_SUCCESS ? RODT_ATTR("#a6eaa8") : RODT_ATTR("#f9dc87") );
  strcat_P( _page, RODT_ATTR(";'>") );
  strcat( _page, _message );
  strcat_P( _page, HTML_DIV_CLOSE_TAG );
}

/**
 * @brief Appends a graph axis title div to the provided buffer.
 *
 * Generates an HTML div for displaying a graph axis title and appends it to
 * the provided buffer.
 *
 * @param _page The buffer to which the graph axis title div is appended.
 * @param _title The title text for the graph axis.
 * @param _style The style attribute for the graph axis title (optional).
 */
void concat_graph_axis_title_div( char *_page, char *_title, char *_style ){

  strcat_P( _page, HTML_DIV_OPEN_TAG );
  strcat_P( _page, HTML_STYLE_ATTR );
  strcat( _page, "'" );
  strcat( _page, _style );
  strcat( _page, ";'>" );
  strcat( _page, _title );
  strcat_P( _page, HTML_DIV_CLOSE_TAG );
}

/**
 * @brief Appends an SVG element to the provided buffer.
 *
 * Generates an SVG element with the specified attributes and appends it to
 * the provided buffer.
 *
 * @param _page The buffer to which the SVG element is appended.
 * @param _path The SVG path data.
 * @param _width The width of the SVG element (default: HTML_SVG_DEFAULT_WIDTH).
 * @param _height The height of the SVG element (default: HTML_SVG_DEFAULT_HEIGHT).
 * @param _fill The fill color of the SVG element (default: HTML_SVG_DEFAULT_FILL).
 */
void concat_svg_tag( char *_page, const char *_path, int _width, int _height, char *_fill ){

  char _widthbuff[7]; memset(_widthbuff, 0, 7);
  itoa( _width, _widthbuff, 10 );
  char _heightbuff[7]; memset(_heightbuff, 0, 7);
  itoa( _height, _heightbuff, 10 );

  strcat_P( _page, HTML_SVG_OPEN_TAG );
  strcat_P( _page, HTML_WIDTH_ATTR );
  strcat( _page, "'" );
  strcat( _page, _widthbuff );
  strcat( _page, "'" );
  strcat_P( _page, HTML_HEIGHT_ATTR );
  strcat( _page, "'" );
  strcat( _page, _heightbuff );
  strcat( _page, "'" );
  strcat_P( _page, HTML_FILL_ATTR );
  strcat( _page, "'" );
  strcat( _page, _fill );
  strcat( _page, "'>" );
  strcat_P( _page, _path );
  strcat_P( _page, HTML_SVG_CLOSE_TAG );
}

/**
 * @brief Appends an SVG menu card to the provided buffer.
 *
 * Generates an SVG menu card with the specified attributes and appends it to
 * the provided buffer.
 *
 * @param _page The buffer to which the SVG menu card is appended.
 * @param _menu_title The title of the menu card.
 * @param _svg_path The SVG path data for the menu card icon.
 * @param _menu_link The link associated with the menu card.
 */
void concat_svg_menu_card( char *_page, const char *_menu_title, const char *_svg_path, char *_menu_link ){

  strcat_P( _page, HTML_DIV_OPEN_TAG );
  strcat( _page, ">" );
  strcat_P( _page, HTML_LINK_OPEN_TAG );
  strcat_P( _page, HTML_HREF_ATTR );

  strcat( _page, "'" );
  strcat( _page, _menu_link );
  strcat( _page, "'>" );

  concat_svg_tag( _page, _svg_path );

  strcat_P( _page, HTML_SPAN_OPEN_TAG );
  strcat( _page, ">" );
  strcat_P( _page, _menu_title );
  strcat_P( _page, HTML_SPAN_CLOSE_TAG );

  strcat_P( _page, HTML_LINK_CLOSE_TAG );
  strcat_P( _page, HTML_DIV_CLOSE_TAG );
}

/**
 * @brief Appends an HTML table heading row to the provided buffer.
 *
 * Generates an HTML table heading row with the specified attributes and appends
 * it to the provided buffer.
 *
 * @param _page The buffer to which the table heading row is appended.
 * @param _headings The array of headings for the table row.
 * @param _size The number of headings in the row.
 * @param _row_class The class attribute for the table row (optional).
 * @param _row_style The style attribute for the table row (optional).
 * @param _head_class The class attribute for the table headings (optional).
 * @param _head_style The style attribute for the table headings (optional).
 */
void concat_table_heading_row( char *_page, char** _headings, int _size, const char *_row_class, const char *_row_style, const char *_head_class, const char *_head_style ){

  strcat_P( _page, HTML_TR_OPEN_TAG );
  concat_class_attribute( _page, _row_class );
  concat_style_attribute( _page, _row_style );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );

  for (int i = 0; i < _size; i++) {
    strcat_P( _page, HTML_TH_OPEN_TAG );
    concat_class_attribute( _page, _head_class );
    concat_style_attribute( _page, _head_style );
    strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
    strcat( _page, _headings[i] );
    strcat_P( _page, HTML_TH_CLOSE_TAG );
  }

  strcat_P( _page, HTML_TR_CLOSE_TAG );
}

/**
 * @brief Appends an HTML table data row to the provided buffer.
 *
 * Generates an HTML table data row with the specified attributes and appends
 * it to the provided buffer.
 *
 * @param _page The buffer to which the table data row is appended.
 * @param _data_items The array of data items for the table row.
 * @param _size The number of data items in the row.
 * @param _row_class The class attribute for the table row (optional).
 * @param _row_style The style attribute for the table row (optional).
 * @param _data_class The class attribute for the table data cells (optional).
 * @param _data_style The style attribute for the table data cells (optional).
 */
void concat_table_data_row( char *_page, char** _data_items, int _size, const char *_row_class, const char *_row_style, const char *_data_class, const char *_data_style ){

  strcat_P( _page, HTML_TR_OPEN_TAG );
  concat_class_attribute( _page, _row_class );
  concat_style_attribute( _page, _row_style );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );

  for (int i = 0; i < _size; i++) {
    strcat_P( _page, HTML_TD_OPEN_TAG );
    concat_class_attribute( _page, _data_class );
    concat_style_attribute( _page, _data_style );
    strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
    strcat( _page, _data_items[i] );
    strcat_P( _page, HTML_TD_CLOSE_TAG );
  }

  strcat_P( _page, HTML_TR_CLOSE_TAG );
}

#endif