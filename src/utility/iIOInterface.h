/************************** Input/Output Interface ***************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The iIOInterface class defines an abstract interface for input/output operations.
It provides a standardized API for connecting, disconnecting, sending, and receiving
data. This interface is designed to be implemented by various communication backends
such as serial, network, or other I/O devices.

Author          : Suraj I.
Created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_IO_INTERFACE_H_
#define _I_IO_INTERFACE_H_

#include "DataTypeDef.h"

/**
 * @class iIOInterface
 * @brief Abstract interface for input/output operations.
 *
 * The iIOInterface class provides a set of virtual methods for managing connections,
 * sending data, receiving data, and checking connection states. It is designed to
 * be implemented by specific I/O backends such as serial communication or network
 * sockets.
 */
class iIOInterface
{

public:
  /**
   * @brief Constructor for the iIOInterface class.
   *
   * Initializes the iIOInterface object.
   */
  iIOInterface() {}

  /**
   * @brief Destructor for the iIOInterface class.
   *
   * Cleans up resources used by the iIOInterface object.
   */
  virtual ~iIOInterface() {}

  // Connection management APIs

  /**
   * @brief Connects to a host using an IP address and port.
   * @param host Pointer to the host IP address.
   * @param port The port number to connect to.
   * @return Connection status (0 for success, negative for failure).
   */
  virtual int16_t connect(const uint8_t *host, uint16_t port) { return -1; }

  /**
   * @brief Connects using a port and speed (e.g., for serial communication).
   * @param port The port number to connect to.
   * @param speed The communication speed (e.g., baud rate).
   * @return Connection status (0 for success, negative for failure).
   */
  virtual int16_t connect(uint16_t port, uint64_t speed) { return -1; }

  /**
   * @brief Disconnects the current connection.
   * @return Disconnection status (0 for success, negative for failure).
   */
  virtual int16_t disconnect() = 0;

  /**
   * @brief Opens a connection using a port and speed.
   * @param port The port number to open (default is 0).
   * @param speed The communication speed (default is 115200).
   * @return Connection status (0 for success, negative for failure).
   */
  virtual int16_t open(uint16_t port = 0, uint64_t speed = 115200) { return connect(port, speed); }

  /**
   * @brief Closes the current connection.
   * @return Disconnection status (0 for success, negative for failure).
   */
  virtual int16_t close() { return disconnect(); }

  // Data sending APIs

  /**
   * @brief Writes a single byte of data.
   * @param c The byte to write.
   * @return Number of bytes written.
   */
  virtual uint32_t write(uint8_t c) = 0;

  /**
   * @brief Writes a string of bytes.
   * @param c_str Pointer to the byte string.
   * @return Number of bytes written.
   */
  virtual uint32_t write(const uint8_t *c_str) = 0;

  /**
   * @brief Writes a specified number of bytes.
   * @param c_str Pointer to the byte string.
   * @param size The number of bytes to write.
   * @return Number of bytes written.
   */
  virtual uint32_t write(const uint8_t *c_str, uint32_t size) = 0;

  /**
   * @brief Writes a single character.
   * @param c The character to write.
   * @return Number of bytes written.
   */
  virtual uint32_t write(char c) { return write((uint8_t)c); }

  /**
   * @brief Writes a string of characters.
   * @param c_str Pointer to the character string.
   * @return Number of bytes written.
   */
  virtual uint32_t write(const char *c_str) { return write((const uint8_t *)c_str); }

  /**
   * @brief Writes a specified number of characters.
   * @param c_str Pointer to the character string.
   * @param size The number of characters to write.
   * @return Number of bytes written.
   */
  virtual uint32_t write(const char *c_str, uint32_t size) { return write((const uint8_t *)c_str, size); }

  /**
   * @brief Writes a specified number of characters.
   * @param c_str Pointer to the character string.
   * @param maxsize The maximum number of characters to write.
   * @param pad The character to use for padding (default is space).
   * @return Number of bytes written.
   */
  virtual uint32_t write_pad(const char *c_str, uint32_t maxsize, bool prepad=false, char pad=' ') {
    uint32_t size = strlen(c_str);
    if (size < maxsize) {

      for (size_t i = 0; prepad && (i < maxsize - size); i++) {
        write((uint8_t)pad);
      }

      write(c_str);

      for (size_t i = 0; !prepad && (i < maxsize - size); i++){
        write((uint8_t)pad);
      }

      return maxsize;
    } 
    return write(c_str, maxsize); 
  }

  /**
   * @brief Writes a newline character.
   * @return Number of bytes written.
   */
  virtual uint32_t putln() { return write("\r\n"); }

