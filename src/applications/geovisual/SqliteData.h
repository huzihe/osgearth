#pragma once

#ifndef SQLITEDATA_H
#define SQLITEDATA_H 1

#include <osgEarth/Common>
#include <osgEarth/Threading>

namespace osgEarth {
	class SqliteData
	{
	public:
		SqliteData();
		~SqliteData();

		bool procdata();

		bool createTable();

		bool putMetadata(const std::string& name, const std::string& value);

		bool putMapData(double lon, double lat, float amuzith, float elevation);

		bool putShadowmap(double lon, double lat, const std::string& mapdata);

	private:
		void* _database;
		std::string _fullFilename;
		mutable Threading::Mutex _mutex;
	};
}

#endif // SQLITEDATA_H