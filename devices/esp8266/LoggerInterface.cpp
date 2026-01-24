/*********************** Device logging Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#include "LoggerInterface.h"
#include "SerialInterface.h"

/**
 * LoggerInterface constructor.
 */
LoggerInterface::LoggerInterface()
{
    // Serial.begin(115200);
}

/**
 * LoggerInterface destructor.
 */
LoggerInterface::~LoggerInterface()
{
}

/**
 * begin the log config if any
 */
void LoggerInterface::init()
{
    Serial.begin(115200);
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
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    __serial_uart.m_mutex.lock();
    #endif
    Serial.print(info);
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    __serial_uart.m_mutex.unlock();
    #endif
}

/**
 * log the error
 */
void LoggerInterface::log_error(const char *error)
{
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    __serial_uart.m_mutex.lock();
    #endif
    Serial.print(error);
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    __serial_uart.m_mutex.unlock();
    #endif
}

/**
 * log the warning
 */
void LoggerInterface::log_warning(const char *warning)
{
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    __serial_uart.m_mutex.lock();
    #endif
    Serial.print(warning);
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    __serial_uart.m_mutex.unlock();
    #endif
}

/**
 * log the success
 */
void LoggerInterface::log_success(const char *success)
{
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    __serial_uart.m_mutex.lock();
    #endif
    Serial.print(success);
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    __serial_uart.m_mutex.unlock();
    #endif
}

/**
 * log the format
 */
void LoggerInterface::log_format(const char *format, logger_type_t log_type, ...)
{
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    __serial_uart.m_mutex.lock();
    #endif

    int fmtLen = strlen(format); 
    char *fmtLocal = new char[fmtLen+1];
    memset(fmtLocal, 0, fmtLen+1);
    memcpy_P (fmtLocal, format, fmtLen);

    va_list args;
    va_start(args, fmtLocal);

    for (int indx = 0; indx < fmtLen; indx++)
    {
        if( fmtLocal[indx] == '%' && indx < (fmtLen-1) )
        {
            ++indx;

            if (fmtLocal[indx] == 'd')
            {
                int i = va_arg(args, int);
                Serial.print(i);
            }
            else if (fmtLocal[indx] == 'x')
            {
                int x = va_arg(args, int);
                Serial.print(x, HEX);
            }
            else if (fmtLocal[indx] == 'c')
            {
                // A 'char' variable will be promoted to 'int'
                // A character literal in C is already 'int' by itself
                int c = va_arg(args, int);
                Serial.print((char)c);
            }
            else if (fmtLocal[indx] == 'f')
            {
                double d = va_arg(args, double);
                Serial.print(d);
            }
            else if (fmtLocal[indx] == 's')
            {
                char *s = va_arg(args, char*);
                if( nullptr != s )
                {
                    Serial.print(s);
                }
            }
            else
            {
                --indx;
                Serial.print(fmtLocal[indx]);
            }
        }
        else
        {
            Serial.print(fmtLocal[indx]);
        }
    }
    
    va_end(args);

    delete[] fmtLocal;

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    __serial_uart.m_mutex.unlock();
    #endif
}

LoggerInterface __i_logger;