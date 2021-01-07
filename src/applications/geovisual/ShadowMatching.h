#pragma once

#ifndef SHADOWMATCHING_H
#define SHADOWMATCHING_H 1

#include <osgViewer/Viewer>
#include <osgEarth/MapNode>

namespace osgEarth {
	class ShadowMatching
	{
	public:
		ShadowMatching(osgViewer::Viewer* view);

		~ShadowMatching();

		int pickFromAzimuthAndElevation(osg::Vec3 pos, float azimuth, float elevation);

		float getElevation(osg::Vec3 pos, float azimuth, float start, float end);

		void Intersection(osg::Vec3 pos, int interval);
		void showIntersections();

	private:
		osg::ref_ptr<osgViewer::Viewer> _view;
		osg::ref_ptr<osgEarth::MapNode> _mapNode;
		osg::ref_ptr<osg::Geode> _geode;
		osg::ref_ptr<osg::Geometry> _geom;
		osg::ref_ptr<osg::Vec3dArray> _vec;
		osg::Vec3d _baseVector;
	};

}

#endif // SHADOWMATCHING_H