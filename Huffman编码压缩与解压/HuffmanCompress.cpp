//开发者：远渡
//开发时间：2022/12/4
#include <stdio.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits.h>
#include <iostream>
using namespace std;

// 统计字符频度的临时结点
typedef struct 
{
	unsigned char uch;			// 用一个无符号字符存储，大小为1byte共8位，因为无符号位，所以可表示0~255
	unsigned long weight;		// 每类（以二进制编码区分）字符出现频度
} TmpNode;

// 哈夫曼树结点
typedef struct 
{
	unsigned char uch;				// 以8bits为单元的无符号字符
	unsigned long weight;			// 每类（以二进制编码区分）字符出现频度
	char* code;						// 字符对应的哈夫曼编码（动态分配存储空间）
	int parent, lchild, rchild;		// 定义双亲和左右孩子
} HufNode, *HufTree;

void select(HufNode* HT, unsigned int len, int& s1, int& s2)
{
	unsigned int i;
    unsigned long min1 = 0x3f3f3f3f, min2 = 0x3f3f3f3f; //先赋予最大值
    for (i =0; i <len; i+=1)
    {
        if (HT[i].weight < min1 && HT[i].parent == 0)
        {
            min1 = HT[i].weight;
            s1 = i;
        }
    }
    int temp = HT[s1].weight; //将原值存放起来，然后先赋予最大值，防止s1被重复选择
    HT[s1].weight = 0x3f3f3f3f;//赋予最大值，表示最小值已经找到，要找第二最小值
    for (i =0; i <len; i+=1)
    {
        if (HT[i].weight < min2 && HT[i].parent == 0)
        {
            min2 = HT[i].weight;
            s2 = i;
        }
    }
    HT[s1].weight = temp; //恢复原来的值
}

// 建立哈夫曼树
void CreateHuffmanTree(HufNode* HT,int char_types,int node_num)
{
	unsigned int i;
	int s1, s2;
	for(i = char_types; i < node_num; i+=1)  
	{ 
		select(HT, i, s1, s2);		// 选择最小的两个结点
		HT[s1].parent = HT[s2].parent = i; 
		HT[i].lchild = s1; 
		HT[i].rchild = s2; 
		HT[i].weight = HT[s1].weight + HT[s2].weight; 
	} 
}

// 生成哈夫曼编码
void GenerateHuffmanCode(HufNode* HT,int char_types)
{
	unsigned int i;
	int current, next, index;
	char* temp_code = (char* )malloc(256*sizeof(char));//临时表码表，因为该Huffman树最多256个叶子结点，则编码长度不超多255
	temp_code[255] = '\0'; 
	for(i = 0; i < char_types; i+=1)
	{
		index = 256-1;	// 编码临时空间索引初始化
		// 从叶子向根反向遍历求编码
		for(current=i,next=HT[i].parent;next!=0;current=next,next=HT[next].parent)  
			if(HT[next].lchild == current) 
				temp_code[index-=1] = '0';	// 左‘0’
			else 
				temp_code[index-=1] = '1';	// 右‘1’
		HT[i].code = (char* )malloc((256-index)*sizeof(char));			// 为第i个字符编码动态分配存储空间 
		strcpy(HT[i].code, &temp_code[index]);     // 正向保存编码到树结点相应域中
	} 
	free(temp_code);		// 释放编码临时空间
}

unsigned long read_file(char* infile_name,unsigned char char_temp,TmpNode* temp_nodes)
{
    // 遍历文件，获取字符频度
	unsigned long file_len=0;//file_len一定要置零，否则会被编译器赋值意想不到的数导致Depress时有冗余数据
	FILE* infile = fopen(infile_name, "rb");//为避免各种编码问题，采用二进制形式读取
	if (infile == NULL)return -1;//文件不存在返回NULL
	fread((char* )&char_temp, sizeof(unsigned char), 1, infile);//从infile中读取一个unsigned char大小的数据传入char_temp
	while(!feof(infile))
	{
		temp_nodes[char_temp].weight+=1;		// 统计下标对应字符的权重
		file_len+=1;
		fread((char* )&char_temp, sizeof(unsigned char), 1, infile);		// 读入一个字符
	}
	fclose(infile);
    return file_len;
}

