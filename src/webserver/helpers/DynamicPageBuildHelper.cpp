/************************ Dynamic html tag builder ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "DynamicPageBuildHelper.h"


/**
 * append style attribute
 *
 * @param	char*	_page
 * @param	PGM_P _style
 */
void concat_style_attribute( char* _page, PGM_P _style ){

  if( _style ){
    strcat_P( _page, HTML_STYLE_ATTR );
    strcat( _page, "'" );
    strcat_P( _page, _style );
    strcat( _page, "'" );
  }
}

/**
 * append style attribute
 *
 * @param	char*	_page
 * @param	char* _style
 */
void concat_style_attribute( char* _page, char* _style ){

  if( _style ){
    strcat_P( _page, HTML_STYLE_ATTR );
    strcat( _page, "'" );
    strcat( _page, _style );
    strcat( _page, "'" );
  }
}

/**
 * append class attribute
 *
 * @param	char*	_page
 * @param	PGM_P _class
 */
void concat_class_attribute( char* _page, PGM_P _class ){

  if( _class ){
    strcat_P( _page, HTML_CLASS_ATTR );
    strcat( _page, "'" );
    strcat_P( _page, _class );
    strcat( _page, "'" );
  }
}

/**
 * append class attribute
 *
 * @param	char*	_page
 * @param	char* _class
 */
void concat_class_attribute( char* _page, char* _class ){

  if( _class ){
    strcat_P( _page, HTML_CLASS_ATTR );
    strcat( _page, "'" );
    strcat( _page, _class );
    strcat( _page, "'" );
  }
}

/**
 * append id attribute
 *
 * @param	char*	_page
 * @param	PGM_P _id
 */
void concat_id_attribute( char* _page, PGM_P _id ){

  if( _id ){
    strcat_P( _page, HTML_ID_ATTR );
    strcat( _page, "'" );
    strcat_P( _page, _id );
    strcat( _page, "'" );
  }
}

/**
 * append id attribute
 *
 * @param	char*	_page
 * @param	char* _id
 */
void concat_id_attribute( char* _page, char* _id ){

  if( _id ){
    strcat_P( _page, HTML_ID_ATTR );
    strcat( _page, "'" );
    strcat( _page, _id );
    strcat( _page, "'" );
  }
}

/**
 * append colspan attribute
 *
 * @param	char*	_page
 * @param	PGM_P _colspan
 */
void concat_colspan_attribute( char* _page, PGM_P _colspan ){

  if( _colspan ){
    strcat_P( _page, HTML_COLSPAN_ATTR );
    strcat( _page, "'" );
    strcat_P( _page, _colspan );
    strcat( _page, "'" );
  }
}


/**
 * build and append heading html tag to html page.
 * this function uses program memory arguments to optimise ram
 *
 * @param	char*	_page
 * @param	PGM_P _header
 * @param	uint8_t	_header_level|1
 * @param	PGM_P _class_attr|nullptr
 * @param	PGM_P _style_attr|nullptr
 */
