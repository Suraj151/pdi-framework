/******************************** PDI Test **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include "pditest.h"
#include <service_provider/ServiceProvider.h>

int main(int argc, char **argv)
{
    int failures = pditest::runAll(argc, argv);
    ServiceProvider::setTerminal(nullptr);
    return failures;
}