void write_file(char* infile_name,char* outfile_name,int char_types,HufTree HT,unsigned long file_len,unsigned char char_temp)
{
	char buffer[256] = "\0";		// 编码缓冲区
    // 写入字符和相应权重，供解压时重建哈夫曼树
    FILE* outfile = fopen(outfile_name, "wb");					// 打开压缩后将生成的文件
	fwrite((char* )&char_types, sizeof(int), 1, outfile);		// 写入字符种类
	for(int i = 0; i < char_types; i+=1)
	{
		fwrite((char* )&HT[i].uch, sizeof(unsigned char), 1, outfile);			// 写入字符（已排序，读出后顺序不变）
		fwrite((char* )&HT[i].weight, sizeof(unsigned long), 1, outfile);		// 写入字符对应权重
	}
	// 紧接着字符和权重信息后面写入文件长度和字符编码
	fwrite((char* )&file_len, sizeof(unsigned long), 1, outfile);		// 写入文件长度
	FILE* infile = fopen(infile_name, "rb");		// 以二进制形式打开待压缩的文件
	fread((char* )&char_temp, sizeof(unsigned char), 1, infile);     // 每次读取8bits
	while(!feof(infile))
	{
		//字符根据Huffman树找对应编码，将编码写入压缩文件
		for(int i = 0; i < char_types; i+=1)if(char_temp == HT[i].uch)strcat(buffer, HT[i].code);
		// 以8位一单位进行处理
		while(strlen(buffer) >= 8)
		{
			char_temp = '\0';		// 清空字符暂存空间，改为暂存字符对应编码'\0'即空字符，表示为00000000
			for(int i = 0; i < 8; i+=1)//目的是将字符型编码转为整型编码，一个字符型0或1占1byte，而八个整型0或1才占1byte
			{
				char_temp <<= 1;		// 左移一位，新增一位为0
				if(buffer[i] == '1')char_temp |= 1;		// 当编码为"1"，通过或操作符将其添加到字节的最低位
			}
			fwrite((char* )&char_temp, sizeof(unsigned char), 1, outfile);		// 将字节对应编码存入文件
			strcpy(buffer, buffer+8);		// 编码缓存去除已处理的前八位
		}
		fread((char* )&char_temp, sizeof(unsigned char), 1, infile);     // 每次读取8bits
	}
	// 处理最后不足8bits编码
	unsigned int code_len=strlen(buffer);
	if(code_len > 0)
	{
		char_temp = '\0';		
		for(int i = 0; i < code_len; i+=1)
		{
			char_temp <<= 1;		
			if(buffer[i] == '1')char_temp |= 1;
		}
		char_temp <<= 8-code_len;       // 将编码字段从尾部移到字节的高位
		fwrite((char* )&char_temp, sizeof(unsigned char), 1, outfile);       // 存入最后一个字节
	}
    //在子函数中打开的文件资源一定要在子函数中释放，因为子函数执行结束文件资源会跟随释放，如果在父函数中才释放，会因为文件资源已经被释放而报错，强制退出程序
	fclose(infile);
	fclose(outfile);
}