void concat_heading_html_tag( char* _page, PGM_P _heading, uint8_t _heading_level, PGM_P _class_attr, PGM_P _style_attr ){

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
 * build and append input html tag to html page.
 * given type, name, value etc. attributes.
 * this function uses program memory arguments to optimise ram
 *
 * @param	char*	_page
 * @param	PGM_P _name
 * @param	char*	_value
 * @param	int   _maxlength
 * @param	char*	_type
 * @param	bool  _checked
 * @param	bool  _disabled
 * @param	int   _min
 * @param	int   _max
 */
void concat_input_html_tag(
  char* _page,
  PGM_P _name,
  char* _value,
  int _maxlength,
  char* _type,
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
  strcat_P( _page, PSTR("'/>") );
}


/**
 * build and append input html tag to html page.
 * given type, name, value etc. attributes
 *
 * @param	char*	_page
 * @param	char* _name
 * @param	char*	_value
 * @param	int   _maxlength
 * @param	char*	_type
 * @param	bool  _checked
 * @param	bool  _disabled
 * @param	int   _min
 * @param	int   _max
 */
void concat_input_html_tag(
  char* _page,
  char* _name,
  char* _value,
  int _maxlength,
  char* _type,
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
  strcat_P( _page, PSTR("'/>") );
}

/**
 * build and append td input html tag to html page. It build html tr/input tag
 */
void concat_td_input_html_tags(
  char* _page,
  PGM_P _label,
  PGM_P _name,
  char* _value,
  int _maxlength,
  char* _type,
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

void concat_tr_input_html_tags(
  char* _page,
  PGM_P _label,
  PGM_P _name,
  char* _value,
  int _maxlength,
  char* _type,
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
 * build and append td input html tag to html page. It build html tr/input tag
 */
void concat_td_input_html_tags(
  char* _page,
  char* _label,
  char* _name,
  char* _value,
  int _maxlength,
  char* _type,
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

void concat_tr_input_html_tags(
  char* _page,
  char* _label,
  char* _name,
  char* _value,
  int _maxlength,
  char* _type,
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
 * build and append select html tag to html page.
 * given name, options etc. attributes
 *
 * @param	char*	_page
 * @param	char* _name
 * @param	char**	_options
 * @param	int   _size
 * @param	int   _selected
 * @param	int   _exception
 */
void concat_select_html_tag( char* _page, char* _name, char** _options, int _size, int _selected, int _exception, bool _disabled ){

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

/**
 * build and append select html tag to html page.
 * given name, options etc. attributes
 * this function uses program memory arguments to optimise ram
 *
 * @param	char*	_page
 * @param	PGM_P _name
 * @param	char**	_options
 * @param	int   _size
 * @param	int   _selected
 * @param	int   _exception
 */
void concat_select_html_tag( char* _page, PGM_P _name, char** _options, int _size, int _selected, int _exception, bool _disabled ){

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
 * build and append td select html tag to html page. It build html td/select tag
 */
void concat_td_select_html_tags( char* _page, char* _label, char* _name, char** _options, int _size, int _selected, int _exception, bool _disabled ){

  strcat_P( _page, HTML_TD_OPEN_TAG );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
  strcat( _page, _label );
  strcat_P( _page, HTML_TD_CLOSE_TAG );
  strcat_P( _page, HTML_TD_OPEN_TAG );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
  concat_select_html_tag( _page, _name, _options, _size, _selected, _exception, _disabled );
  strcat_P( _page, HTML_TD_CLOSE_TAG );
}

void concat_tr_select_html_tags( char* _page, char* _label, char* _name, char** _options, int _size, int _selected, int _exception, bool _disabled ){

  strcat_P( _page, HTML_TR_OPEN_TAG );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
  concat_td_select_html_tags( _page, _label, _name, _options, _size, _selected, _exception, _disabled );
  strcat_P( _page, HTML_TR_CLOSE_TAG );
}

/**
 * build and append td select html tag to html page. It build html td/select tag
 */
void concat_td_select_html_tags( char* _page, PGM_P _label, PGM_P _name, char** _options, int _size, int _selected, int _exception, bool _disabled ){

  strcat_P( _page, HTML_TD_OPEN_TAG );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
  strcat_P( _page, _label );
  strcat_P( _page, HTML_TD_CLOSE_TAG );
  strcat_P( _page, HTML_TD_OPEN_TAG );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
  concat_select_html_tag( _page, _name, _options, _size, _selected, _exception, _disabled );
  strcat_P( _page, HTML_TD_CLOSE_TAG );
}

void concat_tr_select_html_tags( char* _page, PGM_P _label, PGM_P _name, char** _options, int _size, int _selected, int _exception, bool _disabled ){

  strcat_P( _page, HTML_TR_OPEN_TAG );
  strcat_P( _page, HTML_TAG_CLOSE_BRACKET );
  concat_td_select_html_tags( _page, _label, _name, _options, _size, _selected, _exception, _disabled );
  strcat_P( _page, HTML_TR_CLOSE_TAG );
}

/**
 * build and append tr heading html tag to html page. It build html tr/header tag with
 * inner html attributes
 * this function uses program memory arguments to optimise ram
 *
 * @param	char*	_page
 * @param	PGM_P _heading
 * @param	uint8_t	_header_level|1
 * @param	PGM_P _colspan_attr|nullptr
 * @param	PGM_P _class_attr|nullptr
 * @param	PGM_P _style_attr|nullptr
 */
void concat_tr_heading_html_tags( char* _page, PGM_P _heading, uint8_t	_header_level, PGM_P _colspan_attr, PGM_P _class_attr, PGM_P _style_attr ){

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
 * build and append div html tag to html page. It build html div tag with inner html
 * this function uses program memory arguments to optimise ram
 *
 * @param	char*	_page
 * @param	PGM_P _message
 * @param	int   _status
 */
void concat_flash_message_div( char* _page, PGM_P _message, int _status ){

  strcat_P( _page, HTML_DIV_OPEN_TAG );
  strcat_P( _page, HTML_CLASS_ATTR );
  strcat_P( _page, PSTR("'msg'") );
  strcat_P( _page, HTML_STYLE_ATTR );
  strcat_P( _page, PSTR("'background:") );
  strcat_P( _page, _status==ALERT_DANGER ? PSTR("#ffb2c7"): _status==ALERT_SUCCESS ? PSTR("#a6eaa8") : PSTR("#f9dc87") );
  strcat_P( _page, PSTR(";'>") );
  strcat_P( _page, _message );
  strcat_P( _page, HTML_DIV_CLOSE_TAG );
}

/**
 * build and append div html tag to html page. It build html div tag with inner html
 *
 * @param	char*	_page
 * @param	char* _message
 * @param	int   _status
 */
void concat_flash_message_div( char* _page, char* _message, int _status ){

  strcat_P( _page, HTML_DIV_OPEN_TAG );
  strcat_P( _page, HTML_CLASS_ATTR );
  strcat_P( _page, PSTR("'msg'") );
  strcat_P( _page, HTML_STYLE_ATTR );
  strcat_P( _page, PSTR("'background:") );
  strcat_P( _page, _status==ALERT_DANGER ? PSTR("#ffb2c7"): _status==ALERT_SUCCESS ? PSTR("#a6eaa8") : PSTR("#f9dc87") );
  strcat_P( _page, PSTR(";'>") );
  strcat( _page, _message );
  strcat_P( _page, HTML_DIV_CLOSE_TAG );
}

/**
 * build and append div html tag for graph axis title.
 *
 * @param	char*	_page
 * @param	char* _title
 * @param	char* _style
 */
void concat_graph_axis_title_div( char* _page, char* _title, char* _style ){

  strcat_P( _page, HTML_DIV_OPEN_TAG );
  strcat_P( _page, HTML_STYLE_ATTR );
  strcat( _page, "'" );
  strcat( _page, _style );
  strcat( _page, ";'>" );
  strcat( _page, _title );
  strcat_P( _page, HTML_DIV_CLOSE_TAG );
}

/**
 * build and append svg html tag to html page.
 * this function uses program memory arguments to optimise ram
 *
 * @param	char*	_page
 * @param	PGM_P _path
 * @param	int   _width
 * @param	int   _height
 * @param	char*	_fill
 */
void concat_svg_tag( char* _page, PGM_P _path, int _width, int _height, char* _fill ){

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
 * build and append menu card to html page.
 * this function uses program memory arguments to optimise ram
 *
 * @param	char*	_page
 * @param	PGM_P _menu_title
 * @param	PGM_P _svg_path
 * @param	char*	_menu_link
 */
void concat_svg_menu_card( char* _page, PGM_P _menu_title, PGM_P _svg_path, char* _menu_link ){

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
 * build and append table heading row
 *
 * @param	char*	_page
 * @param	char**	_headings
 * @param	int   _size
 * @param	PGM_P _row_class
 * @param	PGM_P _row_style
 * @param	PGM_P _head_class
 * @param	PGM_P _head_style
 */
void concat_table_heading_row( char* _page, char** _headings, int _size, PGM_P _row_class, PGM_P _row_style, PGM_P _head_class, PGM_P _head_style ){

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
 * build and append table data row
 *
 * @param	char*	_page
 * @param	char**	_data_items
 * @param	int   _size
 * @param	PGM_P _row_class
 * @param	PGM_P _row_style
 * @param	PGM_P _data_class
 * @param	PGM_P _data_style
 */
void concat_table_data_row( char* _page, char** _data_items, int _size, PGM_P _row_class, PGM_P _row_style, PGM_P _data_class, PGM_P _data_style ){

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
