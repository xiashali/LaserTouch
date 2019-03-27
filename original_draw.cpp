// 说明： 本程序演示了如何用程序查询方式读取AD数据
// 请阅读ReadMe.txt文件
#include "stdafx.h"
#include "windows.h"
#include "stdio.h"
#include "math.h"
#include "conio.h"
#include<graphics.h>     //头文件为graphics.h
#include "PCI8664.h"
#include<iostream>
using namespace std;

float all[8][60];//扩展之后的采集卡数据
int num1;  //位选信号位 
float all2[64]; //不再计算平均值 而是记录下每一次采集卡采集到的信号 用于采集卡扩展
int a_time=0; //用于计算平均值时的计数
int iN;
int L=1400;   //界面长 
int S=1200;   //界面宽
int l=16;    //长边的通道个数
int s=16;    //短边的通道个数
int down[18] = { 32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15 };  //屏幕下方的通道序号 所有通道从右下角算起，逆时针一圈
int Left[14] = { 14,13,12,11,10,9,8,7,6,5,4,3,2,1 }; //左边通道 逆时针
int up[18] = {64,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47  };  //上面通道 逆时针
int Right[14] = {46,45,44,43,42,41,40,39,38,37,36,35,34,33 }; //右边通道 逆时针
int sum_down[81];
int sum_left[49];
int sum_up[81];
int sum_right[49];
int InputRange;
int SelectInputRange(void);
int SETV = 3000; //mv //上限电压值
int SETV2 = -2000;  //下限电压值
int time = 0;   //用于数据采集的计数
int s_time = 2; //采集第s_time次的数据作为环境光
int freq=20;  //每freq个数据取一次平均值
float arr[64][20]; //采集卡数据的储存 可以同时储存20组采集卡采集到的信号
float S_arr[64]; //用于储存环境光
#define MAX_AD_CHANNELS 4 // 定义最大通道数
#define AD_DATA_LEN 1024*8 // 要读取和处理的AD数据长度（点或字）
SHORT ADBuffer[AD_DATA_LEN]; // 分配缓冲区(存储原始数据)
void setc(int n);
void setcolor(int i);
void slight();
int main(int argc, char* argv[])
{
	    slight();
		HANDLE hDevice;
		int DeviceLgcID;
		PCI8664_PARA_AD ADPara; // 硬件参数
		LONG nReadSizeWords;   // 每次读取AD数据的长度(字)
		LONG nRetSizeWords;
		BOOL bFirstWait = TRUE; // 为每次等待只显示一次提示
		int nADChannel = 0;
		int ChannelCount = 0;
		SHORT ADData;
		float fVolt;

		DeviceLgcID = 0;
		hDevice = PCI8664_CreateDevice(DeviceLgcID); // 创建设备对象
		if (hDevice == INVALID_HANDLE_VALUE)
		{
			printf("PCI8664_CreateDevice error\n");
			_getch();
			return 0; // 如果创建设备对象失败，则返回
		}

		InputRange = SelectInputRange(); // 要求用户从键盘上选择输入量程

		memset(&ADPara, sizeof(ADPara), 0x00); // 强制初始化为0，以确保各个参数处于确定状态(强烈建议)	
		// 预置硬件参数
		ADPara.ADMode = PCI8664_ADMODE_SEQUENCE; // AD模式为连续模式
		ADPara.FirstChannel = 0; // 同步首通道对(AI0A, AI0B)
		ADPara.LastChannel = 31; // 同步末通道对(AI1A, AI1B)
		ADPara.Frequency = 500000; // 采样频率(Hz)
		ADPara.GroupInterval = 50; // 组间间隔(uS)
		ADPara.LoopsOfGroup = 1;    // 组内各通道点数
		ADPara.Gains = PCI8664_GAINS_1MULT;
		ADPara.InputRange = InputRange;    // 模拟量输入量程范围

		ADPara.TriggerMode = PCI8664_TRIGMODE_SOFT; // 触发模式为硬件触发
		ADPara.TriggerType = PCI8664_TRIGTYPE_EDGE; // 触发类型为边缘触发	
		ADPara.TriggerDir = PCI8664_TRIGDIR_NEGATIVE; // 触发方向为负向
		ADPara.ClockSource = PCI8664_CLOCKSRC_IN;
		ADPara.bClockOutput = PCI8664_CLOCKOUT_ENABLE;//时钟输出

		if (!PCI8664_InitDeviceProAD(hDevice, &ADPara)) // 初始化硬件
		{
			printf("PCI8664_InitDeviceProAD Error\n");
			_getch();
			goto ExitRead;
		}

		ChannelCount = ADPara.LastChannel - ADPara.FirstChannel + 1;
		nReadSizeWords = 4096 - 4096 % ChannelCount; // 将数据长度字转换为双字		

		if (!PCI8664_StartDeviceProAD(hDevice)) // 启动设备
		{
			printf("PCI8664_StartDeviceProAD Error\n");
			_getch();
			goto ExitRead;
		}
		initgraph(L, S);             //使用initgraph函数进行图形初始化
		for (int t = 0; t < 8; ++t)
		{
			line(0, (S / 8) * (t + 1), L, (S / 8) * (t + 1));
		}      //使用line函数画横线
		for (int t = 0; t < 8; ++t)
		{
			line((L / 8) * (t + 1), 0, (L / 8) * (t + 1), S);
		}      //使用line函数画横线

		for (int i = 0; i < 64; ++i)
		{
			setc(i + 1);    //初始化颜色
		}
		int num = 0;
		settextstyle(10, 0, "宋体");  //设置字体与 大小
		for (int a = 0; a < 64; ++a)  //初始化数组
		{
			for (int b = 0; b < 20; ++b)
			{
				arr[a][b] = 0;
			}
		}

		while (!_kbhit())
		{
			bFirstWait = TRUE;
			if (!PCI8664_ReadDeviceProAD_Npt(hDevice, ADBuffer, nReadSizeWords, &nRetSizeWords))
			{
				printf("PCI8664_ReadDeviceProAD_Npt Error\n");
				_getch();
				goto ExitRead;
			}

			nADChannel = ADPara.FirstChannel;
			for (int Index = 0; Index < 64; Index += 2)
			{


				if (_kbhit()) goto ExitRead;
				// 处理同步通道对中的AIxA的数据(x表示某一通道对的序列号)
				ADData = (ADBuffer[Index + 0] ^ 0x0800) & 0x0FFF;
				// 将AIxA原码转换为电压值(mV)
				switch (InputRange)
				{
				case PCI8664_INPUT_N10000_P10000: // -10V - +10V
					fVolt = (float)((20000.00 / 4096) * ADData - 10000.00);
					break;
				case PCI8664_INPUT_N5000_P5000: // -5V - +5V
					fVolt = (float)((10000.00 / 4096) * ADData - 5000.00);
					break;
				case PCI8664_INPUT_0_P10000: // 0V ～ +10V
					fVolt = (float)((10000.00 / 4096) * ADData);
					break;
				default:
					break;
				}

				printf("AI%02dA=%6.2f\t", nADChannel, fVolt); // 显示电压值  fVolt为采集到的电压
				if (time == s_time)  //采集第s_time次的通道值作为现在的环境值
				{
					S_arr[Index] = fVolt;
				}
				else
					if (time>s_time)
				{
						arr[Index][num] = fVolt - S_arr[Index]+ arr[Index][num];
						if (a_time > 0 && a_time%freq == 0)
							arr[Index][num] = arr[Index][num] / freq;    //取平均
				}
				all2[Index] = fVolt;  //采集卡拓展的采集数据
				// 处理同步通道对中的AIxB的数据(x表示某一通道对的序列号)
				ADData = (ADBuffer[Index + 1] ^ 0x0800) & 0x0FFF;
				// 将AIxB原码转换为电压值(mV)
				switch (InputRange)
				{
				case PCI8664_INPUT_N10000_P10000: // -10V - +10V
					fVolt = (float)((20000.00 / 4096) * ADData - 10000.00);
					break;
				case PCI8664_INPUT_N5000_P5000: // -5V - +5V
					fVolt = (float)((10000.00 / 4096) * ADData - 5000.00);
					break;
				case PCI8664_INPUT_0_P10000: // 0V ～ +10V
					fVolt = (float)((10000.00 / 4096) * ADData);
					break;
				default:
					break;
				}

				printf("AI%02dB=%6.2f\t", nADChannel, fVolt); // 显示电压值
				if (time == s_time)
				{
					S_arr[Index+1] = fVolt;
				}
				else
					if (time>s_time)
				   {	
						   arr[Index + 1][num] = fVolt - S_arr[Index + 1]+ arr[Index + 1][num];
						   if (a_time > 0 && a_time%freq == 0)
							   arr[Index + 1][num] =arr[Index+1][num]/ freq;
					};
				all2[Index + 1] = fVolt;
				nADChannel++;
				if (nADChannel > ADPara.LastChannel)
				{
					nADChannel = ADPara.FirstChannel;
					printf("\n");
				}

			} // for(Index=0; Index<64; Index+=2)

			num1 = (all2[61]>2000) * 4 + (all2[62]>2000) * 2 + (all2[63]>2000); //位选
			for (int i = 0; i < 60; ++i)
				{
					all[num1][i] = all2[i];
				}

			if (time>s_time && a_time%freq==0 && a_time>0 )
			{
				num++;
			}      //取完一组平均值后再 取下一组的 数据 
			a_time++;
			time++;
			if (num == 20)
			{

				for (int b = 0; b < 20; ++b)
				{
					for (int a = 0; a < 64; ++a)
					{
						putpixel(b * 4 + ((a) % 8) * (L / 8), (S / 16) + (S / 8) * (a / 8) - (S / 16) * (arr[a][b] / SETV), GREEN);
					}

				}  //把20组数据画出来
				cleardevice();
				for (int t = 0; t < 8; ++t)
				{
					line(0, (S / 8) * (t + 1), L, (S / 8) * (t + 1));
				}      //使用line函数画横线
				for (int t = 0; t < 8; ++t)
				{
					line((L / 8) * (t + 1), 0, (L / 8) * (t + 1), S);
				}      //使用line函数画横线  

				for (int i = 0; i < 64; ++i)
				{
					setc(i + 1);
				}

				for (int a = 0; a < 64; ++a)
				{
					for (int b = 0; b < 19; ++b)
						arr[a][b] = arr[a][b + 1];
				}   //平移一位   数据更新
				num = 19;
				for (int i = 0; i < 9; ++i)
				{
					for (int j = 0; j < 9; ++j)
					{
						sum_down[i * 9 + j] = arr[down[i] - 1][19] + arr[down[9 + j] - 1][19];
					}
				}
				for (int i = 0; i < 7; ++i)
				{
					for (int j = 0; j < 7; ++j)
					{
						sum_left[i * 7 + j] = arr[Left[i] - 1][19] + arr[Left[7 + j] - 1][19];
					}
				}
				for (int i = 0; i < 9; ++i)
				{
					for (int j = 0; j < 9; ++j)
					{
						sum_up[i * 9 + j] = arr[up[i] - 1][19] + arr[up[9 + j] - 1][19];
					}
				}
				for (int i = 0; i < 7; ++i)
				{
					for (int j = 0; j < 7; ++j)
					{
						sum_right[i * 7 + j] = arr[Right[i] - 1][19] + arr[Right[7 + j] - 1][19];
					}
				}
				
			}

		}

	ExitRead:
		PCI8664_ReleaseDeviceProAD(hDevice); // 释放AD
		PCI8664_ReleaseDevice(hDevice); // 释放设备对象
		return 0;
	
}

