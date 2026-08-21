/**************************** IO Interface Tests ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Covers the behaviour iIOInterface and iTerminalInterface provide on top of what
a device implements, which every transport and terminal inherits unchanged.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <StringTerminal.h>
#include <devices/mockdevice/SerialInterface.h>
#include <pditest.h>

#include <unistd.h>

TEST(ioiface, writes_a_single_character)
{
    pditest::StringTerminal terminal;
    terminal.write('x');
    ASSERT_STREQ(terminal.captured().c_str(), "x");
}

TEST(ioiface, writes_a_c_string)
{
    pditest::StringTerminal terminal;
    terminal.write("hello");
    ASSERT_STREQ(terminal.captured().c_str(), "hello");
}

TEST(ioiface, writes_a_bounded_run_of_characters)
{
    pditest::StringTerminal terminal;
    terminal.write("hello world", 5);
    ASSERT_STREQ(terminal.captured().c_str(), "hello");
}

TEST(ioiface, putln_emits_a_carriage_return_and_newline)
{
    pditest::StringTerminal terminal;
    terminal.putln();
    ASSERT_STREQ(terminal.captured().c_str(), "\r\n");
}

TEST(ioiface, writeln_appends_the_line_ending)
{
    pditest::StringTerminal terminal;
    terminal.writeln("line");
    ASSERT_STREQ(terminal.captured().c_str(), "line\r\n");
}

TEST(ioiface, writeln_with_no_argument_emits_only_the_line_ending)
{
    pditest::StringTerminal terminal;
    terminal.writeln();
    ASSERT_STREQ(terminal.captured().c_str(), "\r\n");
}

TEST(ioiface, writes_a_signed_integer)
{
    pditest::StringTerminal terminal;
    terminal.write((int32_t)-4096);
    ASSERT_STREQ(terminal.captured().c_str(), "-4096");
}

TEST(ioiface, writes_an_unsigned_integer)
{
    pditest::StringTerminal terminal;
    terminal.write((uint32_t)4294967295u);
    ASSERT_STREQ(terminal.captured().c_str(), "4294967295");
}

TEST(ioiface, writes_an_unsigned_integer_as_hex)
{
    pditest::StringTerminal terminal;
    terminal.write((uint32_t)0xdeadbeefu, true);
    ASSERT_STREQ(terminal.captured().c_str(), "deadbeef");
}

TEST(ioiface, writes_an_unsigned_integer_as_capital_hex)
{
    pditest::StringTerminal terminal;
    terminal.write((uint32_t)0xdeadbeefu, true, true);
    ASSERT_STREQ(terminal.captured().c_str(), "DEADBEEF");
}

TEST(ioiface, writeln_of_a_number_appends_the_line_ending)
{
    pditest::StringTerminal terminal;
    terminal.writeln((int32_t)7);
    ASSERT_STREQ(terminal.captured().c_str(), "7\r\n");
}

TEST(ioiface, write_pad_pads_after_a_short_value)
{
    pditest::StringTerminal terminal;
    terminal.write_pad("ab", 5);
    ASSERT_STREQ(terminal.captured().c_str(), "ab   ");
}

TEST(ioiface, write_pad_pads_before_when_asked)
{
    pditest::StringTerminal terminal;
    terminal.write_pad("ab", 5, true);
    ASSERT_STREQ(terminal.captured().c_str(), "   ab");
}

TEST(ioiface, write_pad_accepts_a_custom_pad_character)
{
    pditest::StringTerminal terminal;
    terminal.write_pad("ab", 5, false, '.');
    ASSERT_STREQ(terminal.captured().c_str(), "ab...");
}

TEST(ioiface, write_pad_truncates_a_value_wider_than_the_column)
{
    pditest::StringTerminal terminal;
    terminal.write_pad("abcdefgh", 4);
    ASSERT_STREQ(terminal.captured().c_str(), "abcd");
}

TEST(ioiface, write_pad_of_an_exact_width_value_adds_nothing)
{
    pditest::StringTerminal terminal;
    terminal.write_pad("abcd", 4);
    ASSERT_STREQ(terminal.captured().c_str(), "abcd");
}

TEST(ioiface, reads_back_what_was_fed)
{
    pditest::StringTerminal terminal;
    terminal.feed("abc");

    ASSERT_EQ(terminal.read(), (uint8_t)'a');
    ASSERT_EQ(terminal.read(), (uint8_t)'b');
    ASSERT_EQ(terminal.read(), (uint8_t)'c');
}

TEST(ioiface, available_tracks_what_is_left_to_read)
{
    pditest::StringTerminal terminal;
    terminal.feed("abc");

    ASSERT_EQ(terminal.available(), 3);
    terminal.read();
    ASSERT_EQ(terminal.available(), 2);
}

TEST(ioiface, read_string_until_stops_at_the_delimiter)
{
    pditest::StringTerminal terminal;
    pdiutil::string out;
    terminal.feed("name=value");

    terminal.readStringUntil(out, '=');
    ASSERT_STREQ(out.c_str(), "name");
}

TEST(ioiface, read_string_until_can_keep_the_delimiter)
{
    pditest::StringTerminal terminal;
    pdiutil::string out;
    terminal.feed("name=value");

    terminal.readStringUntil(out, '=', true);
    ASSERT_STREQ(out.c_str(), "name=");
}

TEST(ioiface, read_string_until_consumes_everything_when_absent)
{
    pditest::StringTerminal terminal;
    pdiutil::string out;
    terminal.feed("no delimiter here");

    terminal.readStringUntil(out, '#');
    ASSERT_STREQ(out.c_str(), "no delimiter here");
}

TEST(ioiface, read_string_until_honours_the_max_length)
{
    pditest::StringTerminal terminal;
    pdiutil::string out;
    terminal.feed("abcdefghij");

    terminal.readStringUntil(out, '#', false, nullptr, 4);
    ASSERT_STREQ(out.c_str(), "abcd");
}

TEST(ioiface, read_string_until_keeps_a_nul_in_the_payload)
{
    pditest::StringTerminal terminal;
    pdiutil::string out;
    const uint8_t payload[] = {'a', 0x00, 'b', '\n'};
    terminal.feed(payload, sizeof(payload));

    terminal.readStringUntil(out, '\n');
    ASSERT_EQ(out.size(), (size_t)3);
    ASSERT_EQ(out[1], '\0');
}

TEST(ioiface, read_line_strips_the_line_ending)
{
    pditest::StringTerminal terminal;
    pdiutil::string out;
    terminal.feed("first\r\nsecond\r\n");

    terminal.readLine(out);
    ASSERT_STREQ(out.c_str(), "first");

    terminal.readLine(out);
    ASSERT_STREQ(out.c_str(), "second");
}

/**
 * readLine consumes a carriage return then a newline, so it strips a crlf
 * ending. A bare newline is not a terminator and stays in the value. Every
 * caller is http parsing, where crlf is mandated.
 */
