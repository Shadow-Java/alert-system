#include "sys.h"
#include "ov7670.h"
#include "ov7670cfg.h"
#include "timer.h"	  
#include "delay.h"
#include "usart.h"			 
#include "sccb.h"	
#include "exti.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序参考自网友guanfu_wang代码。
//ALIENTEK战舰STM32开发板V3
//OV7670 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/1/18
//版本：V1.0		    							    							  
//////////////////////////////////////////////////////////////////////////////////
 	    
//初始化OV7670
//返回0:成功
//返回其他值:错误代码
u8 OV7670_Init(void)
{
	u8 temp;
	u16 i=0;	  
	//设置IO
 	GPIO_InitTypeDef  GPIO_InitStructure;
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOD|RCC_APB2Periph_GPIOG|RCC_APB2Periph_AFIO, ENABLE);//使能相关端口时钟
 
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_8; 	//PA8 输入 上拉
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_SetBits(GPIOA,GPIO_Pin_8);
		
 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_4;				 // 端口配置
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
 	GPIO_SetBits(GPIOB,GPIO_Pin_3|GPIO_Pin_4);	

	
	GPIO_InitStructure.GPIO_Pin  = 0xff; //PC0~7 输入 上拉
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
 	GPIO_Init(GPIOC, &GPIO_InitStructure);
	 
	
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_6;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
 	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_SetBits(GPIOD,GPIO_Pin_6);
	
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_14|GPIO_Pin_15;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
 	GPIO_Init(GPIOG, &GPIO_InitStructure);
	GPIO_SetBits(GPIOG,GPIO_Pin_14|GPIO_Pin_15);
	
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);	//SWD

 	SCCB_Init();        		//初始化SCCB 的IO口	   	  
 	if(SCCB_WR_Reg(0x12,0x80))return 1;	//复位SCCB	  
	delay_ms(50);  
	//读取产品型号
 	temp=SCCB_RD_Reg(0x0b);   
	if(temp!=0x73)return 2;  
 	temp=SCCB_RD_Reg(0x0a);   
	if(temp!=0x76)return 2;
	//初始化序列	  
	for(i=0;i<sizeof(ov7670_init_reg_tbl)/sizeof(ov7670_init_reg_tbl[0]);i++)
	{
	   	SCCB_WR_Reg(ov7670_init_reg_tbl[i][0],ov7670_init_reg_tbl[i][1]);
  	}
   	return 0x00; 	//ok
} 
////////////////////////////////////////////////////////////////////////////
//OV7670功能设置
//白平衡设置
//0:自动
//1:太阳sunny
//2,阴天cloudy
//3,办公室office
//4,家里home
void OV7670_Light_Mode(u8 mode)
{
	switch(mode)
	{
		case 0://Auto，自动模式
			SCCB_WR_Reg(0X13,0XE7);//COM8设置 
			SCCB_WR_Reg(0X01,0X00);//AWB蓝色通道增益 
			SCCB_WR_Reg(0X02,0X00);//AWB红色通道增益
			break;
		case 1://sunny
			SCCB_WR_Reg(0X13,0XE5);
			SCCB_WR_Reg(0X01,0X5A);
			SCCB_WR_Reg(0X02,0X5C);
			break;	
		case 2://cloudy
			SCCB_WR_Reg(0X13,0XE5);
			SCCB_WR_Reg(0X01,0X58);
			SCCB_WR_Reg(0X02,0X60);
			break;	
		case 3://office
			SCCB_WR_Reg(0X13,0XE5);
			SCCB_WR_Reg(0X01,0X84);
			SCCB_WR_Reg(0X02,0X4c);
			break;	
		case 4://home
			SCCB_WR_Reg(0X13,0XE5);
			SCCB_WR_Reg(0X01,0X96);
			SCCB_WR_Reg(0X02,0X40);
			break;	
	}
}				  
//色度设置
//sat:-2~+2
void OV7670_Color_Saturation(s8 sat)
{
	u8 reg4f5054val=0X80;
 	u8 reg52val=0X22;
	u8 reg53val=0X5E;
 	switch(sat)
	{
		case -2:
			reg4f5054val=0X40;  	 
			reg52val=0X11;
			reg53val=0X2F;	 	 
			break;	
		case -1:
			reg4f5054val=0X66;	    
			reg52val=0X1B;
			reg53val=0X4B;	  
			break;
		case 0://默认就是sat=0,即不调节色度的设置
			reg4f5054val=0X80;
			reg52val=0X22;
			reg53val=0X5E;
			break;
		case 1:
			reg4f5054val=0X99;	   
			reg52val=0X28;
			reg53val=0X71;	   
			break;	
		case 2:
			reg4f5054val=0XC0;	   
			reg52val=0X33;
			reg53val=0X8D;	   
			break;	
	}
	SCCB_WR_Reg(0X4F,reg4f5054val);	//色彩矩阵系数1
	SCCB_WR_Reg(0X50,reg4f5054val);	//色彩矩阵系数2 
	SCCB_WR_Reg(0X51,0X00);			//色彩矩阵系数3  
	SCCB_WR_Reg(0X52,reg52val);		//色彩矩阵系数4 
	SCCB_WR_Reg(0X53,reg53val);		//色彩矩阵系数5 
	SCCB_WR_Reg(0X54,reg4f5054val);	//色彩矩阵系数6  
	SCCB_WR_Reg(0X58,0X9E);			//MTXS 
}
//亮度设置
//bright：-2~+2
void OV7670_Brightness(s8 bright)
{
	u8 reg55val=0X00;//默认就是bright=0
  	switch(bright)
	{
		case -2://-2
			reg55val=0XB0;	 	 
			break;	
		case -1://-1
			reg55val=0X98;	 	 
			break;
		case 0:
			reg55val=0X00;//默认就是bright=0
			break;
		case 1://1
			reg55val=0X18;	 	 
			break;	
		case 2://2
			reg55val=0X30;	 	 
			break;	
	}
	SCCB_WR_Reg(0X55,reg55val);	//亮度调节 
}
//对比度设置
//contrast：-2~+2
void OV7670_Contrast(s8 contrast)
{
	u8 reg56val=0X40;//默认就是contrast=0
  	switch(contrast)
	{
		case -2:
			reg56val=0X30;	 	 
			break;	
		case -1:
			reg56val=0X38;	 	 
			break;
		case 0:
			reg56val=0X40;
			break;
		case 1:
			reg56val=0X50;	 	 
			break;	
		case 2:
			reg56val=0X60;	 	 
			break;	
	}
	SCCB_WR_Reg(0X56,reg56val);	//对比度调节 
}
//特效设置
//0:普通模式    
//1,负片
//2,黑白   
//3,偏红色
//4,偏绿色
//5,偏蓝色
//6,复古	    
void OV7670_Special_Effects(u8 eft)
{
	switch(eft)
	{
		case 0://正常
			SCCB_WR_Reg(0X3A,0X04);//TSLB设置 
			SCCB_WR_Reg(0X67,0X80);//MANV,手动V值
			SCCB_WR_Reg(0X68,0XC0);//MANU,手动U值
			break;
		case 1://负片
			SCCB_WR_Reg(0X3A,0X24);
			SCCB_WR_Reg(0X67,0X80);
			SCCB_WR_Reg(0X68,0X80);
			break;	
		case 2://黑白
			SCCB_WR_Reg(0X3A,0X14);
			SCCB_WR_Reg(0X67,0X80);
			SCCB_WR_Reg(0X68,0X80);
			break;	
		case 3://偏红色
			SCCB_WR_Reg(0X3A,0X14);
			SCCB_WR_Reg(0X67,0X80);
			SCCB_WR_Reg(0X68,0Xc0);
			break;	
		case 4://偏绿色
			SCCB_WR_Reg(0X3A,0X14);
			SCCB_WR_Reg(0X67,0X40);
			SCCB_WR_Reg(0X68,0X40);
			break;	
		case 5://偏蓝色
			SCCB_WR_Reg(0X3A,0X14);
			SCCB_WR_Reg(0X67,0XC0);
			SCCB_WR_Reg(0X68,0X80);
			break;	
		case 6://复古
			SCCB_WR_Reg(0X3A,0X14);
			SCCB_WR_Reg(0X67,0X40);
			SCCB_WR_Reg(0X68,0XA0);
			break;	 
	}
}	
//设置图像输出窗口
//对QVGA设置。
void OV7670_Window_Set(u16 sx,u16 sy,u16 width,u16 height)
{
	u16 endx;
	u16 endy;
	u8 temp; 
	endx=sx+width*2;	//V*2
 	endy=sy+height*2;
	if(endy>784)endy-=784;
	temp=SCCB_RD_Reg(0X03);				//读取Vref之前的值
	temp&=0XF0;
	temp|=((endx&0X03)<<2)|(sx&0X03);
	SCCB_WR_Reg(0X03,temp);				//设置Vref的start和end的最低2位
	SCCB_WR_Reg(0X19,sx>>2);			//设置Vref的start高8位
	SCCB_WR_Reg(0X1A,endx>>2);			//设置Vref的end的高8位

	temp=SCCB_RD_Reg(0X32);				//读取Href之前的值
	temp&=0XC0;
	temp|=((endy&0X07)<<3)|(sy&0X07);
	SCCB_WR_Reg(0X17,sy>>3);			//设置Href的start高8位
	SCCB_WR_Reg(0X18,endy>>3);			//设置Href的end的高8位
}

 







