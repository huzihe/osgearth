/*
 * @File: ShadowMatching.cpp
 * @Author: hzh
 * @Date: 2021/01/09 22:07
 * 
 */
#include "ShadowMatching.h"
#include <osgEarth/GeoMath>
#include <osgUtil/RayIntersector>
#include <osg/CoordinateSystemNode>
#include <osg/MatrixTransform>



#define LC "[geovisual] "

using namespace osgEarth;
using namespace osgUtil;
using namespace osg;

ShadowMatching::ShadowMatching(osgViewer::Viewer* view)
{
	_view = view;
	
	_mapNode = osgEarth::MapNode::get(_view->getSceneData());
	_geode = new osg::Geode();
	_geom = new osg::Geometry();
	_vec = new osg::Vec3dArray();
	_baseVector = osg::Vec3d(-2850000.0, 4650000.0, 3280000.0); //基准数值，防止世界坐标数值太大，出现闪动

	_sqliteData = new SqliteData();
	_sqliteData->procdata();

	_sqliteData->putMetadata("name", "Shadow Matching results");
	_sqliteData->putMetadata("location", "Shanghai urban area");
	_sqliteData->putMetadata("description", "sqilte database for building boundary map results calculated by shadow matching altorithm");
}

ShadowMatching::~ShadowMatching()
{
}

int 
ShadowMatching::pickFromAzimuthAndElevation(osg::Vec3 pos, float azimuth, float elevation)
{
	float azi = osg::DegreesToRadians(azimuth);
	float ele = osg::DegreesToRadians(elevation);

	osg::Vec3 direction = osg::Vec3(sin(azi)*cos(ele), cos(azi)*cos(ele), sin(ele));

	osg::EllipsoidModel* em = new osg::EllipsoidModel();
	osg::Vec3d world;
	em->convertLatLongHeightToXYZ(osg::DegreesToRadians(pos.y()), osg::DegreesToRadians(pos.x()), pos.z(), world.x(), world.y(), world.z());

	osg::Matrixd localtoworld;
	em->computeCoordinateFrame(osg::DegreesToRadians(pos.y()), osg::DegreesToRadians(pos.x()), localtoworld);
	osg::Vec3d reDirection = localtoworld.preMult(direction);
	reDirection.normalize();

	osg::ref_ptr<osgUtil::RayIntersector> ray = new osgUtil::RayIntersector(osgUtil::Intersector::MODEL, world, reDirection);
	osgUtil::IntersectionVisitor iv(ray.get());
	_mapNode->getLayerNodeGroup()->accept(iv);
	if (ray->containsIntersections())
	{
		osgUtil::RayIntersector::Intersections intersections = ray->getIntersections();
		osgUtil::RayIntersector::Intersections::iterator iter = intersections.begin();
		_vec->push_back(iter->getWorldIntersectPoint() - _baseVector);

		//OSG_NOTICE << "This is a hit test!";
		return 0;
	}
	return -1;
}

float 
ShadowMatching::getElevation(osg::Vec3 pos, float azimuth, float start, float end)
{
	float half = (start + end) / 2;
	float result;
	if (end - start < 1) {
		result = half;
	}
	else
	{
		float flag = pickFromAzimuthAndElevation(pos, azimuth, half);
		if (flag != -1) {
			result = getElevation(pos, azimuth, half, end);
		}
		else
		{
			result = getElevation(pos, azimuth, start, half);
		}
	}
	return result;
}


void 
ShadowMatching::Intersection(osg::Vec3 pos, int interval)
{
	osg::Vec2Array* v2array = new osg::Vec2Array;
	std::string s = "";

	for (int i = 0; i < 360; i += interval)
	{
		osg::Vec2 v2 = osg::Vec2();
		v2.x() = i;
		v2.y() = getElevation(pos, i, 7, 90);
		v2array->push_back(v2);
		s += "[" + toString<int>(int(i)) + "," + toString<int>(int(v2.y())) + "],";
		_sqliteData->putMapData(pos.x(), pos.y(), v2.x(), v2.y());
	}
	_geom->setVertexArray(_vec.get());
	osg::ref_ptr<osg::Vec4Array> vc = new osg::Vec4Array();
	vc->push_back(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
	_geom->setColorArray(vc.get());
	_geom->setColorBinding(osg::Geometry::BIND_OVERALL);
	_geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS,0,_vec->size()));
	_geode->addDrawable(_geom.get());
	osg::ref_ptr<osg::MatrixTransform> trans = new osg::MatrixTransform(osg::Matrix::translate(_baseVector));
	trans->addChild(_geode);
	_mapNode->addChild(trans);
}

void 
ShadowMatching::showIntersections()
{
}
