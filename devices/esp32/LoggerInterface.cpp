/*********************** Device logging Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#include "LoggerInterface.h"

/**
 * LoggerInterface constructor.
 */
LoggerInterface::LoggerInterface() : m_io(nullptr)
{
}

/**
 * LoggerInterface destructor.
 */
LoggerInterface::~LoggerInterface()
{
    m_io = nullptr;
}

/**
 * begin the log config if any
 */
void LoggerInterface::init(iIOInterface *io)
{
    m_io = io;
    if (nullptr != m_io && !m_io->isopen())
    {
        m_io->open(0, 115200);
    }
}

/**
 * log the content
 */
void LoggerInterface::log(logger_type_t log_type, const char *content)
{
    switch (log_type)
    {
    case INFO_LOG:
        LogFmtI("%s", content);
        break;

    case ERROR_LOG:
        LogFmtE("%s", content);
        break;

    case WARNING_LOG:
        LogFmtW("%s", content);
        break;

    case SUCCESS_LOG:
        LogFmtS("%s", content);
        break;

    default:
        break;
    }
}

/**
 * log the info
 */
void LoggerInterface::log_info(const char *info)
{
    if (nullptr != m_io)
    {
        m_io->write(info);
    }
}

/**
 * log the error
 */
void LoggerInterface::log_error(const char *error)
{
    if (nullptr != m_io)
    {
        m_io->write(error);
    }
}

/**
 * log the warning
 */
void LoggerInterface::log_warning(const char *warning)
{
    if (nullptr != m_io)
    {
        m_io->write(warning);
    }
}

/**
 * log the success
 */
void LoggerInterface::log_success(const char *success)
{
    if (nullptr != m_io)
    {
        m_io->write(success);
    }
}

/**
 * log the format
 */
void LoggerInterface::log_format(const char *format, logger_type_t log_type, ...)
{
    if (nullptr == m_io)
    {
        return;
    }

    size_t fmtLen = strlen(format);
    char *fmtLocal = new char[fmtLen+1];
    memset(fmtLocal, 0, fmtLen+1);
    memcpy_P (fmtLocal, format, fmtLen);

    va_list args;
    va_start(args, fmtLocal);

    for (size_t indx = 0; indx < fmtLen; indx++)
    {
        if( fmtLocal[indx] == '%' && indx < (fmtLen-1) )
        {
            ++indx;

            if (fmtLocal[indx] == 'd')
            {
                int i = va_arg(args, int);
                m_io->write((int32_t)i);
            }
            else if (fmtLocal[indx] == 'u')
            {
                unsigned int u = va_arg(args, unsigned int);
                m_io->write((uint32_t)u);
            }
            else if (fmtLocal[indx] == 'x')
            {
                int x = va_arg(args, int);
                m_io->write((uint32_t)x, true);
            }
            else if (fmtLocal[indx] == 'c')
            {
                // A 'char' variable will be promoted to 'int'
                // A character literal in C is already 'int' by itself
                int c = va_arg(args, int);
                m_io->write((char)c);
            }
            else if (fmtLocal[indx] == 'f')
            {
                double d = va_arg(args, double);
                m_io->write(d);
            }
            else if (fmtLocal[indx] == 's')
            {
                char *s = va_arg(args, char*);
                if( nullptr != s )
                {
                    m_io->write(s);
                }
            }
            else
            {
                --indx;
                m_io->write(fmtLocal[indx]);
            }
        }
        else
        {
            m_io->write(fmtLocal[indx]);
        }
    }

    va_end(args);

    delete[] fmtLocal;
}

LoggerInterface __i_logger;