TEST(ioiface, read_line_leaves_a_bare_newline_in_the_value)
{
    pditest::StringTerminal terminal;
    pdiutil::string out;
    terminal.feed("solo\n");

    terminal.readLine(out);
    ASSERT_STREQ(out.c_str(), "solo\n");
}

TEST(ioiface, read_line_stops_at_the_end_of_the_available_input)
{
    pditest::StringTerminal terminal;
    pdiutil::string out;
    terminal.feed("trailing");

    terminal.readLine(out);
    ASSERT_STREQ(out.c_str(), "trailing");
}

TEST(ioiface, read_line_clears_the_previous_value)
{
    pditest::StringTerminal terminal;
    pdiutil::string out = "stale";
    terminal.feed("fresh\r\n");

    terminal.readLine(out);
    ASSERT_STREQ(out.c_str(), "fresh");
}

TEST(ioiface, the_yield_hook_runs_while_reading)
{
    static int yields = 0;
    yields = 0;

    pditest::StringTerminal terminal;
    pdiutil::string out;
    terminal.feed("abcd\n");

    terminal.readStringUntil(out, '\n', false, []() { yields++; });
    ASSERT_GT(yields, 0);
}

TEST(ioiface, open_and_close_track_the_connection)
{
    pditest::StringTerminal terminal;
    ASSERT_EQ(terminal.connected(), (int8_t)1);

    terminal.close();
    ASSERT_EQ(terminal.connected(), (int8_t)0);
}

