/******************************** Fuzz JSON ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The json value reader every remote payload goes through: gpio and serial
commands from a broker, the iot server's configuration, the firmware version a
server publishes. The key is matched as a whole string token and the value is
copied into a caller's fixed buffer, so both the token walk and the copy bound
are reached with the text, key and buffer size all chosen by the input.

Author          : Suraj I.
created Date    : 9th Oct 2026
******************************************************************************/

#include <FuzzCommon.h>
#include <utility/StringOperations.h>

namespace
{
    const char *const KEYS[] = {"data", "D1", "D10", "a0", "mode", "val", "cmp", "latest", "did", "channelhost", "", "\""};
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    pdifuzz::FuzzInput input(data, size);

    const char *key = KEYS[input.pick(sizeof(KEYS) / sizeof(KEYS[0]))];
    uint8_t width = input.pick(64) + 1;

    pdiutil::string text((const char *)input.rest(), input.remaining());

    char *value = new char[width];
    __get_from_json(text.c_str(), key, value, width);
    delete[] value;

    pdiutil::string escaped;
    __append_json_escaped(escaped, text.c_str());

    return 0;
}
