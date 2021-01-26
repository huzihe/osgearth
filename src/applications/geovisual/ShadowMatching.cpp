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
	_gridGeode = new osg::Geode();
	_geom = new osg::Geometry();
	_vec = new osg::Vec3Array();
	_geomGri = new osg::Geometry();
	_vecGrid = new osg::Vec3Array();
	_geomGridSha = new osg::Geometry();
	_vecGridSha = new osg::Vec3Array();

	_baseVector = osg::Vec3(-2850000.0, 4650000.0, 3280000.0); //基准数值，防止世界坐标数值太大，出现闪动

	_sqliteData = new SqliteData();
	_sqliteData->procdata();

	_sqliteData->putMetadata("name", "Shadow Matching results");
	_sqliteData->putMetadata("location", "Shanghai urban area");
	_sqliteData->putMetadata("description", "sqilte database for building boundary map results calculated by shadow matching altorithm");

	//test 
	osg::Vec2 lf = osg::Vec2(121.519000, 31.239000);  //121.450, 31.200
	osg::Vec2 rt = osg::Vec2(121.520000, 31.240000);  //121.550, 31.250
	caculateSM(lf, rt, 0.00002);
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

		//将数据存入sqlite数据库中
		_sqliteData->putMapData(pos.x(), pos.y(), v2.x(), v2.y());
	}
	_sqliteData->putShadowmap(pos.x(), pos.y(), s);
	//用于显示建筑模型相交点
	showIntersectPoints();
}


bool osgEarth::ShadowMatching::showIntersectPoints()
{
	_geom->setVertexArray(_vec.get());
	osg::ref_ptr<osg::Vec4Array> vc = new osg::Vec4Array();
	vc->push_back(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
	_geom->setColorArray(vc.get());
	_geom->setColorBinding(osg::Geometry::BIND_OVERALL);
	_geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, _vec->size()));
	_geode->addDrawable(_geom.get());
	osg::ref_ptr<osg::MatrixTransform> trans = new osg::MatrixTransform(osg::Matrix::translate(_baseVector));
	trans->addChild(_geode);
	_mapNode->addChild(trans);
	return false;
}

bool osgEarth::ShadowMatching::showGridPoints()
{
	_geomGri->setVertexArray(_vecGrid.get());
	osg::ref_ptr<osg::Vec4Array> vcori = new osg::Vec4Array();
	vcori->push_back(osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f));
	_geomGri->setColorArray(vcori.get());
	_geomGri->setColorBinding(osg::Geometry::BIND_OVERALL);
	_geomGri->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, _vecGrid->size()));
	_gridGeode->addDrawable(_geomGri.get());


	_geomGridSha->setVertexArray(_vecGridSha.get());
	osg::ref_ptr<osg::Vec4Array> vc = new osg::Vec4Array();
	vc->push_back(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
	_geomGridSha->setColorArray(vc.get());
	_geomGridSha->setColorBinding(osg::Geometry::BIND_OVERALL);
	_geomGridSha->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, _vecGridSha->size()));
	_gridGeode->addDrawable(_geomGridSha.get());

	osg::ref_ptr<osg::MatrixTransform> trans = new osg::MatrixTransform(osg::Matrix::translate(_baseVector));
	trans->addChild(_gridGeode);
	_mapNode->addChild(trans);
	return false;
}

void osgEarth::ShadowMatching::caculateSM(osg::Vec2 lb, osg::Vec2 rt, double interval)
{
	for (double i = lb.x(); i < rt.x(); i += interval)
	{
		for (double j = lb.y(); j < rt.y(); j += interval)
		{
			osg::Vec3 pos = osg::Vec3(i, j, 0);
			if (isIntersected(pos))
			{
				Intersection(pos, 2);
			}
		}
	}
	showGridPoints();
}

//判断点是否在建筑面外
bool osgEarth::ShadowMatching::isIntersected(osg::Vec3 pos)
{
	osg::EllipsoidModel* em = new osg::EllipsoidModel();
	osg::Vec3d world;
	em->convertLatLongHeightToXYZ(osg::DegreesToRadians(pos.y()), osg::DegreesToRadians(pos.x()), pos.z(), world.x(), world.y(), world.z());

	osg::Vec3 direction = em->computeLocalUpVector(world.x(), world.y(), world.z());

	

	osg::ref_ptr<osgUtil::RayIntersector> ray = new osgUtil::RayIntersector(osgUtil::Intersector::MODEL, world, direction);
	osgUtil::IntersectionVisitor iv(ray.get());
	_mapNode->getLayerNodeGroup()->accept(iv);
	if (ray->containsIntersections())
	{
		osgUtil::RayIntersector::Intersections intersections = ray->getIntersections();
		osgUtil::RayIntersector::Intersections::iterator iter = intersections.begin();
		_vecGridSha->push_back(world - _baseVector);
		return false;
	}
	_vecGrid->push_back(world - _baseVector);
	return true;
}
