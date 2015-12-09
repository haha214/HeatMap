#include <osgDB/ReadFile>
#include "HeatMap.h"
#include <iostream>
#include "Quadtree.h"

//算是CUDA核函数的一个代理，用于调用核函数进行计算，它的实现位于cu文件上
extern "C"
void generateTexFromHeatPoint(const dim3& blocks,
							  const dim3& threads,
							  void* texBuffer,
							  unsigned int texPitch,
							  void* heatPoints,
							  void* treePoints,
							  void* colorMap,
							  unsigned int pointNum,
							  unsigned int imageWidth,
							  unsigned int imageHeight,
							  float minLon,
							  float maxLon,
							  float minLat,
							  float maxLat,
							  int times,
							  int tnum,
							  int tlayer);

HeatMap::HeatMap(unsigned int heatPointNum,
				 unsigned int treeNum,
				 unsigned int width,
				 unsigned int height,
				 float minLon,
				 float maxLon,
				 float minLat,
				 float maxLat,
				 int times,
				 int tnum,
				 int tlayer):osgCuda::Computation()
{	
	mHeatPointNum = heatPointNum;
	mTreeNum = treeNum;
	mWidth = width;
	mHeight = height;
	mMinLon = minLon;
	mMaxLon = maxLon;
	mMinLat = minLat;
	mMaxLat = maxLat;
	mTimes = times;
	mTnum = tnum;
	mLayer = tlayer;
	mIsValid = true;
	mNeedUpdate = true;
}

void HeatMap::UpdatePoint( HeatPoint* point,int index )
{
	if (mIsValid)
	{
		//将需要更新的数据传递到显存
		memcpy((void*)((HeatPoint*)mHeatPoint->map(osgCompute::MAP_HOST_TARGET)+index),
			(void*)point,
			sizeof(HeatPoint));
	}
	mTexGenerater->Update();
}

void HeatMap::UpdateAllPoint( HeatPoint* pointList )
{
	if (IsValid())
	{
		//将需要更新的数据传递到显存
		memcpy(mHeatPoint->map(osgCompute::MAP_HOST_TARGET),
			(void*)pointList,
			sizeof(HeatPoint)*mHeatPointNum);
	}
	mTexGenerater->Update();
}

void HeatMap::UpdateAllTree( Tree* pointList )
{
	if (IsValid())
	{
		//将需要更新的数据传递到显存
		memcpy(mTree->map(osgCompute::MAP_HOST_TARGET),
			(void*)pointList,
			sizeof(Tree)*mTreeNum);
	}
	mTexGenerater->Update();
}


bool HeatMap::Init()
{
	//初始化texture
	mTexture = new osgCuda::Texture2D;
	mTexture->setInternalFormat( GL_RGBA32F_ARB );  
	mTexture->setSourceFormat( GL_RGBA );
	mTexture->setSourceType( GL_FLOAT );
	mTexture->setTextureHeight(mHeight);
	mTexture->setTextureWidth(mWidth);

	//将数据加一个标示，用于在计算中识别数据的类型
	mTexture->addIdentifier( "TRG_BUFFER" );

	//初始化保存热点数据的显存区域
	mHeatPoint = new osgCuda::Memory();
	//设置区域大小
	mHeatPoint->setElementSize(sizeof(HeatPoint));
	//设置每一维数据的大小，此处只有一维数据
	mHeatPoint->setDimension(0,mHeatPointNum);
	mHeatPoint->addIdentifier("SRC_ARRAY");

	//设置好后进行初始化
	if (!mHeatPoint->init())
	{
		mIsValid = false;
		return false;
	}
	//初始化保存树节点数据的显存区域
	mTree = new osgCuda::Memory();
	//设置区域大小
	mTree->setElementSize(sizeof(Tree));
	//设置每一维数据的大小，此处只有一维数据
	mTree->setDimension(0,mTreeNum);
	mTree->addIdentifier("TRE_ARRAY");

	//设置好后进行初始化
	if (!mTree->init())
	{
		mIsValid = false;
		return false;
	}

	//生成颜色表数据空间
	osg::Image* colorImg = osgDB::readImageFile("color.png");
	if (!colorImg)
	{
		osg::notify(osg::NOTICE) << "Could not open \"color.png\" image." << std::endl;
		return NULL;
	}

	cudaChannelFormatDesc srcDesc;
	srcDesc.f = cudaChannelFormatKindUnsigned;
	srcDesc.x = 8;
	srcDesc.y = 8;
	srcDesc.z = 8;
	srcDesc.w = 8;

	mColorMap = new osgCuda::Memory;
	mColorMap->setElementSize( sizeof(osg::Vec4ub) );
	mColorMap->setChannelFormatDesc( srcDesc );
	mColorMap->setDimension( 0, colorImg->s() );
	mColorMap->setDimension( 1, colorImg->t() );
	mColorMap->setImage( colorImg );
	// Mark this buffer as the source array of the module
	mColorMap->addIdentifier( "COLOR_MAP" );

	//初始化计算组件
	mTexGenerater = new GenerateTexModule();
	
	if (!mTexGenerater.valid())
	{
		mIsValid = false;
		return false;
	}
	mTexGenerater->SetUp(mMinLon,mMaxLon,mMinLat,mMaxLat,mHeatPointNum,mTreeNum,mTimes,mTnum,mLayer);
	//表示当前节点会在子节点更新前进行更新
	setComputeOrder(  osgCompute::Computation::PRERENDER_BEFORECHILDREN );
	//注册计算组件
	addModule(*mTexGenerater.get());
	
	//向所有注册的组件传递数据资源
	addResource(*mTexture->getMemory());
	addResource(*mHeatPoint);
	addResource(*mTree);
	addResource(*mColorMap);
	//mTexture->getMemory()->init();
	mTexGenerater->init();
	return true;
}




