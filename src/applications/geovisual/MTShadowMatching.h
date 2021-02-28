#pragma once

#ifndef MTSHADOWMATCHING_H
#define MTSHADOWMATCHING_H 1

#include "ShadowMatching.h"

namespace osgEarth {

	class MTShadowMatching
	{
	public:
		MTShadowMatching();

		~MTShadowMatching();

		unsigned int getNumThreads() const;
		void setNumThreads(unsigned int numThreads);

		bool Intersection(osg::Vec3 pos, int interval);

		virtual void run();

		void setSMHandle(ShadowMatching* sm);

	private:

	protected:
		unsigned int _numThreads;
		osg::ref_ptr<osgViewer::Viewer> _view;
		osg::ref_ptr<ShadowMatching> _shadowMatching;
		osg::ref_ptr<osgEarth::Threading::ThreadPool> _threadPool;

	};

}

#endif // MTSHADOWMATCHING_H