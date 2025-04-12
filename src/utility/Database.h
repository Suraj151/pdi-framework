/****************************** Database **************************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The Database utility provides an abstraction layer for managing database tables
within the PDI stack. It allows for the registration, initialization, and 
management of database tables, including clearing and retrieving table data.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _DATABASE_HANDLER_
#define _DATABASE_HANDLER_

#include <utility/Utility.h>

/**
 * @brief Defines the maximum number of database tables.
 */
#ifdef MAX_DB_TABLES
#define MAX_TABLES MAX_DB_TABLES
#else
#define MAX_TABLES 20
#endif

/**
 * @class DatabaseTableAbstractLayer
 * @brief Abstract layer for database tables.
 *
 * This class provides an interface for managing individual database tables.
 * It allows for the creation, initialization, and clearing of tables. The
 * class also tracks the total number of table instances and ensures that the
 * number of tables does not exceed the defined maximum.
 */
class DatabaseTableAbstractLayer
{

public:
    friend class Database;

    /**
     * @brief Constructor for the DatabaseTableAbstractLayer class.
     *
     * Registers the table instance and increments the total instance count,
     * ensuring the number of instances does not exceed the maximum limit.
     */
    DatabaseTableAbstractLayer()
    {
        if (DatabaseTableAbstractLayer::m_total_instances < MAX_TABLES)
        {
            DatabaseTableAbstractLayer::m_instances[DatabaseTableAbstractLayer::m_total_instances] = this;
            DatabaseTableAbstractLayer::m_total_instances++;
        }
    }

    /**
     * @brief Destructor for the DatabaseTableAbstractLayer class.
     */
    ~DatabaseTableAbstractLayer()
    {
    }

    /**
     * @brief Initializes the table.
     *
     * This method must be overridden by derived classes to provide specific
     * initialization logic for the table.
     */
    virtual void boot() = 0;

    /**
     * @brief Clears the table data.
     *
     * This method must be overridden by derived classes to provide specific
     * logic for clearing the table data.
     */
    virtual void clear() = 0;

    /**
     * @var DatabaseTableAbstractLayer* m_instances[MAX_TABLES]
     * @brief Array of pointers to table instances.
     */
    static DatabaseTableAbstractLayer *m_instances[MAX_TABLES];

    /**
     * @var int m_total_instances
     * @brief Total number of table instances.
     */
    static int m_total_instances;
};

/**
 * @struct struct_tables
 * @brief Represents a database table structure.
 *
 * This structure contains metadata about a database table, including its
 * address, size, and associated instance.
 */
struct struct_tables
{
    uint16_t m_table_address; ///< Address of the table in memory.
    uint16_t m_table_size;    ///< Size of the table in bytes.
    DatabaseTableAbstractLayer *m_instance; ///< Pointer to the table instance.
};

/**
 * @class Database
 * @brief Manages database tables.
 *
 * The Database class provides methods for initializing the database, registering
 * tables, retrieving the last table, and clearing all tables. It maintains a
 * collection of registered tables and enforces a maximum database size.
 */
class Database
{

public:
    /**
     * @var pdiutil::vector<struct_tables> m_database_tables
     * @brief Vector of registered database tables.
     */
    pdiutil::vector<struct_tables> m_database_tables;

    /**
     * @brief Constructor for the Database class.
     */
    Database();

    /**
     * @brief Destructor for the Database class.
     */
    ~Database();

    /**
     * @brief Initializes the database with a specified size.
     * @param _size The maximum size of the database in bytes.
     */
    void init_database(uint32_t _size);

    /**
     * @brief Registers a new table in the database.
     * @param _table The table structure to register.
     * @return True if the table was successfully registered, false otherwise.
     */
    bool register_table(struct_tables &_table);

    /**
     * @brief Retrieves the last registered table.
     * @return The structure of the last registered table.
     */
    struct_tables get_last_table(void);

    /**
     * @brief Clears all tables in the database.
     */
    void clear_all(void);

    /**
     * @var uint32_t m_max_db_size
     * @brief Maximum size of the database in bytes.
     */
    uint32_t m_max_db_size;
};

/**
 * @brief Global instance of the Database class.
 *
 * This instance is used to manage database operations throughout the PDI stack.
 */
extern Database __database;

#endif