  /**
   * @brief Writes a character followed by a newline.
   * @param c The character to write.
   * @return Number of bytes written.
   */
  virtual uint32_t writeln(char c) { return (write(c) + putln()); }

  /**
   * @brief Writes a string followed by a newline.
   * @param c_str The string to write.
   * @return Number of bytes written.
   */
  virtual uint32_t writeln(const char *c_str = "") { return (write(c_str) + putln()); }

  /**
   * @brief Writes a string of characters followed by a newline.
   * @param c_str Pointer to the character string.
   * @param size The number of characters to write.
   * @return Number of bytes written.
   */
  virtual uint32_t writeln(const char *c_str, uint32_t size) { return (write(c_str, size) + putln()); }

  /**
   * @brief Writes an integer value as a string.
   * @param val The integer value to write.
   * @return Number of bytes written.
   */
  virtual uint32_t write(int32_t val) {
    char tembuff[25];
    memset(tembuff, 0, 25);
    sprintf(tembuff, "%d", val);
    return write(tembuff);
  }

  /**
   * @brief Writes a long integer value as a string.
   * @param val The long integer value to write.
   * @return Number of bytes written.
   */
  virtual uint32_t write(int64_t val) {
    char tembuff[25];
    memset(tembuff, 0, 25);
    sprintf(tembuff, "%ld", val);
    return write(tembuff);
  }

  /**
   * @brief Writes an unsigned integer value as a string.
   * @param val The unsigned integer value to write.
   * @param hex If true, writes the value in hexadecimal format.
   * @param cap If true, uses uppercase letters for hexadecimal.
   * @return Number of bytes written.
   */
  virtual uint32_t write(uint32_t val, bool hex = false, bool cap = false) {
    char tembuff[25];
    memset(tembuff, 0, 25);
    sprintf(tembuff, hex ? (cap ? "%X" : "%x") : "%u", val);
    return write(tembuff);
  }

  /**
   * @brief Writes a floating-point value as a string.
   * @param val The floating-point value to write.
   * @return Number of bytes written.
   */
  virtual uint32_t write(double val) {
    char tembuff[25];
    memset(tembuff, 0, 25);
    sprintf(tembuff, "%f", val);
    return write(tembuff);
  }

  /**
   * @brief Writes an integer value followed by a newline.
   * @param val The integer value to write.
   * @return Number of bytes written.
   */
  virtual uint32_t writeln(int32_t val) { return (write(val) + putln()); }

  /**
   * @brief Writes a long integer value followed by a newline.
   * @param val The long integer value to write.
   * @return Number of bytes written.
   */
  virtual uint32_t writeln(int64_t val) { return (write(val) + putln()); }

  /**
   * @brief Writes a floating-point value followed by a newline.
   * @param val The floating-point value to write.
   * @return Number of bytes written.
   */
  virtual uint32_t writeln(double val) { return (write(val) + putln()); }

  /**
   * @brief Writes an unsigned integer value followed by a newline.
   * @param val The unsigned integer value to write.
   * @param hex If true, writes the value in hexadecimal format.
   * @param cap If true, uses uppercase letters for hexadecimal.
   * @return Number of bytes written.
   */
  virtual uint32_t writeln(uint32_t val, bool hex = false, bool cap = false) { return (write(val, hex, cap) + putln()); }

  /**
   * @brief Writes a read-only string.
   * @param c_str The string to write.
   * @return Number of bytes written.
   */
  virtual uint32_t write_ro(const char *c_str) { return -1; }

  /**
   * @brief Writes a read-only string followed by a newline.
   * @param c_str The string to write.
   * @return Number of bytes written.
   */
  virtual uint32_t writeln_ro(const char *c_str) { return (write_ro(c_str) + putln()); }

  // Data receiving APIs

  /**
   * @brief Reads a single byte of data.
   * @return The byte read.
   */
  virtual uint8_t read() = 0;

  /**
   * @brief Reads a specified number of bytes into a buffer.
   * @param buf Pointer to the buffer to store the data.
   * @param size The number of bytes to read.
   * @return Number of bytes read.
   */
  virtual uint32_t read(uint8_t *buf, uint32_t size) = 0;

  // Utility APIs

  /**
   * @brief Checks the number of bytes available to read.
   * @return Number of bytes available.
   */
  virtual int32_t available() = 0;

  /**
   * @brief Checks if the connection is active.
   * @return Connection status (1 for connected, 0 for disconnected).
   */
  virtual int8_t connected() = 0;

  /**
   * @brief Checks if the connection is open.
   * @return Connection status (1 for open, 0 for closed).
   */
  virtual int8_t isopen() { return connected(); }

