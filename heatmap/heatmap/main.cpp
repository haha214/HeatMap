#include <osgCuda/Computation>
#include <osgCuda/Memory>
#include <osgCuda/Geometry>
#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>
#include <osgDB/WriteFile>
#include <vector>
#include "HeatMap.h"
#include "C3DHeatMap.h"
#include "Quadtree.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <fstream>
#include <math.h>
#include <osgEarthUtil/EarthManipulator>
#include <osgEarth/MapNode>
#include <osgEarthUtil//SkyNode>

using namespace std;

extern time_t startall =0 ;
//生成一个贴有特定纹理的方形面片
osg::Geode* getTexturedQuad( osg::Texture2D& trgTexture )
{
	osg::Geode* geode = new osg::Geode;
	osg::Vec3 llCorner = osg::Vec3(0,0,0);
	osg::Vec3 width = osg::Vec3(2048,0,0);
	osg::Vec3 height = osg::Vec3(0,2048,0);

	////////// 
	// QUAD //
	//////////
	osg::ref_ptr<osg::Geometry> geom = osg::createTexturedQuadGeometry( llCorner, width, height );
	geode->addDrawable( geom );
	geode->getOrCreateStateSet()->setTextureAttributeAndModes( 0, &trgTexture, osg::StateAttribute::ON );
	geode->getOrCreateStateSet()->setMode(GL_BLEND,osg::StateAttribute::ON);
	return geode;
}


