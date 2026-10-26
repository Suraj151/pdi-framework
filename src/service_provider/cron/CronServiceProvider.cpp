/**************************** Cron Service ************************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 7th Sep 2026
******************************************************************************/

#include "CronServiceProvider.h"

#ifdef ENABLE_CRON_SERVICE

#include <utility/EventUtil.h>
#include <utility/StringOperations.h>
#include <utility/DataTypeConversions.h>
#include <helpers/ConfigHelper.h>
#include <service_provider/cmd/ScriptRunner.h>
#include <service_provider/time/TimeServiceProvider.h>

#ifdef ENABLE_AUTH_SERVICE
#include <service_provider/user/UserStoreService.h>
#endif

CronServiceProvider __cron_service;

CronServiceProvider::CronServiceProvider() : ServiceProvider(SERVICE_CRON, RODT_ATTR("Cron")),
                                             m_scanning(false),
                                             m_listener_id(EVENT_LISTENER_ID_INVALID)
{
}

CronServiceProvider::~CronServiceProvider()
{
}

/**
 * Reads a decimal out of a field item, false unless every character of it is a
 * digit, so a spec that is not a number cannot read as zero.
 */
bool CronServiceProvider::parseNumber(const pdiutil::string &text, uint32_t &out)
{
  return ParseUint32(text.c_str(), (uint16_t)text.size(), out);
}

/**
 * Whether a field spec covers the value, over the standard grammar of
 * comma-separated items, each a star, a number or a range, with an optional
 * step. Out-of-range and malformed specs match nothing.
 */
bool CronServiceProvider::fieldMatches(const pdiutil::string &spec, uint8_t value,
                                       uint8_t lo, uint8_t hi)
{
  if( spec.empty() || value < lo || value > hi ){
    return false;
  }

  pdiutil::string::size_type at = 0;

  while( at <= spec.size() ){

    pdiutil::string::size_type end = at;
    while( end < spec.size() && CRON_LIST_CHAR != spec[end] ) end++;

    pdiutil::string item = spec.substr(at, end - at);
    at = end + 1;

    if( item.empty() ){
      continue;
    }

    uint32_t step = 1;
    pdiutil::string::size_type slash = 0;
    while( slash < item.size() && CRON_STEP_CHAR != item[slash] ) slash++;

    if( slash < item.size() ){
      pdiutil::string stepstr = item.substr(slash + 1);
      if( !parseNumber(stepstr, step) || 0 == step ) continue;
      item = item.substr(0, slash);
    }

    uint32_t first = lo;
    uint32_t last = hi;

    if( item.empty() ){
      continue;
    }

    if( 1 == item.size() && CRON_ANY_CHAR == item[0] ){

      first = lo;
      last = hi;
    }else{

      pdiutil::string::size_type dash = 0;
      while( dash < item.size() && CRON_RANGE_CHAR != item[dash] ) dash++;

      if( dash < item.size() ){

        pdiutil::string lostr = item.substr(0, dash);
        pdiutil::string histr = item.substr(dash + 1);

        if( !parseNumber(lostr, first) || !parseNumber(histr, last) ) continue;
      }else{

        if( !parseNumber(item, first) ) continue;
        last = first;
      }
    }

    if( first < lo || last > hi || first > last ){
      continue;
    }

    if( value < first || value > last ){
      continue;
    }

    if( 0 == ((uint32_t)value - first) % step ){
      return true;
    }
  }

  return false;
}

/**
 * Reads one table row into the five field specs and the command, false when
 * the line is blank, a comment, or does not carry all of them.
 */
bool CronServiceProvider::parseRow(const pdiutil::string &line, pdiutil::string *fields,
                                   pdiutil::string &command)
{
  command.clear();

  for( uint8_t i = 0; i < CRON_FIELD_COUNT; i++ ){
    fields[i].clear();
  }

  pdiutil::string::size_type at = 0;

  while( at < line.size() && __is_blank(line[at]) ) at++;

  if( at >= line.size() || CRON_COMMENT_CHAR == line[at] ){
    return false;
  }

  for( uint8_t i = 0; i < CRON_FIELD_COUNT; i++ ){

    pdiutil::string::size_type end = at;
    while( end < line.size() && !__is_blank(line[end]) ) end++;

    if( end == at ){
      return false;
    }

    fields[i] = line.substr(at, end - at);

    at = end;
    while( at < line.size() && __is_blank(line[at]) ) at++;
  }

  if( at >= line.size() ){
    return false;
  }

  command = line.substr(at);

  return !command.empty();
}

/**
 * Writes an empty table carrying only its column names when none is there, so
 * there is always a file to edit rather than one to know how to create.
 */
