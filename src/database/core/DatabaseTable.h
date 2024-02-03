/************************** Database Factory **********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _DATABASE_FACTORY_
#define _DATABASE_FACTORY_

#include "Database.h"

/**
 * DatabaseTable class
 */
template <class Table>
class DatabaseTable : public DatabaseTableAbstractLayer
{

public:
	/**
	 * DatabaseTable constructor
	 */
	DatabaseTable()
	{
	}

	/**
	 * DatabaseTable destructor
	 */
	~DatabaseTable()
	{
	}

	/**
	 * register table with address and default copy
	 *
	 * @param   uint16_t	_table_address
	 * @param   const Table *_default_table_copy
	 */
	void register_table(uint16_t _table_address)
	{
		struct_tables _t;
		_t.m_table_address = _table_address;
		_t.m_table_size = sizeof(Table);
		// _t.m_default_table = (void *)_default_table_copy;
		_t.m_instance = this;
		__database.register_table(_t);
	}

	/**
	 * return table in database by their address
	 *
	 * @param   uint16_t  _address
	 * @return  bool	  status of operation
	 */
	bool get_table(Table* _object, uint16_t _address)
	{
		bool bStatus = false;
		for (uint8_t i = 0; i < __database.m_database_tables.size(); i++)
		{
			if (__database.m_database_tables[i].m_table_address == _address)
			{
				loadConfig(_object, _address);
				bStatus = true;
				break;
			}
		}
		return bStatus;
	}

	/**
	 * clear table in database by their address
	 *
	 * @param   uint16_t  _address
	 * @return  bool
	 */
	bool clear_table(uint16_t _address)
	{
		bool bStatus = false;
		for (uint8_t i = 0; i < __database.m_database_tables.size(); i++)
		{
			if (__database.m_database_tables[i].m_table_address == _address)
			{
				clearConfigs<Table>(_address);
				bStatus = true;
				break;
			}
		}
		return bStatus;
	}

	/**
	 * set table in database by their address
	 *
	 * @param   type of database table struct  _object
	 * @param   uint16_t  _address
	 * @return  bool
	 */
	bool set_table(Table* _object, uint16_t _address)
	{
		bool bStatus = false;
		for (uint8_t i = 0; i < __database.m_database_tables.size(); i++)
		{
			if (__database.m_database_tables[i].m_table_address == _address)
			{
				saveConfig(_object, _address);
				bStatus = true;
				break;
			}
		}
		return bStatus;
	}
};

#endif
