/* -*-c++-*- */
/* osgEarth - Geospatial SDK for OpenSceneGraph
* Copyright 2020 Pelican Mapping
* http://osgearth.org
*
* osgEarth is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
* IN THE SOFTWARE.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include <osgViewer/Viewer>
#include <osgEarth/Notify>
#include <osgEarth/EarthManipulator>
#include <osgEarth/ExampleResources>
#include <osgEarth/MapNode>
#include <osgEarth/Threading>
#include <iostream>

#include <osgEarth/LabelNode>
#include <osgEarth/PlaceNode>
#include <osgEarth/AnnotationLayer>
#include <osgEarth/XmlUtils>

#include <osgEarth/Metrics>
#include "ShadowMatching.h"

#define LC "[viewer] "

using namespace osgEarth;
using namespace osgEarth::Util;

namespace osgEarth {
	
	struct SMHandler : public osgGA::GUIEventHandler
	{
		SMHandler(MapNode* mapNode, char c) : _mapNode(mapNode), _c(c), _layer(0L) {
			_annoGroup = new osg::Group();
			_mapNode->addChild(_annoGroup);
		}

		void setViewer(osgViewer::Viewer* viewer)
		{
			_viewer = viewer;
		}

		osg::Vec3d getPos(const osgGA::GUIEventAdapter& ea,
			osgGA::GUIActionAdapter& aa, osg::Vec3d& pos)
		{
			pos = osg::Vec3d(0, 0, 0);
			//osgViewer::Viewer* pViewer = dynamic_cast<osgViewer::Viewer*>(&aa);
			if (_viewer == NULL)
			{
				return osg::Vec3d(0, 0, 0);
			}
			// 获取当前点
			osgUtil::LineSegmentIntersector::Intersections intersection;
			double x = ea.getX();
			double y = ea.getY();
			_viewer->computeIntersections(ea.getX(), ea.getY(), intersection);
			osgUtil::LineSegmentIntersector::Intersections::iterator iter
				= intersection.begin();
			if (iter != intersection.end())
			{
				osg::ref_ptr<osg::EllipsoidModel> em = new osg::EllipsoidModel();
				_mapNode->getMapSRS()->getGeodeticSRS()->getEllipsoid()->convertXYZToLatLongHeight(
					iter->getWorldIntersectPoint().x(), iter->getWorldIntersectPoint().y(), iter->getWorldIntersectPoint().z(),
					pos.y(), pos.x(), pos.z());
				pos.x() = osg::RadiansToDegrees(pos.x());
				pos.y() = osg::RadiansToDegrees(pos.y());
				//return iter->getWorldIntersectPoint();
				return pos;
			}
			return osg::Vec3d(0, 0, 0);
		}

		bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa, osg::Object*, osg::NodeVisitor*)
		{
			osg::Vec3d vecPos;
			switch (ea.getEventType())
			{
				// 点击事件
				case osgGA::GUIEventAdapter::PUSH:
				{
					osg::Vec3d pos = getPos(ea, aa, vecPos);
					// 鼠标左键
					if (ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
					{
						m_vecPostion = pos;
					}
					break;
				}


			// 鼠标移动事件
			case osgGA::GUIEventAdapter::MOVE:
			{
				osg::Vec3d pos = getPos(ea, aa, vecPos);
				break;
			}

			// 鼠标释放事件
			case osgGA::GUIEventAdapter::RELEASE:
			{
				osg::Vec3d pos = getPos(ea, aa, vecPos);
				// 鼠标左键
				if (ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
				{
					// 如果释放的点和点击的点同一，则发送单击事件发生的位置
					if (m_vecPostion == pos && m_vecPostion != osg::Vec3d(0, 0, 0))
					{
						ShadowMatching* s = new osgEarth::ShadowMatching(_viewer);
						//s->Intersection(m_vecPostion, 2);

						Style pm;
						pm.getOrCreate<IconSymbol>()->url()->setLiteral("../data/placemark32.png");
						pm.getOrCreate<IconSymbol>()->declutter() = true;
						pm.getOrCreate<TextSymbol>()->halo() = Color("#5f5f5f");
						_annoGroup->addChild(new osgEarth::PlaceNode(GeoPoint(_mapNode->getMapSRS()->getGeographicSRS(), m_vecPostion.x(), m_vecPostion.y()), "O", pm));
					}
				}
				break;
			}
			}
			if (ea.getEventType() == ea.KEYDOWN && ea.getKey() == _c)
			{
				osg::Vec3d world;
				_mapNode->getTerrain()->getWorldCoordsUnderMouse(aa.asView(), ea.getX(), ea.getY(), world);

				GeoPoint coords;
				coords.fromWorld(_mapNode->getMap()->getSRS(), world);

				if (!_layer)
				{
					_layer = new AnnotationLayer();
					_layer->setName("User-created Labels");
					_mapNode->getMap()->addLayer(_layer);
				}

				LabelNode* label = new LabelNode();
				label->setText("Label");
				label->setPosition(coords);

				Style style;
				TextSymbol* symbol = style.getOrCreate<TextSymbol>();
				symbol->alignment() = symbol->ALIGN_CENTER_CENTER;
				symbol->size() = 24;
				symbol->halo()->color().set(.1, .1, .1, 1);
				label->setStyle(style);

				_layer->addChild(label);

				osg::ref_ptr<XmlDocument> xml = new XmlDocument(label->getConfig());
				xml->store(std::cout);
			}
			
			return false;
		}
		char _c;
		osg::ref_ptr<MapNode> _mapNode;
		AnnotationLayer* _layer;
		osg::Vec3d m_vecPostion;
		osg::ref_ptr<osgViewer::Viewer> _viewer;
		osg::ref_ptr<osg::Group> _annoGroup;
	};
}

int
usage(const char* name)
{
    OE_NOTICE
        << "\nUsage: " << name << " file.earth" << std::endl
        << MapNodeHelper().usage() << std::endl;

    return 0;
}


int
main(int argc, char** argv)
{
    osgEarth::initialize();

    osg::ArgumentParser arguments(&argc,argv);

    // help?
    if ( arguments.read("--help") )
        return usage(argv[0]);

    // create a viewer:
    osgViewer::Viewer viewer(arguments);

    // Tell the database pager to not modify the unref settings
    viewer.getDatabasePager()->setUnrefImageDataAfterApplyPolicy( true, false );

    // thread-safe initialization of the OSG wrapper manager. Calling this here
    // prevents the "unsupported wrapper" messages from OSG
    osgDB::Registry::instance()->getObjectWrapperManager()->findWrapper("osg::Image");

    // install our default manipulator (do this before calling load)
    viewer.setCameraManipulator( new EarthManipulator(arguments) );

    // disable the small-feature culling
    viewer.getCamera()->setSmallFeatureCullingPixelSize(-1.0f);

    // load an earth file, and support all or our example command-line options
    // and earth file <external> tags
    osg::Node* node = MapNodeHelper().load(arguments, &viewer);
    if ( node )
    {
		osg::ref_ptr<osgEarth::MapNode> s_mapNode = osgEarth::MapNode::get(node);
        viewer.setSceneData( node );
		//viewer.addEventHandler(new CPickHandler(&viewer));
		osg::ref_ptr<SMHandler> smhandler = new SMHandler(s_mapNode.get(), 'L');
		smhandler->setViewer(&viewer);
		viewer.addEventHandler(smhandler);
		return viewer.run();
        //return Metrics::run(viewer);
    }
    else
    {
        return usage(argv[0]);
    }
}