  /**
   * @brief Flushes the I/O buffer.
   */
  virtual void flush() = 0;
};


/**
 * @class iTerminalInterface
 * @brief Interface for terminal-specific operations.
 *
 * The iTerminalInterface class extends the iIOInterface class and provides
 * additional methods for terminal-specific operations such as cursor movement,
 * text color, and background color.
 */
class iTerminalInterface : public iIOInterface
{
public:
  /**
   * @brief Constructor for the iTerminalInterface class.
   *
   * Initializes the iTerminalInterface object.
   */
  iTerminalInterface() {}

  /**
   * @brief Destructor for the iTerminalInterface class.
   *
   * Cleans up resources used by the iTerminalInterface object.
   */
  virtual ~iTerminalInterface() {}

  /**
   * @brief Erases the display and sets the cursor to home position.
   * @param None
   * @return None
   */
  void csi_erase_display()
  {
    write_ro(RODT_ATTR(TERMINAL_ESCAPE_SEQ));
    write_ro(RODT_ATTR("2J"));
    write_ro(RODT_ATTR(TERMINAL_ESCAPE_SEQ));
    write_ro(RODT_ATTR("H"));
  }

  /**
   * @brief Erases in the current line.
   * @param lineerasemode The mode for erasing in the current line.
   * 0: Erase from cursor to end of line
   * 1: Erase from beginning of line to cursor
   * 2: Erase entire line
   * @return None
   */
  void csi_erase_in_line(int32_t lineerasemode)
  {
    write_ro(RODT_ATTR(TERMINAL_ESCAPE_SEQ));
    write(lineerasemode);
    write_ro(RODT_ATTR("K"));
  }

  /**
   * @brief Moves the cursor to the home position (top-left corner).
   * @param None
   * @return None
   */ 
  void csi_cursor_home()
  {
    write_ro(RODT_ATTR(TERMINAL_ESCAPE_SEQ));
    write_ro(RODT_ATTR("H"));
  }

  /**
   * @brief Moves the cursor to a specified position.
   * @param x The x-coordinate (column) to move to.
   * @param y The y-coordinate (row) to move to.
   * @return None
   */
  void csi_cursor_move(uint8_t x, uint8_t y)
  {
    write_ro(RODT_ATTR(TERMINAL_ESCAPE_SEQ));
    write(y);
    write((uint8_t)';');
    write(x);
    write_ro(RODT_ATTR("H"));
  }


  /**
   * @brief Moves the cursor left by a specified number of steps.
   * @param steps The number of steps to move left (default is 1).
   * @return None
   */
  void csi_cursor_move_left(int32_t steps=1)
  {
    if(steps <= 0) return;
    write_ro(RODT_ATTR(TERMINAL_ESCAPE_SEQ));
    write(steps);
    write_ro(RODT_ATTR("D"));
  }


  /**
   * @brief Moves the cursor right by a specified number of steps.
   * @param steps The number of steps to move right (default is 1).
   * @return None
   */
  void csi_cursor_move_right(int32_t steps=1)
  {
    if(steps <= 0) return;
    write_ro(RODT_ATTR(TERMINAL_ESCAPE_SEQ));
    write(steps);
    write_ro(RODT_ATTR("C"));
  }

  /**
   * @brief Set the text color using ANSI escape codes.
   * @param color The color code (0-255) for the text.
   * @return None
   */
  // void csi_set_text_color(uint8_t color)
  // {
  //   write_ro(RODT_ATTR(TERMINAL_ESCAPE_SEQ));
  //   write_ro(RODT_ATTR("38;5;"));
  //   write(color);
  //   write_ro(RODT_ATTR("m"));
  // }

  /**
   * @brief Set the background color using ANSI escape codes.
   * @param color The color code (0-255) for the background.
   * @return None
   */
  // void csi_bg_color(uint8_t color)
  // {
  //   write_ro(RODT_ATTR(TERMINAL_ESCAPE_SEQ));
  //   write_ro(RODT_ATTR("48;5;"));
  //   write(color);
  //   write_ro(RODT_ATTR("m"));
  // }

  /**
   * @brief Resets the text and background color to default.
   * @param None
   * @return None
   */
  void csi_reset_style_color()
  {
    write_ro(RODT_ATTR(TERMINAL_ESCAPE_SEQ));
    write_ro(RODT_ATTR("0m"));
  }

  /**
   * @brief With timestamp will print timestamp first
   * derived class should implement this function to print timestamp
   * @param None
   * @return this
   */
  virtual iTerminalInterface* with_timestamp()
  {
    return this;
  }
};

#endif