//这里举例说明HeatMap类的使用方式
int main()
{
	 startall = clock();
	osg::ref_ptr<osg::Group> scene = new osg::Group;
	
	//新建HeatMap类，表明有四个热点，图的分辨率为4096，表示的区域为 经度（0-10度），纬度（0-10度）
	//目前还有一些临界问题没考虑，如地球上-180度和180度结合部位的计算
	//int numHeatPoint = 100000;
	int numHeatPoint = 1797170;
	int numseekgroup = 0;
	int numseekdata = 3*numseekgroup;
	int times = 1000;
	int tnum = numHeatPoint / times;
	HeatPoint* h = new HeatPoint[numHeatPoint];
    HeatPoint* pxh = new HeatPoint[numHeatPoint];//经过分层排序后的热点数组
	int mapwidth = 2048;
	int mapheight = 2048;
	float mapminlon = -129.0;
	float mapmaxlon = -61.0;
	float mapminlat = 18.8;
	float mapmaxlat = 51.18;//美国地域数据范围

	/*float mapminlon = 73.5;
	float mapmaxlon = 135.7;
	float mapminlat = 18.15;
	float mapmaxlat = 53.5;*///中国地域数据范围

    //为根初始化
	TreeNode* root = new TreeNode;
	root->minlon = mapminlon;
	root->maxlon = mapmaxlon;
	root->minlat = mapminlat;
	root->maxlat = mapmaxlat;
	root->num = 0;
	root->mark = 0;
	root->numpoint = 0;

	int heatpointindex = 0;//热点分类追踪下标
	int layer = 5;

	QuadTree* quadTree = new QuadTree(heatpointindex,layer);
	int treenum = quadTree->Calnum();//计算树的总节点数目
	std::cout<<"层数："<<layer<<"树的总结点数目："<<treenum<<std::endl;
	Tree* tr = new Tree[treenum];//存放传递到显存中的树结构
	quadTree->CreatTree(root);//构建树结构

	fstream infile;
	infile.open("../data/dataUSEdge.txt");
	//float fvalue;
	int i=0;
	while(!infile.eof())
	{
		if(i>(numHeatPoint-1))
		{
			break;
		}
		
		infile>>h[i].lon;
		infile>>h[i].lat;
		int value;
		infile>>value;
		h[i].k = 0.0008/**(float)value*/;
		//h[i].k = 0.001;
		h[i].s = 0.08;
		i++;
	

	}//美国地域数据读取
	//while(!infile.eof())
	//{
	//	if(i>(numHeatPoint-1))
	//	{
	//		break;
	//	}
	//	infile>>h[i].lat;
	//	infile>>h[i].lon;
	//	h[i].k = 0.001;
	//	h[i].s = 0.1;
	//	i++;

	//	for(int i=0;i<(1+numseekdata);i++)
	//	{
	//		infile>>fvalue;
	//	}
	//}//中国地域数据读取
	infile.close();//将文件中的热点数据读入并存放在热点数组h中
	//for (int i=0;i<1700000;i++)
	//{
	//		cout<<h[i].lon<<" "<<h[i].lat<<endl;
	//}

    //将热点按地域划分
	for(int i=0;i<numHeatPoint;i++)
	{
		quadTree->Divide(h[i],root);
	}

	quadTree->PutPoint(root,pxh);//将划分后的热点放入数组pxh中
	quadTree->PutTree(tr,root);//复制树结构到数组中
	delete(h);
	time_t endcpu = clock();
	std::cout<<"CPU计算用时："<<(double)(endcpu-startall)/1000.0<<std::endl;
   
    HeatMap* heatMap = new HeatMap(numHeatPoint,treenum,mapwidth,mapheight,mapminlon,mapmaxlon,mapminlat,mapmaxlat,times,tnum,layer);

	//HeatMap类继承于osgComputation类，必须添加到场景树下，否则不执行
	scene->addChild(heatMap);


	osg::ref_ptr<osg::Texture2D> mChinamap;
	osg::Image* mapImg = osgDB::readImageFile("chinamap.png");
	if (!mapImg)
	{
		osg::notify(osg::NOTICE) << "Could not open \"chinamap.png\" image." << std::endl;
		return NULL;
	}
	mChinamap = new osg::Texture2D(mapImg);
	//mChinamap->setTextureHeight(mHeight);
	//mChinamap->setTextureWidth(mWidth);
	mChinamap->setImage(mapImg);
    osgViewer::Viewer viewer;
	osg::Node* earth = osgDB::readNodeFile("D:\\OSG\\OSGEarth\\cloudmade.earth");
	scene->addChild(earth);

	osgEarth::MapNode* mapNode = osgEarth::MapNode::findMapNode(earth);
	osgEarth::Util::SkyNode* mSky = new osgEarth::Util::SkyNode(mapNode->getMap());
	mSky->setDateTime(2012,3,9,4);
	mSky->attach(&viewer);
	scene->addChild(mSky);


	
	if (heatMap->Init())
	{
		//调用HeatMap类的GetTexture函数获取计算得到的热度图纹理，可按普通纹理使用
		//新建一个面片，用来贴生成的纹理
		//scene->addChild(getTexturedQuad(*heatMap->GetTexture()));

		//用于绘制高程图的网格
		C3DHeatMap* c3dheatMap = new C3DHeatMap(mapminlon,mapmaxlon,mapminlat,mapmaxlat);
		c3dheatMap->Create(*heatMap->GetTexture());


		osg::StateSet* stateset = c3dheatMap->getOrCreateStateSet();
		//stateset->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
		//stateset->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);





		//添加到场景中
		//scene->addChild(getTexturedQuad(*mChinamap));
		scene->addChild(c3dheatMap);
		

	}
	
	
	
	viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
	viewer.setReleaseContextAtEndOfFrameHint(false);
	viewer.setSceneData(scene.get());

	
	//用自定义的热点数据更新热图
	//如果只有一个数据改变，还可调用 UpdatePoint(HeatPoint* point,int index)进行单独更新
	//此函数的每次调用将会引起一次CUDA计算
	/*heatMap->UpdateAllPoint(h);*/
	heatMap->UpdateAllPoint(pxh);
	heatMap->UpdateAllTree(tr);
	//viewer.setCameraManipulator(new osgGA::TrackballManipulator);

	viewer.setCameraManipulator(new osgEarth::Util::EarthManipulator);
	viewer.setUpViewInWindow(100,100,800,600);


	while(!viewer.done())
	{			
		viewer.frame();		
	}




	//viewer.setUpViewInWindow(100,100,800,600);
	//viewer.getCamera()->setClearColor(osg::Vec4(1.0,1.0,1.0,1.0));
	//viewer.getCamera()->setViewMatrixAsLookAt(osg::Vec3(1114.367310,-1491.598999,1113.737549),
	//	osg::Vec3(1114.334473,-1490.685059,1113.332886),
	//	osg::Vec3(-0.026274,0.403902,0.914425));
	//
	//while(!viewer.done())
 //	{			
 //		viewer.frame();	
	//	osg::Vec3 eyepos,eyecenter,eyeup;
	//	viewer.getCamera()->getViewMatrixAsLookAt(eyepos,eyecenter,eyeup);
	//	/*printf("eyepos:%f,%f,%f eyecenter:%f,%f,%f eyeup:%f,%f,%f \n",
	//		eyepos.x(),eyepos.y(),eyepos.z(),
	//		eyecenter.x(),eyecenter.y(),eyecenter.z(),
	//		eyeup.x(),eyeup.y(),eyeup.z());*/
 //	}
}