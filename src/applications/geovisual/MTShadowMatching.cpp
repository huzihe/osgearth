/*
 * @File: MTShadowMatching.cpp
 * @Author: hzh
 * @Date: 2021/01/30 21:27
 *
 */

#include "MTShadowMatching.h"

using namespace osgEarth;

#define LC "[geovisual] "

namespace
{
	/**
	 * A TaskRequest that runs a TileHandler in a background thread.
	 */
	class ShadowMatchingTask : public osg::Operation
	{
	public:
		ShadowMatchingTask(ShadowMatching* sm, osg::Vec3 pos, int interval):
			_sm(sm),
			_pos(pos),
			_interval(interval)
		{

		}

		virtual void operator()(osg::Object*)
		{
			if (_sm.valid())
			{
				_sm->Intersection(_pos, _interval);
			}
		}

		osg::Vec3 _pos;
		int _interval;
		osg::ref_ptr<ShadowMatching> _sm;
	};
}

MTShadowMatching::MTShadowMatching()
{
}

MTShadowMatching::~MTShadowMatching()
{
}

unsigned int osgEarth::MTShadowMatching::getNumThreads() const
{
	return _numThreads;
}

void osgEarth::MTShadowMatching::setNumThreads(unsigned int numThreads)
{
	_numThreads = numThreads;
}

bool osgEarth::MTShadowMatching::Intersection(osg::Vec3 pos, int interval)
{
	while (_threadPool->getNumOperationsInQueue() > 1000)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	// Add the tile to the task queue.
	_threadPool->run(new ShadowMatchingTask(_shadowMatching.get(),pos,interval));
	return true;

}

void osgEarth::MTShadowMatching::run()
{
	// Start up the task service
	OE_INFO << "Starting " << _numThreads << " threads " << std::endl;

	_threadPool = new ThreadPool("Geovisual.ShadowMatching", _numThreads);

	while (_threadPool->getNumOperationsInQueue() > 0)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

}

void osgEarth::MTShadowMatching::setSMHandle(ShadowMatching * sm)
{
	_shadowMatching = sm;
}
