/************************** Database Factory **********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _DATABASE_FACTORY_
#define _DATABASE_FACTORY_

#include <config/Config.h>
#include <utility/Database.h>
#include <interface/pdi.h>

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
	 * register table with address
	 *
	 * @param   uint16_t	_table_address
	 */
	void register_table(uint16_t _table_address)
	{
		struct_tables _t;
		_t.m_table_address = _table_address;
		_t.m_table_size = sizeof(Table);
		_t.m_instance = this;
		__database.register_table(_t);
	}

	/**
	 * return table in database by their address
	 *
	 * @param   uint16_t  _address
	 * @param   type of database table struct  _object
	 * @return  bool	  status of operation
	 */
	bool get_table(uint16_t _address, Table* _object)
	{
		bool bStatus = false;
		for (uint8_t i = 0; i < __database.m_database_tables.size(); i++)
		{
			if (__database.m_database_tables[i].m_table_address == _address)
			{
				__i_db.loadConfig<Table>(_address, _object);
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
				__i_db.clearConfig<Table>(_address);
				bStatus = true;
				break;
			}
		}
		return bStatus;
	}

	/**
	 * set table in database by their address
	 *
	 * @param   uint16_t  _address
	 * @param   type of database table struct  _object
	 * @return  bool
	 */
	bool set_table(uint16_t _address, Table* _object)
	{
		bool bStatus = false;
		for (uint8_t i = 0; i < __database.m_database_tables.size(); i++)
		{
			if (__database.m_database_tables[i].m_table_address == _address)
			{
				__i_db.saveConfig<Table>(_address, _object);
				bStatus = true;
				break;
			}
		}
		return bStatus;
	}
};

#endif
