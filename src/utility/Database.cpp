/****************************** Database **************************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#include "Database.h"

// Initialize static members of DatabaseTableAbstractLayer
int DatabaseTableAbstractLayer::m_total_instances = 0;
DatabaseTableAbstractLayer *DatabaseTableAbstractLayer::m_instances[MAX_TABLES] = {nullptr};

/**
 * @brief Constructor for the Database class.
 *
 * Initializes the database object and sets the maximum database size to 0.
 */
Database::Database() : m_max_db_size(0)
{
}

/**
 * @brief Destructor for the Database class.
 *
 * Cleans up resources used by the database object.
 */
Database::~Database()
{
}

/**
 * @brief Initializes the database with a specified size.
 *
 * This method sets the maximum size of the database and reserves memory for
 * the database tables. It also initializes all registered database table
 * instances by calling their `boot` method.
 *
 * @param _size The maximum size of the database in bytes.
 */
void Database::init_database(uint32_t _size)
{
    m_max_db_size = _size;
    this->m_database_tables.reserve(MAX_TABLES);

    for (size_t i = 0; i < DatabaseTableAbstractLayer::m_total_instances; i++)
    {
        DatabaseTableAbstractLayer::m_instances[i]->boot();
    }
}

/**
 * @brief Clears all tables in the database.
 *
 * This method iterates through all registered database tables and calls their
 * `clear` method to reset their data.
 */
void Database::clear_all()
{
    for (uint8_t i = 0; i < this->m_database_tables.size(); i++)
    {
        if (nullptr != this->m_database_tables[i].m_instance)
        {
            this->m_database_tables[i].m_instance->clear();
        }
    }
}

/**
 * @brief Registers a new table in the database.
 *
 * This method adds a new table to the database if its address and size do not
 * overlap with existing tables and if it fits within the maximum database size.
 *
 * @param _table The table structure to register.
 * @return True if the table was successfully registered, false otherwise.
 */
bool Database::register_table(struct_tables &_table)
{
    struct_tables _last = this->get_last_table();
    if (
        (_last.m_table_address + _last.m_table_size + 2) < _table.m_table_address &&
        (_table.m_table_address + _table.m_table_size + 2) < m_max_db_size)
    {
        this->m_database_tables.push_back(_table);
        return true;
    }
    return false;
}

/**
 * @brief Retrieves the last registered table from the database.
 *
 * This method finds and returns the table with the highest address in the
 * database. If no tables are registered, it returns an empty table structure.
 *
 * @return The structure of the last registered table.
 */
struct_tables Database::get_last_table()
{
    struct_tables _last;
    uint8_t _last_add = 0;
    memset(&_last, 0, sizeof(struct_tables));
    for (uint8_t i = 1; i < this->m_database_tables.size(); i++)
    {
        if (this->m_database_tables[_last_add].m_table_address < this->m_database_tables[i].m_table_address)
        {
            _last_add = i;
        }
    }
    if (_last_add != 0)
        return this->m_database_tables[_last_add];
    return _last;
}

/**
 * @brief Global instance of the Database class.
 *
 * This instance is used to manage database operations throughout the PDI stack.
 */
Database __database;
