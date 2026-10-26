/******************************* FatFs Wrapper ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Builds the vendored FatFs sources, set up by fatfs/ffconf.h, with the code
conversions for code page 437 taken from FatFsUnicode.c, whose tables sit in
flash.

Author          : Suraj I.
created Date    : 21st Oct 2026
******************************************************************************/

#include "fatfs/ff.c"
#if FF_USE_LFN != 0 && FF_CODE_PAGE == 437
#include "FatFsUnicode.c"
#else
#include "fatfs/ffunicode.c"
#endif