TEST(ioiface, is_secure_defaults_to_false)
{
    pditest::StringTerminal terminal;
    ASSERT_FALSE(terminal.isSecure());
}

TEST(terminal, defaults_to_a_serial_type_and_standard_geometry)
{
    pditest::StringTerminal terminal;
    ASSERT_EQ(terminal.get_terminal_type(), TERMINAL_TYPE_SERIAL);
    ASSERT_EQ(terminal.get_column_width(), (uint16_t)80);
    ASSERT_EQ(terminal.get_row_count(), (uint16_t)24);
}

TEST(terminal, geometry_can_be_changed)
{
    pditest::StringTerminal terminal;
    terminal.set_column_width(132);
    terminal.set_row_count(50);

    ASSERT_EQ(terminal.get_column_width(), (uint16_t)132);
    ASSERT_EQ(terminal.get_row_count(), (uint16_t)50);
}

TEST(terminal, zero_geometry_is_ignored)
{
    pditest::StringTerminal terminal;
    terminal.set_column_width(100);
    terminal.set_column_width(0);
    terminal.set_row_count(30);
    terminal.set_row_count(0);

    ASSERT_EQ(terminal.get_column_width(), (uint16_t)100);
    ASSERT_EQ(terminal.get_row_count(), (uint16_t)30);
}

TEST(terminal, type_round_trips)
{
    pditest::StringTerminal terminal;
    terminal.set_terminal_type(TERMINAL_TYPE_SSH);
    ASSERT_EQ(terminal.get_terminal_type(), TERMINAL_TYPE_SSH);
}

TEST(terminal, erase_display_emits_the_expected_sequence)
{
    pditest::StringTerminal terminal;
    terminal.csi_erase_display();
    ASSERT_STREQ(terminal.captured().c_str(), "\033[2J\033[H");
}

TEST(terminal, cursor_home_emits_the_expected_sequence)
{
    pditest::StringTerminal terminal;
    terminal.csi_cursor_home();
    ASSERT_STREQ(terminal.captured().c_str(), "\033[H");
}

TEST(terminal, erase_in_line_carries_the_mode)
{
    pditest::StringTerminal terminal;
    terminal.csi_erase_in_line(2);
    ASSERT_STREQ(terminal.captured().c_str(), "\033[2K");
}

TEST(terminal, cursor_move_places_row_before_column)
{
    pditest::StringTerminal terminal;
    terminal.csi_cursor_move(10, 5);
    ASSERT_STREQ(terminal.captured().c_str(), "\033[5;10H");
}

TEST(terminal, cursor_moves_emit_their_direction)
{
    pditest::StringTerminal terminal;

    terminal.csi_cursor_move_left(3);
    ASSERT_STREQ(terminal.captured().c_str(), "\033[3D");

    terminal.forget();
    terminal.csi_cursor_move_right(4);
    ASSERT_STREQ(terminal.captured().c_str(), "\033[4C");

    terminal.forget();
    terminal.csi_cursor_move_up(1);
    ASSERT_STREQ(terminal.captured().c_str(), "\033[1A");

    terminal.forget();
    terminal.csi_cursor_move_down(2);
    ASSERT_STREQ(terminal.captured().c_str(), "\033[2B");
}

TEST(terminal, a_cursor_move_of_zero_or_less_emits_nothing)
{
    pditest::StringTerminal terminal;

    terminal.csi_cursor_move_left(0);
    terminal.csi_cursor_move_right(-1);
    terminal.csi_cursor_move_up(0);
    terminal.csi_cursor_move_down(-5);

    ASSERT_STREQ(terminal.captured().c_str(), "");
}

