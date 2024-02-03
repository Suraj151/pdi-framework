/****************************** Database **************************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _DATABASE_HANDLER_
#define _DATABASE_HANDLER_

#include <database/eeprom/EepromDatabase.h>

/**
 * define database tables max limit
 */
#define MAX_TABLES 20

class DatabaseTableAbstractLayer
{

public:
	friend class Database;

	/**
	 * DatabaseTableAbstractLayer constructor
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
	 * DatabaseTableAbstractLayer destructor
	 */
	~DatabaseTableAbstractLayer()
	{
	}

	/**
	 * override boot
	 */
	virtual void boot() = 0;
	/**
	 * override clear
	 */
	virtual void clear() = 0;

	/**
	 * @array	DatabaseTableAbstractLayer* m_instances
	 */
	static DatabaseTableAbstractLayer *m_instances[MAX_TABLES];
	/**
	 * @var	int m_total_instances
	 */
	static int m_total_instances;
};

/**
 * define table structure here
 */
struct struct_tables
{
	uint16_t m_table_address;
	uint16_t m_table_size;
	// void *m_default_table;
	DatabaseTableAbstractLayer *m_instance;
};

/**
 * Database class
 */
class Database
{

public:
	std::vector<struct_tables> m_database_tables;

	Database()
	{
	}
	~Database()
	{
	}

	void init_database(uint16_t _size = DATABASE_MAX_SIZE);
	bool register_table(struct_tables &_table);
	struct_tables get_last_table(void);
	void clear_all(void);
};

extern Database __database;
#endif