bool CronServiceProvider::ensureTableFile()
{
  pdiutil::string table = CHARPTR_WRAP(CRONTAB_FILE_PATH);
  pdiutil::string header = CHARPTR_WRAP(CRONTAB_FILE_HEADER);
  pdiutil::vector<config_kv_t> nodefaults;

  return ensureConfigFile(table.c_str(), nodefaults, header.c_str(), CRONTAB_FILE_PERMS);
}

/**
 * Whether every field of a row covers the instant given.
 */
bool CronServiceProvider::rowMatches(const pdiutil::string *fields, const datetime_t &now)
{
  return fieldMatches(fields[CRON_FIELD_MINUTE], now.m_minute, 0, 59) &&
         fieldMatches(fields[CRON_FIELD_HOUR], now.m_hour, 0, 23) &&
         fieldMatches(fields[CRON_FIELD_DOM], now.m_day, 1, 31) &&
         fieldMatches(fields[CRON_FIELD_MONTH], now.m_month, 1, 12) &&
         fieldMatches(fields[CRON_FIELD_DOW], now.m_weekday, 0, 6);
}

/**
 * Walks the table once against the instant given, running whatever matches.
 * Public so the shell can ask what would run without waiting for the minute.
 */
uint8_t CronServiceProvider::runDueJobs(const datetime_t &now, bool dryrun,
                                        iTerminalInterface *terminal)
{
  if( !now.m_valid ){
    return 0;
  }

  pdiutil::string table = CHARPTR_WRAP(CRONTAB_FILE_PATH);

  if( !__i_fs.isFileExist(table.c_str()) ){
    return 0;
  }

  uint8_t taken = 0;
  uint8_t fired = 0;

  for( int32_t n = 0; taken < CRON_MAX_ENTRIES; n++ ){

    pdiutil::string line;

    __i_dvc_ctrl.yield();

    if( __i_fs.readLineInFile(table.c_str(), n, line, nullptr, [](void){
          __i_dvc_ctrl.yield();
        }) < 0 ){
      break;
    }

    pdiutil::string fields[CRON_FIELD_COUNT];
    pdiutil::string command;

    if( !parseRow(line, fields, command) ){
      continue;
    }

    taken++;

    if( !rowMatches(fields, now) ){
      continue;
    }

    fired++;

    if( dryrun ){

      if( nullptr != terminal ){
        terminal->writeln(command.c_str());
      }
      continue;
    }

#ifdef ENABLE_AUTH_SERVICE
    ScriptRunner::runDetachedLine(command.c_str(), USER_STORE_ROOT_UID);
#else
    ScriptRunner::runDetachedLine(command.c_str(), 0);
#endif
  }

  return fired;
}

bool CronServiceProvider::initService(void *arg)
{
  ensureTableFile();

  m_listener_id = __utl_event.add_event_listener(EVENT_TIME_MINUTE, [](void *e){

    if( nullptr == e ){
      return;
    }

    datetime_t *now = (datetime_t *)e;

    if( !now->m_valid || __cron_service.m_scanning ){
      return;
    }

    __cron_service.m_scanning = true;
    __cron_service.runDueJobs(*now);
    __cron_service.m_scanning = false;
  });

  iTerminalInterface *line = serviceBootLine();
  if( nullptr != line ){
    line->write_ro(RODT_ATTR("reading "));
    line->write_ro(RODT_ATTR(CRONTAB_FILE_PATH));
    line->write_ro(RODT_ATTR(", up to "));
    line->write((uint32_t)CRON_MAX_ENTRIES);
    line->writeln_ro(RODT_ATTR(" jobs"));
  }

  return ServiceProvider::initService(arg);
}

bool CronServiceProvider::stopService()
{
  __utl_event.remove_event_listener(m_listener_id);

  return ServiceProvider::stopService();
}

void CronServiceProvider::resetServiceState()
{
  m_scanning = false;
  m_listener_id = EVENT_LISTENER_ID_INVALID;
}

/**
 * print the service configuration to the terminal
 */
void CronServiceProvider::printConfigToTerminal(iTerminalInterface *terminal)
{
  if( nullptr == terminal ){
    return;
  }

  terminal->write_ro(RODT_ATTR("table: "));
  terminal->writeln_ro(RODT_ATTR(CRONTAB_FILE_PATH));
  terminal->write_ro(RODT_ATTR("clock: "));
  terminal->writeln_ro(__time_service.isTimeValid() ?
                       RODT_ATTR("valid, jobs run") :
                       RODT_ATTR("not valid, jobs held"));
}

#endif