void HuffmanCode_match(FILE* infile,char* outfile_name,HufTree HT,unsigned char code_temp,int node_num,int char_types,unsigned long file_len)
{
    int root= node_num-1;		// 保存根节点索引，供匹配编码使用
    unsigned long writen_len = 0;		// 控制文件写入长度
	FILE* outfile = fopen(outfile_name, "wb");		// 打开压缩后将生成的文件
	while(1)
	{
		fread((char* )&code_temp, sizeof(unsigned char), 1, infile);//由于写入时是一个byte一个byte地写，则读取时也是以byte计算
		// 处理读取的一个字符长度的编码（通常为8位）
		for(int i = 0; i < 8; i+=1)
		{
			// 由根向下直至叶子结点正向匹配编码对应字符
			if(code_temp & 128)root = HT[root].rchild;
			else root = HT[root].lchild;
			if(root < char_types)
			{
				fwrite((char* )&HT[root].uch, sizeof(unsigned char), 1, outfile);
				writen_len+=1;
				if (writen_len == file_len) break;//因为压缩时对最后一个byte不足八位强行用0补足，所以需要用file_len控制不读入填充符
				root = node_num-1;        // 复位为根索引，匹配下一个字符
			}
			code_temp<<=1;		// 将编码缓存的下一位移到最高位，供匹配
		}
		if (writen_len == file_len) break;		// 控制文件长度，跳出外层循环
	}
	// 关闭文件
	fclose(infile);
	fclose(outfile);
}




int Compress(char* infile_name, char* outfile_name)// 压缩
{
	unsigned char char_temp;		// 暂存8bits字符
	/*
	** 动态分配256个结点，暂存字符频度，
	** 统计并拷贝到树结点后立即释放
	*/
	TmpNode* temp_nodes =(TmpNode* )malloc(256*sizeof(TmpNode));		

	// 初始化暂存结点
	for(int i = 0; i < 256; i+=1)
	{
		temp_nodes[i].weight = 0;
		temp_nodes[i].uch = (unsigned char)i;		// 数组的256个下标与256种字符对应
	}
    
	unsigned long file_len=read_file(infile_name,char_temp,temp_nodes);
    if (file_len==-1)
    {
        cout<<"未成功打开文件，请重新运行程序"<<endl;
        exit(-1);
    }

	// 排序，将频度为零的放最后，剔除
	for(int i = 0; i < 256-1; i+=1)           
		for(int j = i+1; j < 256; j+=1)
			if(temp_nodes[i].weight < temp_nodes[j].weight)//将权值大的排在前面
			{
				TmpNode node_temp = temp_nodes[i];
				temp_nodes[i] = temp_nodes[j];
				temp_nodes[j] = node_temp;
			}
	
    int count=0;
	//删掉权值为0的字符，权值不为0的字符数就是实际的字符种类数
	for(count=0;count<256;count+=1)if(temp_nodes[count].weight == 0)break;
    int char_types=count;
    
	if (char_types == 1)
	{
		FILE* outfile = fopen(outfile_name, "wb");					// 以二进制形式写入文件
		fwrite((char* )&char_types, sizeof(int), 1, outfile);		// 写入字符种类
		fwrite((char* )&temp_nodes[0].uch, sizeof(unsigned char), 1, outfile);		// 写入唯一的字符
		fwrite((char* )&temp_nodes[0].weight, sizeof(unsigned long), 1, outfile);		// 写入字符频度，也就是文件长度
		free(temp_nodes);
		fclose(outfile);
	}
	else
	{
		int node_num = 2 * char_types - 1;		// 根据字符种类数，计算建立哈夫曼树所需结点数,由Huffman树性质可知n=2n2+1
		HufTree HT = (HufNode* )malloc(node_num*sizeof(HufNode));		// 动态建立哈夫曼树所需结点     

		// 初始化前char_types个结点
		
		for(int i = 0; i < char_types; i+=1)
		{ 
			// 将暂存结点的字符和频度拷贝到树结点
			HT[i].uch = temp_nodes[i].uch; 
			HT[i].weight = temp_nodes[i].weight;
			HT[i].parent = 0; 
		}	
		free(temp_nodes); // 释放字符频度统计的暂存区

		// 初始化后node_num-char_kins个结点
		for(int i=char_types;i<node_num;i+=1)HT[i].parent = 0; 
		

		CreateHuffmanTree(HT, char_types, node_num);		// 创建哈夫曼树

		GenerateHuffmanCode(HT, char_types);		// 生成哈夫曼编码

		write_file(infile_name,outfile_name,char_types,HT,file_len,char_temp);
		// 释放内存
		for(int i = 0; i < char_types; i+=1)free(HT[i].code);
		free(HT);
	}
}




