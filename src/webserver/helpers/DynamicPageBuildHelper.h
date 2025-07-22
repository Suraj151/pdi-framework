/************************ Dynamic HTML Tag Builder ****************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `DynamicPageBuildHelper.h` file provides utility functions to dynamically
generate HTML content for the web server. These functions allow for the creation
of various HTML elements, such as input fields, table rows, headings, select
dropdowns, SVG elements, and flash messages. The generated HTML is appended to
a provided buffer to construct complete web pages dynamically.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _EW_HTML_PAGE_HELPER_H_
#define _EW_HTML_PAGE_HELPER_H_

#include <webserver/helpers/icon/SvgIcons.h>
#include <webserver/helpers/HtmlTagsAndAttr.h>
#include <utility/Utility.h>

/**
 * @enum FLASH_MSG_TYPE
 * @brief Defines the types of flash messages displayed on the web server.
 *
 * - `ALERT_DANGER`: Represents an error or danger message.
 * - `ALERT_SUCCESS`: Represents a success message.
 * - `ALERT_WARNING`: Represents a warning message.
 */
enum FLASH_MSG_TYPE {
  ALERT_DANGER,
  ALERT_SUCCESS,
  ALERT_WARNING
};

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
  int _maxlength = HTML_INPUT_TAG_DEFAULT_MAXLENGTH,
  char *_type = HTML_INPUT_TEXT_TAG_TYPE,
  bool _checked = false,
  bool _disabled = false,
  int _min = HTML_INPUT_RANGE_DEFAULT_MIN,
  int _max = HTML_INPUT_RANGE_DEFAULT_MAX
);

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
  int _maxlength = HTML_INPUT_TAG_DEFAULT_MAXLENGTH,
  char *_type = HTML_INPUT_TEXT_TAG_TYPE,
  bool _checked = false,
  bool _disabled = false,
  int _min = HTML_INPUT_RANGE_DEFAULT_MIN,
  int _max = HTML_INPUT_RANGE_DEFAULT_MAX
);

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
  char *_label,
  char *_name,
  char *_value,
  int _maxlength = HTML_INPUT_TAG_DEFAULT_MAXLENGTH,
  char *_type = HTML_INPUT_TEXT_TAG_TYPE,
  bool _checked = false,
  bool _disabled = false,
  int _min = HTML_INPUT_RANGE_DEFAULT_MIN,
  int _max = HTML_INPUT_RANGE_DEFAULT_MAX
);

/**
 * @brief Appends a class attribute to the provided buffer.
 *
 * Generates a class attribute and appends it to the provided buffer.
 *
 * @param _page The buffer to which the class attribute is appended.
 * @param _class The class attribute value.
 */
void concat_class_attribute(char *_page, const char *_class);
void concat_class_attribute(char *_page, char *_class);

/**
 * @brief Appends a style attribute to the provided buffer.
 *
 * Generates a style attribute and appends it to the provided buffer.
 *
 * @param _page The buffer to which the style attribute is appended.
 * @param _style The style attribute value.
 */
void concat_style_attribute(char *_page, const char *_style);
void concat_style_attribute(char *_page, char *_style);

/**
 * @brief Appends an ID attribute to the provided buffer.
 *
 * Generates an ID attribute and appends it to the provided buffer.
 *
 * @param _page The buffer to which the ID attribute is appended.
 * @param _id The ID attribute value.
 */
void concat_id_attribute(char *_page, const char *_id);
void concat_id_attribute(char *_page, char *_id);

/**
 * @brief Appends a colspan attribute to the provided buffer.
 *
 * Generates a colspan attribute and appends it to the provided buffer.
 *
 * @param _page The buffer to which the colspan attribute is appended.
 * @param _colspan The colspan attribute value.
 */
void concat_colspan_attribute(char *_page, const char *_colspan);

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
void concat_heading_html_tag(
  char *_page,
  const char *_heading,
  uint8_t _heading_level = 1,
  const char *_class_attr = nullptr,
  const char *_style_attr = nullptr
);

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
void concat_select_html_tag(
  char *_page,
  char *_name,
  char **_options,
  int _size,
  int _selected,
  int _exception = -1,
  bool _disabled = false
);
void concat_select_html_tag(
  char *_page,
  const char *_name,
  char **_options,
  int _size,
  int _selected,
  int _exception = -1,
  bool _disabled = false
);

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
void concat_td_select_html_tags(
  char *_page,
  char *_label,
  char *_name,
  char **_options,
  int _size,
  int _selected,
  int _exception = -1,
  bool _disabled = false
);
void concat_td_select_html_tags(
  char *_page,
  const char *_label,
  const char *_name,
  char **_options,
  int _size,
  int _selected,
  int _exception = -1,
  bool _disabled = false
);

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
void concat_tr_select_html_tags(
  char *_page,
  char *_label,
  char *_name,
  char **_options,
  int _size,
  int _selected,
  int _exception = -1,
  bool _disabled = false
);
void concat_tr_select_html_tags(
  char *_page,
  const char *_label,
  const char *_name,
  char **_options,
  int _size,
  int _selected,
  int _exception = -1,
  bool _disabled = false
);

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
void concat_tr_heading_html_tags(
  char *_page,
  const char *_heading,
  uint8_t _header_level = 1,
  const char *_colspan_attr = nullptr, 
  const char *_class_attr = nullptr, 
  const char *_style_attr = nullptr
);

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
void concat_flash_message_div(char *_page, const char *_message, int _status);
void concat_flash_message_div(char *_page, char *_message, int _status);

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
void concat_graph_axis_title_div(char *_page, char *_title, char *_style = "");

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
void concat_svg_tag(
  char *_page,
  const char *_path,
  const char *_style = nullptr,
  const char *_viewbox = nullptr,
  int _width = HTML_SVG_DEFAULT_WIDTH,
  int _height = HTML_SVG_DEFAULT_HEIGHT,
  char *_fill = HTML_SVG_DEFAULT_FILL
);

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
void concat_svg_menu_card(
  char *_page,
  const char *_menu_title,
  const char *_svg_path,
  char *_menu_link
);

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
void concat_table_heading_row(
  char *_page,
  char **_headings,
  int _size,
  const char *_row_class,
  const char *_row_style,
  const char *_head_class,
  const char *_head_style
);

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
void concat_table_data_row(
  char *_page,
  char **_data_items,
  int _size,
  const char *_row_class,
  const char *_row_style,
  const char *_data_class,
  const char *_data_style,
  const char **_td_colspan_attr = nullptr
);

/**
 * @brief Appends an HTML link element to the provided buffer.
 *
 * Generates an HTML link element with the specified attributes and appends
 * it to the provided buffer.
 *
 * @param _page The buffer to which the link element is appended.
 * @param _href The href attribute for the link.
 * @param _innerhtml The innerhtml part of link.
 * @param _class The class attribute for the link(optional).
 * @param _style The style attribute for the link(optional).
 */
void concat_link_element(
  char *_page,
  const char *_href,
  const char *_innerhtml,
  const char *_class = nullptr,
  const char *_style = nullptr
);

#endif