TEST(terminal, reverse_video_and_reset_emit_their_sequences)
{
    pditest::StringTerminal terminal;

    terminal.csi_reverse_video();
    ASSERT_STREQ(terminal.captured().c_str(), "\033[7m");

    terminal.forget();
    terminal.csi_reset_style_color();
    ASSERT_STREQ(terminal.captured().c_str(), "\033[0m");
}

TEST(terminal, device_status_report_emits_its_sequence)
{
    pditest::StringTerminal terminal;
    terminal.csi_dsr();
    ASSERT_STREQ(terminal.captured().c_str(), "\033[6n");
}

/* ------------------------------------------------------------------- flush */

/**
 * Which side a flush acts on. Output-only is the default because the shell
 * flushes after every command, and a flush that also dropped unread input
 * would throw away whatever was typed while that command ran.
 */

static int pipe_with(const char *bytes, int &writeend)
{
    int fds[2];
    if (pipe(fds) != 0) return -1;

    if (bytes && *bytes)
    {
        ssize_t put = ::write(fds[1], bytes, strlen(bytes));
        (void)put;
    }

    writeend = fds[1];
    return fds[0];
}

TEST(flushtype, the_default_leaves_unread_input_alone)
{
    int writeend = -1;
    int readend = pipe_with("typed ahead", writeend);
    ASSERT_TRUE(readend >= 0);

    UARTSerial serial(readend, STDOUT_FILENO);
    ASSERT_TRUE(serial.available() > 0);

    serial.flush();
    ASSERT_TRUE(serial.available() > 0);

    close(readend);
    close(writeend);
}

TEST(flushtype, an_explicit_transmit_flush_leaves_unread_input_alone)
{
    int writeend = -1;
    int readend = pipe_with("typed ahead", writeend);
    ASSERT_TRUE(readend >= 0);

    UARTSerial serial(readend, STDOUT_FILENO);

    serial.flush(FLUSH_TX);
    ASSERT_TRUE(serial.available() > 0);

    close(readend);
    close(writeend);
}

TEST(flushtype, a_receive_flush_discards_unread_input)
{
    int writeend = -1;
    int readend = pipe_with("typed ahead", writeend);
    ASSERT_TRUE(readend >= 0);

    UARTSerial serial(readend, STDOUT_FILENO);
    ASSERT_TRUE(serial.available() > 0);

    serial.flush(FLUSH_RX);
    ASSERT_EQ(serial.available(), 0);

    close(readend);
    close(writeend);
}

TEST(flushtype, flushing_both_discards_unread_input)
{
    int writeend = -1;
    int readend = pipe_with("typed ahead", writeend);
    ASSERT_TRUE(readend >= 0);

    UARTSerial serial(readend, STDOUT_FILENO);

    serial.flush(FLUSH_ALL);
    ASSERT_EQ(serial.available(), 0);

    close(readend);
    close(writeend);
}

TEST(flushtype, the_side_tests_only_answer_for_the_side_asked_about)
{
    ASSERT_TRUE(IsFlushTx(FLUSH_TX));
    ASSERT_FALSE(IsFlushRx(FLUSH_TX));

    ASSERT_TRUE(IsFlushRx(FLUSH_RX));
    ASSERT_FALSE(IsFlushTx(FLUSH_RX));

    ASSERT_TRUE(IsFlushTx(FLUSH_ALL));
    ASSERT_TRUE(IsFlushRx(FLUSH_ALL));
}

/**
 * A value the enum does not define yet must not be treated as either side, so
 * that adding one later cannot silently widen what an existing flush does.
 */
TEST(flushtype, an_unknown_value_acts_on_neither_side)
{
    ASSERT_FALSE(IsFlushTx(FLUSH_MAX));
    ASSERT_FALSE(IsFlushRx(FLUSH_MAX));
    ASSERT_FALSE(IsFlushTx(FLUSH_MAX + 7));
    ASSERT_FALSE(IsFlushRx(FLUSH_MAX + 7));
}
