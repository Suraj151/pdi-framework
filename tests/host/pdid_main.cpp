/********************************* pdid **************************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The whole stack as a host process. Same entry points the sketch uses, so what
runs here is what runs on a board.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <PdiStack.h>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <unistd.h>

static volatile sig_atomic_t s_running = 1;

static void onSignal(int signum)
{
    s_running = 0;
}

static void usage()
{
    printf("pdid - the pdi stack as a host process\n\n");
    printf("  --nvm <file>        keep the config store in this file\n");
    printf("  --fs-image <file>   keep the filesystem image in this file\n");
    printf("  --seed <n>          seed the pseudo random source\n");
    printf("  --epoch <n>         start with the clock already synced to this time\n");
    printf("  --ssid <name>       associate with this network at boot\n");
    printf("  --headless          do not read the terminal from stdin\n");
    printf("  --help              this text\n");
}

int main(int argc, char **argv)
{
    const char *nvmpath = nullptr;
    const char *fsimagepath = nullptr;
    const char *ssid = nullptr;
    uint32_t seed = 0;
    uint32_t epoch = 0;
    bool headless = false;

    for (int i = 1; i < argc; i++)
    {
        if (0 == strcmp(argv[i], "--nvm") && (i + 1) < argc)
        {
            nvmpath = argv[++i];
        }
        else if (0 == strcmp(argv[i], "--fs-image") && (i + 1) < argc)
        {
            fsimagepath = argv[++i];
        }
        else if (0 == strcmp(argv[i], "--seed") && (i + 1) < argc)
        {
            seed = (uint32_t)strtoul(argv[++i], nullptr, 10);
        }
        else if (0 == strcmp(argv[i], "--epoch") && (i + 1) < argc)
        {
            epoch = (uint32_t)strtoul(argv[++i], nullptr, 10);
        }
        else if (0 == strcmp(argv[i], "--ssid") && (i + 1) < argc)
        {
            ssid = argv[++i];
        }
        else if (0 == strcmp(argv[i], "--headless"))
        {
            headless = true;
        }
        else if (0 == strcmp(argv[i], "--help"))
        {
            usage();
            return 0;
        }
        else
        {
            printf("unrecognised argument: %s\n\n", argv[i]);
            usage();
            return 2;
        }
    }

    signal(SIGINT, onSignal);
    signal(SIGTERM, onSignal);

    // storage and nvm are attached before init, so a persisted image is the one
    // the stack mounts rather than something restored underneath it
    if (nullptr != fsimagepath)
    {
        __i_storage.attachBackingFile(fsimagepath);
    }
    if (nullptr != nvmpath)
    {
        __i_db.attachBackingFile(nvmpath);
    }
    if (0 != seed)
    {
        __i_dvc_ctrl.seedRandom(seed);
    }
    if (0 != epoch)
    {
        __i_ntp.set_ntp_time(epoch);
    }

    if (headless)
    {
        __serial_uart.setDescriptors(-1, STDOUT_FILENO);
    }

    PdiStack.initialize();

    if (nullptr != ssid)
    {
        char ssidbuf[33];
        memset(ssidbuf, 0, sizeof(ssidbuf));
        strncpy(ssidbuf, ssid, sizeof(ssidbuf) - 1);
        __i_wifi.begin(ssidbuf);
    }

    while (s_running)
    {
        PdiStack.serve();
    }

    if (nullptr != fsimagepath)
    {
        __i_storage.flush();
    }

    printf("\npdid stopped\n");
    return 0;
}