bool GenerateTexModule::init()
{
	if (!_sourceBuffer||!_targetBuffer||!_treeBuffer)
	{
		return false;
	}

	//表示每个block的线程分布为16*16
	_threads = dim3(16,16,1);
	unsigned int numReqBlocksWidth = 0, numReqBlocksHeight = 0;

	//根据要生成的纹理大小和block内线程的分布计算block的个数
	//以保证每个像素用一个线程计算
	if( _targetBuffer->getDimension(0) % 16 == 0) 
		numReqBlocksWidth = _targetBuffer->getDimension(0) / 16;
	else
		numReqBlocksWidth = _targetBuffer->getDimension(0) / 16 + 1;

	if( _targetBuffer->getDimension(1) % 16 == 0) 
		numReqBlocksHeight = _targetBuffer->getDimension(1) / 16;
	else
		numReqBlocksHeight = _targetBuffer->getDimension(1) / 16 + 1;

	_blocks = dim3( numReqBlocksWidth, numReqBlocksHeight, 1 );

	//最后需调用一次父类的初始化函数
	return osgCompute::Module::init();
}

//在computation类的每次更新时调用，用来执行一次计算
void GenerateTexModule::launch()
{	
	//如果热点数据没有更新则不用计算
	if (mNeedUpdate)
	{	
		extern time_t startall;
		time_t start = clock();
		std::cout<<"开始launch！"<<std::endl;
		memset((float*)_targetBuffer->map(osgCompute::MAP_HOST_TARGET),0.2,4*_targetBuffer->getDimension(0)*_targetBuffer->getDimension(1));
		//做一次计算，算是核函数的代理函数，实质是调用了核函数
		generateTexFromHeatPoint(_blocks,
			_threads,
			_targetBuffer->map(),
			_targetBuffer->getPitch(),
			_sourceBuffer->map(osgCompute::MAP_DEVICE_SOURCE),
			_treeBuffer->map(osgCompute::MAP_DEVICE_SOURCE),
			_colorMapBuffer->map(osgCompute::MAP_DEVICE_ARRAY),
			_heatPointNum,
			_targetBuffer->getDimension(0),
			_targetBuffer->getDimension(1),
			_minLon,
			_maxLon,
			_minLat,
			_maxLat,
			_times,
			_tnum,
			_tlayer);

		cudaThreadSynchronize();
		time_t end = clock();
		std::cout<<"CUDA计算用时："<<(double)(end-start)/1000.0<<std::endl;
		std::cout<<"总计算用时："<<(double)(end-startall)/1000.0<<std::endl;
		//做完计算后将更新标志清空，使下次数据更新之前不再做计算
		mNeedUpdate = false;

	

	}
}

//接受从computation类传递来的数据对象，作为源数据和存放目标数据
void GenerateTexModule::acceptResource( osgCompute::Resource& resource )
{
	//存放结果的缓存，即生成的纹理
	if( resource.isIdentifiedBy( "TRG_BUFFER" ) )
	{
		_targetBuffer = dynamic_cast<osgCompute::Memory*>( &resource );
	}

	//存放热图中热点的位置等信息
	if( resource.isIdentifiedBy( "SRC_ARRAY" ) )
		_sourceBuffer = dynamic_cast<osgCompute::Memory*>( &resource );

	//存放树节点的相关信息
	if( resource.isIdentifiedBy( "TRE_ARRAY" ) )
		_treeBuffer = dynamic_cast<osgCompute::Memory*>( &resource );

	//颜色表区域
	if( resource.isIdentifiedBy( "COLOR_MAP" ) )
		_colorMapBuffer = dynamic_cast<osgCompute::Memory*>( &resource );
}
