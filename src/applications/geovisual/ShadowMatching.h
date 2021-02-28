#pragma once

#ifndef SHADOWMATCHING_H
#define SHADOWMATCHING_H 1

#include <osgViewer/Viewer>
#include <osgEarth/MapNode>

#include "SqliteData.h"

using namespace osg;

namespace osgEarth {
	class ShadowMatching : public Referenced
	{
	public:
		ShadowMatching(osgViewer::Viewer* view);

		~ShadowMatching();

		bool pickFromAzimuthAndElevation(osg::Vec3 pos, float azimuth, float elevation);

		float getElevation(osg::Vec3 pos, float azimuth, float start, float end);

		void Intersection(osg::Vec3 pos, int interval);

		//模型相交点
		bool showIntersectPoints();

		//格网点
		bool showGridPoints();

		void caculateSM(osg::Vec2d lb, osg::Vec2d rt, double interval);

		bool isIntersected(osg::Vec3 pos);

		std::string int_to_string(int value);

	private:
		osg::ref_ptr<osgViewer::Viewer> _view;
		osg::ref_ptr<osgEarth::MapNode> _mapNode;
		osg::ref_ptr<osg::Geode> _geode;  //存交点
		osg::ref_ptr<osg::Geometry> _geom;  //交点
		osg::ref_ptr<osg::Vec3Array> _vec;


		osg::ref_ptr<osg::Geode> _gridGeode;  //存各网点
		osg::ref_ptr<osg::Geometry> _geomGri;  //格网点
		osg::ref_ptr<osg::Vec3Array> _vecGrid; //格网点数据

		osg::ref_ptr<osg::Geometry> _geomGridSha;  //被遮挡格网点
		osg::ref_ptr<osg::Vec3Array> _vecGridSha; //被遮挡格网点数据

		osg::Vec3d _baseVector;

		SqliteData* _sqliteData;
	};

}

#endif // SHADOWMATCHING_H