#include "Quadtree.h"
#include <fstream>
#include <math.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <vector>

//struct HeatPoint;

QuadTree::QuadTree(
				   int index,
				   int layer
				   )
{
	mIndex = index;
	mLayer = layer;
	mTreeNum = 0;
	mNum = 0;

}


void QuadTree::CreatTree(TreeNode* rot)      //构建四叉树
	{
		
		if((rot->num)>=mNum)      //如果区域大小足够小，小于设定的临界值，不在继续划分，跳出递归
		{
			/*float kuadulon = rot->maxlon - rot->minlon;
			float kuadulat = rot->maxlat - rot->minlat;
			std::cout<<"lon跨度:"<<kuadulon<<"lat跨度："<<kuadulat<<std::endl;*/
			rot->mark = 1;  //叶子节点标记
			rot->firch = NULL;
			rot->secch = NULL;
			rot->thrch = NULL;
			rot->fouch = NULL;
		}
		else  //目前区域仍可以继续划分下去
		{
			TreeNode* one = new TreeNode;  //构建四个子节点
			TreeNode* two = new TreeNode;
			TreeNode* three = new TreeNode;
			TreeNode* four = new TreeNode;
			float midlon = (rot->maxlon + rot->minlon)/2.0;
			float midlat = (rot->maxlat + rot->minlat)/2.0;


			one->minlon = rot->minlon;  //划分四个子节点的区域范围
			one->maxlon = midlon;
			one->minlat = rot->minlat;
			one->maxlat = midlat;
			one->mark = 0;
			one->numpoint = 0;

			two->minlon = midlon;
			two->maxlon = rot->maxlon;
			two->minlat = rot->minlat;
			two->maxlat = midlat;
			two->mark = 0;
			two->numpoint = 0;

			three->minlon = rot->minlon;
			three->maxlon = midlon;
			three->minlat = midlat;
			three->maxlat = rot->maxlat;
			three->mark = 0;
			three->numpoint = 0;

			four->minlon = midlon;
			four->maxlon = rot->maxlon;
			four->minlat = midlat;
			four->maxlat = rot->maxlat;
			four->mark = 0;
			four->numpoint = 0;

			one->num = 4*(rot->num)+1;//若某一非叶子结点其序号为n，其四个子结点序号分别为4n+1，4n+2，4n+3，4n+4
			two->num = 4*(rot->num)+2; 
			three->num = 4*(rot->num)+3;
			four->num = 4*(rot->num)+4;

			rot->firch = one;
			rot->secch = two;
			rot->thrch = three;
			rot->fouch = four;

			CreatTree(one);
			CreatTree(two);
			CreatTree(three);
			CreatTree(four);
		}

	}

void QuadTree::Divide(HeatPoint j,TreeNode* rot)//热点数组按地域划分
	{
		float s = j.s *3;//热点影响范围，以热点位置为圆心，3s为半径，做圆，热点对圆形范围外地域产生的热度值近似为0，可以忽略不做计算，只需计算热点对圆形区域产生的热度值
		if (rot->mark == 1)// 如果是叶子节点
		{
			rot->h.push_back(j);//将热点压入当前结点的vector中
			rot->numpoint ++;//当前结点的热点计数器加1
		}

		else  //如果不是叶子结点,判断热点影响范围属于当前结点的哪个子节点范围内
			if ((j.lat+s) <= rot->firch->maxlat && (j.lat-s) >= rot->firch->minlat && (j.lon+s) <= rot->firch->maxlon && (j.lon-s) >= rot->firch->minlon)
			{
				Divide(j,rot->firch);//继续递归遍历
			}
			else
				if ((j.lat+s) <= rot->secch->maxlat && (j.lat-s) >= rot->secch->minlat && (j.lon+s) <= rot->secch->maxlon && (j.lon-s) >= rot->secch->minlon)
				{
					Divide(j,rot->secch);
				} 
				else
					if ((j.lat+s) <= rot->thrch->maxlat && (j.lat-s) >= rot->thrch->minlat && (j.lon+s) <= rot->thrch->maxlon && (j.lon-s) >= rot->thrch->minlon)
					{
						Divide(j,rot->thrch);
					}
					else
						if ((j.lat+s) <= rot->fouch->maxlat && (j.lat-s) >= rot->fouch->minlat && (j.lon+s) <= rot->fouch->maxlon && (j.lon-s) >= rot->fouch->minlon)
						{
							Divide(j,rot->fouch);
						}
						else //热点影响范围不属于当前结点的四个子节点中的任何一个范围内
						{
							rot->h.push_back(j);//热点压入当前结点的vector中
							rot->numpoint ++; //当前结点的热点计数器加1
						}

	}

void QuadTree::PutPoint (TreeNode* rot,HeatPoint* pt)//将按地域划分之后的热点放入新数组中
{
	int numpoint = rot->numpoint;
	if (numpoint==0)  //如果当前结点所代表区域内没有热点
	{
		rot->minindex = -1; //将热点范围下标置为-1
		rot->maxindex = -1;

	} 
	else  //如果结点表示的区域内有热点
	{
		rot->minindex = mIndex;  //该结点表示范围内最小热点数组下标
		rot->maxindex = mIndex+numpoint-1; //该节点表示范围内最大热点数组下标
		for (int i=0;i<numpoint;i++) //vector中的每个热点复制到数组pt中，同时索引值随着增长
		{
			pt[mIndex] = rot->h.at(i);
			mIndex++;
		}
	}

	if (rot->mark == 1)
	{
		return;
	} 
	else  //如果当前结点不是叶子结点
	{
		PutPoint(rot->firch,pt); //递归遍历
		PutPoint(rot->secch,pt);
		PutPoint(rot->thrch,pt);
		PutPoint(rot->fouch,pt);

	}
}

void QuadTree::PutTree(Tree* t,TreeNode* rot)//构建传入GPU中的树结构
{
	int i = rot->num;//树结点的序号即为其数组下标
	int mark = rot->mark;//用于判断是否为叶子结点
	float minlon = rot->minlon;// 计算经度纬度中间值
	float maxlon = rot->maxlon;
	t[i].midlon = (minlon+maxlon)/2.0;
	float minlat = rot->minlat;
	float maxlat = rot->maxlat;
	t[i].midlat = (minlat+maxlat)/2.0;

	t[i].minindex = float(rot->minindex);//热点数组下标传递
	t[i].maxindex = float(rot->maxindex);
	if (mark==1)
	{
		return;
	} 
	else//如果不是叶子结点
	{
		PutTree(t,rot->firch);//递归
		PutTree(t,rot->secch);
		PutTree(t,rot->thrch);
		PutTree(t,rot->fouch);
	}
}

//int QuadTree::Callayer()//计算树的层数
//{
//	int mapsize = mWid * mLeng;
//	int di = 2;
//	double zmapsize = double(mapsize);
//	double zdi = double(di);
//	double mici ;
//	mici = floor(log(zmapsize)/log(zdi)+0.5);
//	int zmici = int(mici);
//	mLayer = (zmici-6)/2+1;
//	return(mLayer);
//}

int QuadTree::Calnum()//计算树的节点个数
{
	double b = pow(4.0,mLayer);
	int c = int(b);
    mTreeNum = (c-1)/3;

	double a = pow(4.0,mLayer-1);
	int d = int(a);
	mNum = (d-1)/3;
	std::cout<<"叶子节点起始下标为"<<mNum<<std::endl;
	return(mTreeNum);
}

