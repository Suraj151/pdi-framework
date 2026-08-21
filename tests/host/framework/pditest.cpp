/******************************** PDI Test **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include "pditest.h"
#include <cstdlib>
#include <ctime>

namespace pditest
{

    static TestCase *s_head = nullptr;
    static TestCase *s_tail = nullptr;
    static TestCase *s_running = nullptr;
    static int s_current_failures = 0;
    static bool s_verbose = false;

    static const char *COLOR_RESET = "\033[0m";
    static const char *COLOR_PASS = "\033[32m";
    static const char *COLOR_FAIL = "\033[31m";
    static const char *COLOR_DIM = "\033[2m";

    void registerTest(TestCase *testcase)
    {
        if (nullptr == s_head)
        {
            s_head = testcase;
            s_tail = testcase;
            return;
        }

        s_tail->next = testcase;
        s_tail = testcase;
    }

    void reportFailure(const char *file, int line, const char *expr, const char *detail)
    {
        s_current_failures++;

        const char *suite = (nullptr != s_running) ? s_running->suite : "?";
        const char *name = (nullptr != s_running) ? s_running->name : "?";

        printf("\n  %sFAIL%s %s.%s\n", COLOR_FAIL, COLOR_RESET, suite, name);
        printf("       at %s:%d\n", file, line);
        printf("       %s\n", expr);
        if (nullptr != detail)
        {
            printf("       %s\n", detail);
        }
    }

    void describeBytes(char *buf, size_t size, const void *data, size_t len)
    {
        const uint8_t *bytes = (const uint8_t *)data;
        size_t used = 0;
        size_t shown = len > 16 ? 16 : len;

        for (size_t i = 0; i < shown && (used + 4) < size; i++)
        {
            used += (size_t)snprintf(buf + used, size - used, "%02x", bytes[i]);
        }

        if (shown < len && (used + 4) < size)
        {
            snprintf(buf + used, size - used, "..");
        }
    }

    /**
     * a filter of "suite" matches every test in it, "suite.name" matches one
     */
    static bool matchesFilter(const TestCase *testcase, const char *filter)
    {
        if (nullptr == filter)
        {
            return true;
        }

        const char *dot = strchr(filter, '.');
        if (nullptr == dot)
        {
            return 0 == strcmp(testcase->suite, filter);
        }

        size_t suitelen = (size_t)(dot - filter);
        if (strlen(testcase->suite) != suitelen)
        {
            return false;
        }
        if (0 != strncmp(testcase->suite, filter, suitelen))
        {
            return false;
        }

        return 0 == strcmp(testcase->name, dot + 1);
    }

    static double elapsedms(const struct timespec &start, const struct timespec &end)
    {
        return ((double)(end.tv_sec - start.tv_sec) * 1000.0) +
               ((double)(end.tv_nsec - start.tv_nsec) / 1000000.0);
    }

    int runAll(int argc, char **argv)
    {
        const char *filter = nullptr;
        bool listonly = false;

        for (int i = 1; i < argc; i++)
        {
            if (0 == strcmp(argv[i], "--filter") && (i + 1) < argc)
            {
                filter = argv[++i];
            }
            else if (0 == strcmp(argv[i], "--list"))
            {
                listonly = true;
            }
            else if (0 == strcmp(argv[i], "--verbose"))
            {
                s_verbose = true;
            }
        }

        if (listonly)
        {
            for (TestCase *t = s_head; nullptr != t; t = t->next)
            {
                printf("%s.%s\n", t->suite, t->name);
            }
            return 0;
        }

        int total = 0;
        int passed = 0;
        int failed = 0;
        const char *lastsuite = nullptr;
        struct timespec runstart;
        struct timespec runend;
        clock_gettime(CLOCK_MONOTONIC, &runstart);

        for (TestCase *t = s_head; nullptr != t; t = t->next)
        {
            if (!matchesFilter(t, filter))
            {
                continue;
            }

            if (nullptr == lastsuite || 0 != strcmp(lastsuite, t->suite))
            {
                printf("\n%s%s%s\n", COLOR_DIM, t->suite, COLOR_RESET);
                lastsuite = t->suite;
            }

            total++;
            s_running = t;
            s_current_failures = 0;

            struct timespec start;
            struct timespec end;
            clock_gettime(CLOCK_MONOTONIC, &start);
            t->fn();
            clock_gettime(CLOCK_MONOTONIC, &end);

            s_running = nullptr;

            if (0 == s_current_failures)
            {
                passed++;
                if (s_verbose)
                {
                    printf("  %sok%s   %s %s(%.2f ms)%s\n", COLOR_PASS, COLOR_RESET, t->name,
                           COLOR_DIM, elapsedms(start, end), COLOR_RESET);
                }
                else
                {
                    printf("  %sok%s   %s\n", COLOR_PASS, COLOR_RESET, t->name);
                }
            }
            else
            {
                failed++;
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &runend);

        printf("\n%s\n", "----------------------------------------------------------");
        if (0 == failed)
        {
            printf("%s%d passed%s, %d total in %.2f ms\n", COLOR_PASS, passed, COLOR_RESET, total,
                   elapsedms(runstart, runend));
        }
        else
        {
            printf("%s%d failed%s, %d passed, %d total in %.2f ms\n", COLOR_FAIL, failed, COLOR_RESET,
                   passed, total, elapsedms(runstart, runend));
        }

        return (0 == failed) ? 0 : 1;
    }

} // namespace pditest
