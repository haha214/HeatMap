#ifndef GENERATE_KERNEL_H
#define GENERATE_KERNEL_H 1

#define E 2.718281828459          
#define SQRT2PI 2.5066282746

//将颜色表存储区域映射到一个纹理上，就可以以纹理的形式进行存取
texture<uchar4, 2, cudaReadModeNormalizedFloat> colorTex;

//计算在某个源影响下当前点的高斯值
__device__
float getGaussian(float4* sourcePoint,float trgLon,float trgLat)
{
	//求像素表示的位置和热点所在位置的距离
	float spaceLon = trgLon - (*sourcePoint).z;
	float spaceLat = trgLat - (*sourcePoint).w;
	float pointSpace2 = spaceLon*spaceLon + spaceLat*spaceLat;
	
	//高斯公式的分母
	float denominator = (*sourcePoint).y * SQRT2PI;
	//高斯公式的分子
	
	float numerator = pow(E,(pointSpace2/(-2.0*(*sourcePoint).y*(*sourcePoint).y))); 
	
	//最后要乘以夸张系数
	return (*sourcePoint).x*numerator/denominator;
}

//计算两点间距离
__device__
float dis_PP(float x1,float y1,float x2,float y2)
{
    return sqrt( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) );
}

//计算线程核函数，此函数被并发调用，仅计算一个像素的像素值

__global__ 
void generateTexKernel(float4* targetBuffer,unsigned int trgPitch,float4* sourceBuffer,float4* treeBuffer,unsigned int pointNum,unsigned int imageWidth, unsigned int imageHeight,float minLon,float maxLon,float minLat,float maxLat,int layer)
{
	//计算当前线程在整个线程方阵中所处的位置，用于决定负责计算的像素位置
	unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
    
	//如果线程号超出了纹理大小就不用计算，直接退出就行
	if( x < imageWidth && y < imageHeight )
    {
		//根据线程号计算到当前像素的位置
		float4* target = (float4*)(((char*) targetBuffer) + trgPitch * y ) + x;
		
		//if( (*target).w < 0.9999999999 )
		{
			//计算当前像素表示的实际坐标
			float lon = ((float)(x)/(float)imageWidth)*(float)(maxLon - minLon)+(float)minLon;
			float lat = ((float)(y)/(float)imageHeight)*(float)(maxLat - minLat)+(float)minLat;

			float num = (*target).w;//当前像素表示的实际坐标的热度值

			int treenum = 0;//当前访问的四叉树数组下标
			for(int i=0;i<layer;i++)//遍历四叉树过程中，当前像素表示的地理坐标只可能属于当前树节点的四个子节点表示的地理范围中的一个,故判断的次数等于树的层数
			{

				int maxindex = int((*(treeBuffer+treenum)).z);//需要计算的热点在热点数组中的最大下标
				int minindex = int((*(treeBuffer+treenum)).w);//需要计算的热点在热点数组中的最小下标
			    float midlon = (*(treeBuffer+treenum)).x;//当前树节点代表的地域范围的经度中值
				float midlat = (*(treeBuffer+treenum)).y;//当前树节点代表的地域范围的纬度中值

				if(minindex!=-1)//在当前树节点表示的地域范围下有热点
				{

					for(int j=minindex;j<=maxindex;j++)//计算该范围内的热点产生的热度值，进行累加
				  {
                      float dis = dis_PP(lon,lat,(*(sourceBuffer+j)).z,(*(sourceBuffer+j)).w);
                      float s = 3 * ((*(sourceBuffer+j)).y);
                     if (dis<=s)
                     {
                         num += getGaussian(sourceBuffer+j,lon,lat)*(0.9-num);
                     }
					 

				  }
				}
				//继续遍历，判断当前像素表示的地理坐标属于当前树节点的四个子节点中的哪一个
				if(lon<midlon && lat<midlat)
				{
					treenum = treenum*4+1;
				}
				else 
					if(lon>=midlon && lat<midlat)
					{
						treenum = treenum*4+2;
					}
					else
						if(lon<midlon && lat>=midlat)
						{
							treenum = treenum*4+3;
						}
						else
						{
							treenum = treenum*4+4;
						}

				
			}

			////按顺序计算所有热点对当前像素的影响，并将所有影响值叠加
			//for(unsigned int i=0;i<pointNum;i++)
			//{			
			//	num += getGaussian(sourceBuffer+i,lon,lat)*(0.9-num);
			//}	

		//	float _x = num>1.0?1.0:num;
			//去绑定的颜色纹理里边取颜色	
		//	float4 color = tex2D( colorTex, _x*1.111111, 0.5 );

			//纹理的透明度暂时只与此处的热度强度相关
		//	color.w = _x;
			(*target) = make_float4(num,num,num,num);
		}
	}
}


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
							  int tlayer)
{
	colorTex.normalized = true;                      // normalized texture coordinates (element of [0:1])
    colorTex.filterMode = cudaFilterModeLinear;      // bilinear interpolation 
    colorTex.addressMode[0] = cudaAddressModeClamp;  // wrap texture coordinates
    colorTex.addressMode[1] = cudaAddressModeClamp;

    // 将显存区域绑定到一个纹理
    cudaBindTextureToArray( colorTex, reinterpret_cast<cudaArray*>(colorMap) );

	generateTexKernel<<<blocks,threads>>>(reinterpret_cast<float4*>(texBuffer),
		texPitch,
		reinterpret_cast<float4*>(heatPoints),
		reinterpret_cast<float4*>(treePoints),
		pointNum,
		imageWidth,
		imageHeight,
		minLon,
		maxLon,
		minLat,
		maxLat,
		tlayer);
		cudaThreadSynchronize();

	
	

	
	//cudaMemcpy2D(texBuffer,texPitch,tmpTexBuffer,texPitch,texPitch,imageHeight,cudaMemcpyDeviceToDevice);


}

#endif