int Decompress(char* infile_name, char* outfile_name)// 解压
{
	unsigned long file_len;
	int char_types;		// 存储字符种类
	unsigned char code_temp;		// 暂存8bits编码

	FILE* infile = fopen(infile_name, "rb");		// 以二进制方式打开压缩文件
	// 判断输入文件是否存在
	if(infile == NULL)return -1;
	// 读取压缩文件前端的字符及对应编码，用于重建哈夫曼树
	fread((char* )&char_types, sizeof(int), 1, infile);     // 读取字符种类数
	if (char_types == 1)//对压缩文件为单一重复字符作特殊处理
	{
		fread((char* )&code_temp, sizeof(unsigned char), 1, infile);     // 读取唯一的字符
		fread((char* )&file_len, sizeof(unsigned long), 1, infile);     // 读取文件长度
		FILE* outfile = fopen(outfile_name, "wb");					// 打开压缩后将生成的文件
		while (file_len--)
			fwrite((char* )&code_temp, sizeof(unsigned char), 1, outfile);
		fclose(infile);
		fclose(outfile);
	}
	else
	{
        int node_num=2*char_types-1;// 根据字符种类数，计算建立哈夫曼树所需结点数。由Huffman树性质可知n=2n2+1
		HufTree HT=(HufNode* )malloc(node_num*sizeof(HufNode));		// 动态分配哈夫曼树结点空间
		// 读取字符及对应权重，存入哈夫曼树节点
		for(int i = 0; i < char_types; i+=1)     
		{
			fread((char* )&HT[i].uch, sizeof(unsigned char), 1, infile);		// 读入字符
			fread((char* )&HT[i].weight, sizeof(unsigned long), 1, infile);	// 读入字符对应权重
			HT[i].parent = 0;
		}
		// 初始化后node_num-char_kins个结点的parent
		for(int i=char_types; i < node_num; i+=1)
			HT[i].parent = 0;

		CreateHuffmanTree(HT, char_types, node_num);		// 重建哈夫曼树（与压缩时的一致）

		// 读完字符和权重信息，紧接着读取文件长度和编码，进行解码
		fread((char* )&file_len, sizeof(unsigned long), 1, infile);	//读入文件长度
		
		HuffmanCode_match(infile,outfile_name,HT,code_temp,node_num,char_types,file_len);
		// 释放内存
		free(HT);
	}
}

int main()
{
	while(1)
	{
		short choice;
        short status=0;		
		char infile_name[256], outfile_name[256];		//输入输出文件名
		cout<<"Please input your choice:"<<endl;
		cout<<"1--->Compress\n2--->Decompress\n3--->Exit"<<endl;// 输入所选择操作类型的数字代号：1：压缩，2：解压，3：退出
		cin>>choice;
		if(choice==3)
        {
            cout<<"Welcome to use again!"<<endl;
			break;
        }
		else
		{
			cout<<"Please input the infile name:"<<endl;
			fflush(stdin);		// 清空标准输入流，防止干扰gets函数读取文件名
			gets(infile_name);
			cout<<"Please input the outfile name:"<<endl;
			fflush(stdin);
			gets(outfile_name);
		}
		switch(choice)
		{
		case 1: cout<<"Compressing,Please wait a moment!"<<endl;
				status= Compress(infile_name, outfile_name);	// 压缩，返回值用于判断是否文件名不存在
				break;
		case 2: cout<<"Decompressing,Please wait a moment"<<endl;
				status= Decompress(infile_name, outfile_name);		// 解压，返回值用于判断是否文件名不存在
				break;		
		}
		if(status==-1)printf("Sorry, infile \"%s\" doesn't exist!\n", infile_name);		// 如果标志为‘-1’则输入文件不存在
        else cout<<"Mission accomplished!"<<endl;
	}
	return 0;
}
