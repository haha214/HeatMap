#pragma once
#include <osgCuda/Computation>
#include <osgCuda/Memory>
#include <osgCuda/Geometry>
#include <osgCuda/Texture>
#include <cuda_runtime.h>




//热点信息结构体
struct HeatPoint
{
	float k;//热度夸张系数，即计算得到的高斯值将会乘上此系数
	float s;//高斯函数标准差，越大表示函数曲线越平滑
	float lon;//热点所处经度，单位为角度
	float lat;//热点所处纬度，单位为角度
};

struct Tree;
class GenerateTexModule;

//热度图类
class HeatMap:public osgCuda::Computation
{
public:
	HeatMap(unsigned int heatPointNum,//热点个数
		unsigned int treeNum,//树节点个数
		unsigned int width,//热图宽度
		unsigned int height,//热图高度
		float minLon,//热图表示的最小经度
		float maxLon,//热图表示的最大经度
		float minLat,//热图表示的最小纬度
		float maxLat,//热图表示的最大纬度
		int times,
		int tnum,
		int tlayer//树的层数
		);

	//更新index指示位置的热点数据
	void UpdatePoint(HeatPoint* point,int index);

	//更新全部热点数据
	void UpdateAllPoint(HeatPoint* pointList);

	void UpdateAllTree(Tree* pointList);
	inline bool IsValid(){return mIsValid;}

	//初始化函数，主要功能为各成员变量的初始化
	bool Init();

	//获取计算得到的热度图
	osg::Texture2D* GetTexture(){return mTexture.get();}
protected:
	osg::ref_ptr<osgCuda::Texture2D> mTexture;
	unsigned int mHeatPointNum;
	unsigned int mTreeNum;
	unsigned int mWidth;
	unsigned int mHeight; 
	float mMinLon;
	float mMaxLon;
	float mMinLat;
	float mMaxLat;
	int mTimes;
	int mTnum;
	int mLayer;
	bool mIsValid;

	//热点数据的保存区域，位于全局显存（猜的，应该是）
	osg::ref_ptr<osgCuda::Memory> mHeatPoint;

	//颜色表所在区域，在生成纹理时在此处查找响应的颜色值
	osg::ref_ptr<osgCuda::Memory> mColorMap;
    //树结构所在区域
	osg::ref_ptr<osgCuda::Memory> mTree;

	/*
	*可理解为核心计算组件，在本类中注册后会在每次更新遍历时调用该组件的launch函数
	*因此可将需要计算的数据指针传递给该组件，将cuda计算函数写在该组件的launch函数中
	*/
	osg::ref_ptr<GenerateTexModule> mTexGenerater;
	bool mNeedUpdate;
};

//计算组件类
class GenerateTexModule : public osgCompute::Module
{
public:
	GenerateTexModule() : osgCompute::Module() 
	{
		//默认值
		_minLon = 0;
		_maxLon = 10;
		_minLat = 0;
		_maxLat = 10;
		_heatPointNum = 1;
		_treeNum = 0;
		_times = 0;
		_tnum = 0;
		_tlayer = 0;
		clearLocal();
	}
	META_Object( , GenerateTexModule )

	//以下三个函数是必须重载的

	//初始化，主要用于分配内存和显存空间等
	virtual bool init();

	//一般真正的计算函数将在此处调用，当组件注册后将不用手动调用该函数
	virtual void launch();

	//在注册类中通过addResource传递的数据将会自动调用此函数进行接收
	virtual void acceptResource( osgCompute::Resource& resource );	

	virtual void clear() { clearLocal(); osgCompute::Module::clear(); }
	
	void Update(){mNeedUpdate = true;}
	void SetUp(float minLon,float maxLon,float minLat,float maxLat,int heatPointNum,int treeNum,int times,int tnum,int tlayer)
	{
		_minLon = minLon;
		_maxLon = maxLon;
		_minLat = minLat;
		_maxLat = maxLat;
		_heatPointNum = heatPointNum;
		_treeNum = treeNum;
		_times = times;
		_tnum = tnum;
		_tlayer = tlayer;
	}
protected:
	virtual ~GenerateTexModule() { clearLocal(); }
	void clearLocal() 
	{
		_sourceBuffer = NULL;
		_targetBuffer = NULL;
		_colorMapBuffer = NULL;
		_treeBuffer = NULL;
	}

	dim3                                             _threads;
	dim3                                             _blocks;
	osg::ref_ptr<osgCompute::Memory>                 _sourceBuffer;
	osg::ref_ptr<osgCompute::Memory>                 _targetBuffer;
	osg::ref_ptr<osgCompute::Memory>                 _colorMapBuffer;
	osg::ref_ptr<osgCompute::Memory>                 _treeBuffer;
	float                                            _minLon;
	float                                            _maxLon;
	float                                            _minLat;
	float                                            _maxLat;
	unsigned int                                     _heatPointNum;
	unsigned int                                     _treeNum;
	int                                              _times;
	int                                              _tnum;
	int                                              _tlayer;
	bool                                             mNeedUpdate;
private:
	GenerateTexModule(const GenerateTexModule&, const osg::CopyOp& ) {}
	inline GenerateTexModule &operator=(const GenerateTexModule &) { return *this; }
};