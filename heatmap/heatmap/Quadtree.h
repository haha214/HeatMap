#pragma once
#include <vector>
#include "HeatMap.h"
using std::vector;


//CPU上的四叉树结构
struct TreeNode
{
	float minlon; //该地域范围内最小经度
	float maxlon; //该地域范围内最大经度
	float minlat; //该地域范围内最小纬度
	float maxlat; //该地域范围内最大纬度
	TreeNode *firch; //该节点的四个子节点
	TreeNode *secch;
	TreeNode *thrch;
	TreeNode *fouch;
	int mark;    //标记，0表示中间节点，1表示叶子节点
	int num;     //表示节点序号
	int maxindex;//该范围内热点最大下标
	int minindex;//该范围内热点最小下标
	int numpoint;//该范围内热点个数
	vector<HeatPoint> h;//放置热点的容器

};

//传递到GPU上的四叉树结构
struct Tree
{

	float midlon;//该节点代表的地域范围的经度中值
	float midlat;//该节点代表的地域范围的纬度中值
	float maxindex;//属于该节点代表的地域范围内的热点数组最大下标
	float minindex;//属于该节点代表的地域范围内的热点数组最小下标
};

class QuadTree
{
public:
	QuadTree(
		int index, //数组下标索引
		int layer
		);
	void CreatTree(TreeNode* rot);//构建树

	void Divide(HeatPoint j,TreeNode* rot);//热点数组按地域划分

	void PutPoint (TreeNode* rot,HeatPoint* pt);//将按地域划分之后的热点放入数组中

	void PutTree(Tree* t,TreeNode* rot);//构建传入GPU中的树结构

	//int Callayer();//计算树的层数

	int Calnum();//计算树的节点总个数

protected:
	int mIndex;
	int mLayer;//四叉树的层数
	int mTreeNum;//四叉树节点个数
	int mNum;//最后一层叶子节点起始下标

};