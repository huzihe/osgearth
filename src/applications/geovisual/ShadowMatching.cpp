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

#include <string>



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
	osg::Vec2d lf = osg::Vec2d(121.526666, 31.223611);  //121.450, 31.200
	osg::Vec2d rt = osg::Vec2d(121.530833, 31.228333);  //121.550, 31.250
	caculateSM(lf, rt, 0.00002);
}

ShadowMatching::~ShadowMatching()
{
}

bool 
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
		return true;
	}
	return false;
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
		bool flag = pickFromAzimuthAndElevation(pos, azimuth, half);
		if (flag) {
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
	std::string s = "";

	for (int i = 0; i < 360; i += interval)
	{
		//osg::Vec2 v2 = osg::Vec2();
		//v2.x() = i;
		//v2.y() = getElevation(pos, i, 7, 90);
		//s += "[" + toString<int>(int(i)) + "," + toString<int>(int(v2.y())) + "],";  //转字符串效率不高

		int j = getElevation(pos, i, 7, 90);
		//s += "[" + int_to_string(i) + "," + int_to_string(j) + "],";
		s += int_to_string(j) + ",";
		
		////将数据存入sqlite数据库中
		//_sqliteData->putMapData(pos.x(), pos.y(), v2.x(), v2.y());
	}
	_sqliteData->putShadowmap(pos.x(), pos.y(), s);
	//用于显示建筑模型相交点
	//showIntersectPoints();
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

void osgEarth::ShadowMatching::caculateSM(osg::Vec2d lb, osg::Vec2d rt, double interval)
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
	showIntersectPoints();

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


std::string osgEarth::ShadowMatching::int_to_string(int value) {

	static const char digits[19] = {
		'9','8','7','6','5','4','3','2','1','0',
		'1','2','3','4','5','6','7','8','9'
	};
	static const char* zero = digits + 9;//zero->'0'

	char buf[24];      //不考虑线程安全的情况时，可以改成静态变量
	int i = value;
	char *p = buf + 24;
	*--p = '\0';
	do {
		int lsd = i % 10;
		i /= 10;
		*--p = zero[lsd];
	} while (i != 0);
	if (value < 0)
		*--p = '-';
	return std::string(p);
}