void setc(int n)  //画图
{
	if (n <= l+s && n>s)
	{
	
		setcolor(int((arr[down[n - 1-s]-1][19] - SETV2) / (SETV - SETV2) * 1022));
			fillcircle(L - (L / (l+1)) * n, S - 5, 5); 
			char A[10];
			sprintf(A, "%.2f", arr[down[n - 1 - s] - 1][19]);
		
			outtextxy(L - (L / (l+1)) * (n-s), S - 30,A);  ///显示数据
	}
	if (n <=s)
	{
		
		setcolor(int((arr[Left[n - 1]-1][19] - SETV2) / (SETV - SETV2) * 1022));
		fillcircle(5, S - (S/(s+1)) * (n - l), 5);
		char A[10];
		sprintf(A, "%.2f", arr[Left[n - 1] - 1][19]);
	
		outtextxy(13, S - (S / (s+1)) * (n ), A);
	}
	if (n > s+s+l && n <= 64)
	{
		setcolor(int((arr[up[n - l - 1-s-s]-1][19] - SETV2) / (SETV - SETV2) * 1022));
		fillcircle((L/(l+1)) * (n - l-s), 5, 5);
		char A[10];
		sprintf(A, "%.2f", arr[up[n - l - 1 - s - s] - 1][19]);
		
		outtextxy((L / (l+1)) * (n - l - s-s)-15, 13, A);
	}
	if (n >= l+s+1 && n <= l+s+s)
	{
	   setcolor(int((arr[Right[n  - 1 - s-l]-1][19] - SETV2) / (SETV - SETV2) * 1022));
		fillcircle(L-5,  (S/(s+1)) * (n - l-l-s), 5);
		char A[10];
		sprintf(A, "%.2f", arr[Right[n - 1 - s - l] - 1][19]);
		
		outtextxy(L - 40, (S / (s+1)) * (n - l - s)-5, A);
	}
}
void setcolor(int i)  //设置颜色 颜色与电压值大小相关联 setv2最小为蓝色 setv最大为红色 渐变色 蓝 到 绿 再到红
{
	if(i>=0&&i<=255)
	{
		setfillcolor(RGB(0, i, 255));
	}
	if(i>=256&&i<=511)
	{
		setfillcolor(RGB(0, 255, 255 - (i-256)));
	}

	if(i>=512&&i<=767)
	{
		setfillcolor(RGB(i-512, 255, 0));

	}
	else
	{
		setfillcolor(RGB(255, 255 - (i-767), 0));
	}
}
//////////////////////////////////////////////////////
// 获取用户选择的输入量程
int SelectInputRange(void)
{
	LONG InputRange;
Repeat:
	printf("\n");
	printf("0. -10V  ～ +10V\n");
	printf("1. -5V   ～ +5V\n");
	printf("2.  0V   ～ +10V\n");

	printf("Please Select Input Range[0-2]:");
	scanf_s("%d", &InputRange);
	if (InputRange<0 || InputRange>2) goto Repeat; // 判断用户选择的量程是否合法，不合法，则重新选择
	
	
	return InputRange;
}
void slight()
{
	cout << "是否进行环境光校准  0:否 1:是 " << endl;
	cin >> iN;
	while (iN == 0)
	{
		cout << "是否进行环境光校准  0:否 1:是 " << endl;
		cin >> iN;;
	}

}