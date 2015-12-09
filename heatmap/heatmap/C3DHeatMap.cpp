#include "C3DHeatMap.h"
#include <osg/CullFace>
//创建规则网格
osg::Geometry* createRegularGrid(int samplenum)
{
	osg::ref_ptr<osg::Vec3sArray> vec = new osg::Vec3sArray;

	for(int i=0;i<samplenum-1;i++)
		for(int j=0;j<samplenum-1;j++)
		{
			osg::Vec3s vertexLeftButtom(i,j,0); //左下
			osg::Vec3s vertexLeftTop(i,(j+1),0);//左上
			osg::Vec3s vertexRightButtom((i+1),j,0);//右下
			osg::Vec3s vertexRightTop((i+1),(j+1),0.0);//右上
			//绘制规则网格
			vec->push_back(vertexLeftButtom);
			vec->push_back(vertexRightButtom);
			vec->push_back(vertexRightTop);
			vec->push_back(vertexLeftTop);
		}
		osg::Geometry* geo = new osg::Geometry;
		geo->setVertexArray( vec.get() );
		geo->addPrimitiveSet( new osg::DrawArrays( osg::PrimitiveSet::QUADS, 0, (samplenum-1)*(samplenum-1)*4 ) );
		return geo;
}
C3DHeatMap::C3DHeatMap(float minLon,
					   float maxLon,
					   float minLat,
					   float maxLat)
{
	//初始化一些参数
	mSimpleNum=100;//高程点的采样数
	mSpaceLength=10;//采样间距
	mHeightmapID=0;
	mTexturemapID=1;
	minlon = minLon;
	maxlon = maxLon;
	minlat = minLat;
	maxlat = maxLat;
}

C3DHeatMap::~C3DHeatMap(void)
{
}

// 创建包含热度值纹理的组节点
int C3DHeatMap::Create(osg::Texture2D& trgTexture)
{
	//初始化一些参数
	mSimpleNum=2048;//高程点的采样数
	mSpaceLength=1;//采样间距


	// 获取渲染状态StateSet。
	osg::StateSet* state = getOrCreateStateSet();
	// 创建一个PolygonMode 渲染属性。
	osg::PolygonMode* pm = new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
	osg::CullFace* cullface = new osg::CullFace(osg::CullFace::BACK);
	state->setAttributeAndModes( cullface,osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );
	// 强制使用线框渲染。
	//state->setAttributeAndModes( pm,osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );
	//基本绘制栅格
	regularGrid=(osg::Drawable*)createRegularGrid(mSimpleNum);

	//网格纹理
	//SetHeightMap("data/ps_height_4k.png");

	//纹理纹理
	//SetTextureMap("data/ps_texture_4k.png");

	//设置着色器
	osg::Shader* vertexShader = new osg::Shader( osg::Shader::VERTEX );
	osg::Shader* fragmentShader = new osg::Shader( osg::Shader::FRAGMENT );
	vertexShader->loadShaderSourceFromFile( "shader/3DHeatMap.vert" );
	fragmentShader->loadShaderSourceFromFile( "shader/3DHeatMap.frag" );
	osg::Program* program = new osg::Program;
	program->addShader( vertexShader );
	program->addShader( fragmentShader);

	//根目录的渲染状态
	osg::StateSet* stateset = getOrCreateStateSet();
	stateset->setAttributeAndModes( program, osg::StateAttribute::ON );

	osg::Geode* geode = new osg::Geode;
	geode->addDrawable(regularGrid);
	geode->getOrCreateStateSet()->setTextureAttributeAndModes( mHeightmapID, &trgTexture, osg::StateAttribute::ON );
	osg::Uniform* heightMapID = new osg::Uniform( "heightMapID", (int) mHeightmapID );
	stateset->addUniform( heightMapID );
	//采样规模
	osg::Uniform* parSimpleNum = new osg::Uniform( "parSimpleNum",int(mSimpleNum));
	stateset->addUniform( parSimpleNum );
	//采样间隔
	osg::Uniform* parSpacing = new osg::Uniform( "parSpacing",float(mSpaceLength));
	stateset->addUniform( parSpacing );

	osg::Uniform* parMinlon = new osg::Uniform( "parMinlon",float(minlon));
	stateset->addUniform( parMinlon );

	osg::Uniform* parMaxlon = new osg::Uniform( "parMaxlon",float(maxlon));
	stateset->addUniform( parMaxlon );

	osg::Uniform* parMinlat = new osg::Uniform( "parMinlat",float(minlat));
	stateset->addUniform( parMinlat );

	osg::Uniform* parMaxlat = new osg::Uniform( "parMaxlat",float(maxlat));
	stateset->addUniform( parMaxlat );



	//添加彩虹条带
	osg::Image* imageTexture = osgDB::readImageFile("color.png");
	osg::Texture2D* tex = new osg::Texture2D( imageTexture );
	tex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
	tex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
	tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
	tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
	stateset = getOrCreateStateSet();
	stateset->setTextureAttributeAndModes( mTexturemapID, tex, osg::StateAttribute::ON );
	osg::Uniform* textMapID = new osg::Uniform( "textMapID", (int) mTexturemapID );
	stateset->addUniform( textMapID );

	geode->getOrCreateStateSet()->setMode(GL_BLEND,osg::StateAttribute::ON);
	addChild(geode);

	return 0;
}
