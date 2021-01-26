/*
 * @File: SqliteData.cpp
 * @Author: hzh
 * @Date: 2021-1-10 14:32
 *
 */

#include <stdio.h>
#include <sqlite3.h>
#include "SqliteData.h"
#include <osgEarth/Common>

#include <osgEarth/StringUtils>


using namespace osgEarth;
using namespace osgEarth::Util;

#define LC "[viewer] "


SqliteData::SqliteData():
	_mutex("Shadow Matching(OE)")
{
	_fullFilename = "E:/Proj_data/TMS/smmap.db";
	_database = NULL;
}



SqliteData::~SqliteData()
{
}

bool SqliteData::procdata()
{
	bool readWrite = true;
	int flags = readWrite
		? (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX)
		: (SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX);

	sqlite3* db = (sqlite3*)_database;
	sqlite3** dbptr = (sqlite3**)&_database;

	int rc = sqlite3_open_v2(_fullFilename.c_str(), dbptr, flags, 0L);
	if (rc != 0)
	{
		OE_WARN << LC << "Database \"" << _fullFilename << "\": " << sqlite3_errmsg(db) << std::endl;
		return false;
	}

	createTable();

	return false;
}

bool osgEarth::SqliteData::createTable()
{
	sqlite3* db = (sqlite3*)_database;


	std::string query =
		"CREATE TABLE IF NOT EXISTS metadata (" \
		" name text PRIMARY KEY," \
		" value text);";

	if (SQLITE_OK != sqlite3_exec(db, query.c_str(), 0L, 0L, 0L))
	{
		OE_WARN << LC << "Failed to create table [metadata]" << sqlite3_errmsg(db) << std::endl;
		return false;
	}

	query =
		"CREATE TABLE IF NOT EXISTS shadowmap (" \
		" longitude double," \
		" latitude double," \
		" smmap text);";

	if (SQLITE_OK != sqlite3_exec(db, query.c_str(), 0L, 0L, 0L))
	{
		OE_WARN << LC << "Failed to create table [shadowmap]" << sqlite3_errmsg(db) << std::endl;
		return false;
	}

	query =
		"CREATE TABLE IF NOT EXISTS map (" \
		" longitude double," \
		" latitude double," \
		" azimuth float," \
		" elevation float);";
		
	if (SQLITE_OK != sqlite3_exec(db, query.c_str(), 0L, 0L, 0L))
	{
		OE_WARN << LC << "Failed to create table [tiles]: " << sqlite3_errmsg(db) << std::endl;
		//sqlite3_free(errorMsg);
		return false;
	}

	return true;
}

bool osgEarth::SqliteData::putMetadata(const std::string & name, const std::string & value)
{
	Threading::ScopedMutexLock exclusiveLock(_mutex);

	sqlite3* database = (sqlite3*)_database;

	// prep the insert statement.
	sqlite3_stmt* insert = 0L;
	std::string query = Stringify() << "INSERT OR REPLACE INTO metadata (name,value) VALUES (?,?)";
	if (SQLITE_OK != sqlite3_prepare_v2(database, query.c_str(), -1, &insert, 0L))
	{
		OE_WARN << LC << "Failed to prepare SQL: " << query << "; " << sqlite3_errmsg(database) << std::endl;
		return false;
	}

	// bind the values:
	if (SQLITE_OK != sqlite3_bind_text(insert, 1, name.c_str(), name.length(), SQLITE_STATIC))
	{
		OE_WARN << LC << "Failed to bind text: " << query << "; " << sqlite3_errmsg(database) << std::endl;
		return false;
	}
	if (SQLITE_OK != sqlite3_bind_text(insert, 2, value.c_str(), value.length(), SQLITE_STATIC))
	{
		OE_WARN << LC << "Failed to bind text: " << query << "; " << sqlite3_errmsg(database) << std::endl;
		return false;
	}

	// execute the sql. no idea what a good return value should be :/
	sqlite3_step(insert);
	sqlite3_finalize(insert);

	return true;
}


bool osgEarth::SqliteData::putMapData(double lon, double lat, float amuzith, float elevation)
{
	Threading::ScopedMutexLock exclusiveLock(_mutex);

	sqlite3* database = (sqlite3*)_database;

	// Prep the insert statement:
	sqlite3_stmt* insert = NULL;
	std::string query = "INSERT OR REPLACE INTO map (longitude, latitude, azimuth, elevation) VALUES (?, ?, ?, ?)";
	int rc = sqlite3_prepare_v2(database, query.c_str(), -1, &insert, 0L);
	if (rc != SQLITE_OK)
	{
		OE_WARN << LC << "Failed to prepare SQL: " << query << "; " << sqlite3_errmsg(database);
		return false;
	}

	// bind parameters:
	sqlite3_bind_double(insert, 1, lon);
	sqlite3_bind_double(insert, 2, lat);
	sqlite3_bind_int(insert, 3, amuzith);
	sqlite3_bind_int(insert, 4, elevation);

	// run the sql.
	bool ok = true;
	int tries = 0;
	do {
		rc = sqlite3_step(insert);
	} while (++tries < 100 && (rc == SQLITE_BUSY || rc == SQLITE_LOCKED));

	if (SQLITE_OK != rc && SQLITE_DONE != rc)
	{
		OE_WARN << LC << "Failed query: " << query << "(" << rc << ")" << sqlite3_errstr(rc) << "; " << sqlite3_errmsg(database);
		ok = false;
	}

	sqlite3_finalize(insert);


	return false;
}

bool osgEarth::SqliteData::putShadowmap(double lon, double lat, const std::string & mapdata)
{
	Threading::ScopedMutexLock exclusiveLock(_mutex);

	sqlite3* database = (sqlite3*)_database;

	// Prep the insert statement:
	sqlite3_stmt* insert = NULL;
	std::string query = "INSERT OR REPLACE INTO shadowmap (longitude, latitude, smmap) VALUES (?, ?, ?)";
	int rc = sqlite3_prepare_v2(database, query.c_str(), -1, &insert, 0L);
	if (rc != SQLITE_OK)
	{
		OE_WARN << LC << "Failed to prepare SQL: " << query << "; " << sqlite3_errmsg(database);
		return false;
	}

	// bind parameters:
	sqlite3_bind_double(insert, 1, lon);
	sqlite3_bind_double(insert, 2, lat);
	sqlite3_bind_text(insert, 3, mapdata.c_str(), mapdata.length(), SQLITE_STATIC);

	sqlite3_step(insert);
	sqlite3_finalize(insert);

	return true;
}
