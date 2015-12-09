#pragma once
#include <osg/Geometry>
#include <osg/Group>
#include <osg/Texture2D>
#include <osg/PolygonMode>
#include <osg/TextureCubeMap>
#include <osg/Drawable>
#include <osg/Geode>
#include <osgDB/ReadFile>
typedef unsigned int        UINT;
class C3DHeatMap :
	public osg::Group
{
	UINT mHeightmapID;//结果纹理
	UINT mTexturemapID;//彩虹表
	int mSimpleNum;//高程点的采样数
	float mSpaceLength;//采样间距
	osg::Drawable* regularGrid;
	float minlon;
	float maxlon;
	float minlat;
	float maxlat;
public:
	C3DHeatMap(
		float minLon,//热图表示的最小经度
		float maxLon,//热图表示的最大经度
		float minLat,//热图表示的最小纬度
		float maxLat//热图表示的最大纬度
		);
	~C3DHeatMap(void);
	// 创建包含热度值纹理的组节点
	int Create(osg::Texture2D& trgTexture);
};
