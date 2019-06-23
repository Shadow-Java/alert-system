#include "led.h"
#include "delay.h"
#include "key.h"
#include "sys.h"
#include "lcd.h"
#include "usart.h"	 
#include "string.h"
#include "ov7725.h"
#include "ov7670.h"
#include "tpad.h"
#include "timer.h"
#include "exti.h"
#include "usmart.h"
#include "sram.h"
#include "beep.h"
#include "adc.h"
#include "lsens.h"
#include "stmflash.h"
#include "24cxx.h"
#include "w25qxx.h"
#include "touch.h"
#include "malloc.h"
#include "sdio_sdcard.h"
#include "ff.h"  
#include "exfuns.h"   
#include "text.h"
#include "piclib.h"


#define  OV7725 1
#define  OV7670 2

//由于OV7725传感器安装方式原因,OV7725_WINDOW_WIDTH相当于LCD的高度，OV7725_WINDOW_HEIGHT相当于LCD的宽度
//注意：此宏定义只对OV7725有效
#define  OV7725_WINDOW_WIDTH		320 // <=320
#define  OV7725_WINDOW_HEIGHT		240 // <=240

#define ALERT_PHOTO_MAX_NUM 2//最多能在FLASH中存几幅报警图片
#define PHOTO_START_ADDRESS 0X080347FE//在FLASH中存照片的起始地址

const u8*LMODE_TBL[6]={"Auto","Sunny","Cloudy","Office","Home","Night"};//6种光照模式	    
const u8*EFFECTS_TBL[7]={"Normal","Negative","B&W","Redish","Greenish","Bluish","Antique"};	//7种特效 
extern u8 ov_sta;	//在exit.c里 面定义
extern u8 ov_frame;	//在timer.c里面定义 

u8 check_flag=3;//表示是否为第一次检测,即该帧前面没有帧

u8 pixel_R;//某像素点的R值
u8 pixel_G;//某像素点的G值
u8 pixel_B;//某像素点的B值
u8 last_frame_pixel;//上一帧某像素点的灰度值
u8 now_frame_pixel;//当前帧某像素点的灰度值
u32 pixel_change_num=0;//记录变化的像素点数目

u16 past_photo_num=0;//记录本次程序启动已经存了多少张图片

u8 check_mode=1;//检测模式,1为灰度检测,2为RGB检测,3为自动

u8 light_flag=0;//1显示光照强度,0不显示
u8 temp_flag=0;//1显示温度信息,0不显示
u8 light_strong;//记录光照强度
 
//u8 last_frame[OV7725_WINDOW_HEIGHT][OV7725_WINDOW_WIDTH]__attribute__((at(0X68000000)));//存储上一帧图像,存放在外部SRAM中
u16 last_frame[OV7725_WINDOW_HEIGHT][OV7725_WINDOW_WIDTH]__attribute__((at(0X68000000)));//存储上一帧图像,存放在外部SRAM中

u16 mycolor;//调试用
u16 last_one;//调试用

u8 key_index;//输入按键
u8 password_length=0;//密码长度 六位
u8 inputstr[7];//最大输入6个字符
u8 cur_index=120;//索引
u32 password=123456;

u8 res;
DIR picdir;	 		//图片目录
FILINFO picfileinfo;//文件信息
u8 *fn;   			//长文件名
u8 *pname;			//带路径的文件名
u16 curindey;		//图片当前索引
u16 *picindextbl; //图片索引表
u16 totpicnum; //图片文件总数
u8 res; u8 t;u16 temp;
	

void show_picture(){//密码解锁图形界面
 
	POINT_COLOR=RED;
	 while(f_opendir(&picdir,"0:/PICTURE"))//打开图片文件夹
 	{	    
		Show_Str(30,170,240,16,"PICTURE IS WRONG!",16,0);
		delay_ms(200);				  
		LCD_Fill(30,170,240,186,WHITE);//清除显示	     
		delay_ms(200);				  
	}  
	picfileinfo.lfsize=_MAX_LFN*2+1;						//长文件名最大长度
	picfileinfo.lfname=mymalloc(SRAMIN,picfileinfo.lfsize);	//为长文件缓存区分配内存
 	pname=mymalloc(SRAMIN,picfileinfo.lfsize);				//为带路径的文件名分配内存
	//picindextbl=mymalloc(SRAMIN,2*totpicnum);				//申请2*totpicnum个字节的内存,用于存放图片索引
	while(picfileinfo.lfname==NULL||pname==NULL)//内存分配出错
 	{	    
		Show_Str(30,170,240,16,"memory share wrong!",16,0);//内存分配失败
		delay_ms(200);				  
		LCD_Fill(30,170,240,186,WHITE);//清除显示	     
		delay_ms(200);				  
	} 
	//记录索引
    res=f_opendir(&picdir,"0:/PICTURE"); //打开目录
	if(res==FR_OK)
	{
		curindey=0;//当前索引为0
		while(1)//全部查询一遍
		{
			temp=picdir.index;								//记录当前index
			res=f_readdir(&picdir,&picfileinfo);       		//读取目录下的一个文件
			if(res!=FR_OK||picfileinfo.fname[0]==0)break;	//错误了/到末尾了,退出		  
			fn=(u8*)(*picfileinfo.lfname?picfileinfo.lfname:picfileinfo.fname);			 
			res=f_typetell(fn);	
			if((res&0XF0)==0X50)//取高四位,看看是不是图片文件	
			{
				
			}	    
		} 
	}   
	/*Show_Str(30,170,480,16,"Image detection and alarm system is starting ...",16,0); 
	delay_ms(1500);*/
	piclib_init();										//初始化画图	   	   
	curindey=0;											//从0开始显示
	res=f_opendir(&picdir,(const TCHAR*)"0:/PICTURE"); 	//打开目录
	while(res==FR_OK)//打开成功
	{	
	   //dir_sdi(&picdir,picindextbl[curindey]);			//改变当前目录索引
		res=f_readdir(&picdir,&picfileinfo);       		//读取目录下的一个文件
		if(res!=FR_OK||picfileinfo.fname[0]==0)break;	//错误了/到末尾了,退出
		fn=(u8*)(*picfileinfo.lfname?picfileinfo.lfname:picfileinfo.fname);
		strcpy((char*)pname,"0:/PICTURE/");				//复制路径(目录)
		strcat((char*)pname,(const char*)fn);  			//将文件名接在后面
 		LCD_Clear(BLACK);
 		ai_load_picfile(pname,0,0,lcddev.width,lcddev.height,1);//显示图片    
		//Show_Str(2,2,240,16,pname,16,1); 				//显示图片名字
	}
}

//1:<<<键
//2:EXIT键
//3:>>>键
u8 get_keynum_show()//显示界面获取按键信息
{  
    static u8 last_key_show=255;//0
	u8 key=0;//表示没按
    
	tp_dev.scan(0); 
	if(tp_dev.sta&TP_PRES_DOWN){		//触摸屏被按下
		if(tp_dev.x[0]<=145&&tp_dev.x[0]>=45&&tp_dev.y[0]<=705&&tp_dev.y[0]>=655){//按下<<<键
				key=1;
				//key_x=key;
				delay_ms(20);
				
			 // break;
		}else if(tp_dev.x[0]<=435&&tp_dev.x[0]>=335&&tp_dev.y[0]<=705&&tp_dev.y[0]>=655){//按下>>>键
				 key=3;
				 //key_x=key;
				delay_ms(20);
				//break;
		}else if(tp_dev.x[0]<=290&&tp_dev.x[0]>=190&&tp_dev.y[0]<=705&&tp_dev.y[0]>=655){//按下EXIT键
				 key=2;
				 //key_x=key;
				delay_ms(20);
				//break;
		}
					
		if(key){
			if(last_key_show!=255){
				if(last_key_show==key){
					key=0;
				}else{
					last_key_show=key;
				}
			}else{
				last_key_show=key;
			}
		}						
	}else if(last_key_show){
		last_key_show=0;
	}
	
	return key;
}

void touch_gui_main()
{
    POINT_COLOR=BROWN;
    LCD_DrawLine(45,655,145,655);
    LCD_DrawLine(145,655,145,705);
    LCD_DrawLine(145,705,45,705);
    LCD_DrawLine(45,705,45,655);
    LCD_ShowString(80,673,100,20,16,"LIGHT");
    
    LCD_DrawLine(190,655,290,655);
    LCD_DrawLine(290,655,290,705);
    LCD_DrawLine(290,705,190,705);
    LCD_DrawLine(190,705,190,655);
    LCD_ShowString(220,673,100,20,16,"PHOTOS");
      
    LCD_DrawLine(335,655,435,655);
    LCD_DrawLine(435,655,435,705);
    LCD_DrawLine(435,705,335,705);
    LCD_DrawLine(335,705,335,655);
    LCD_ShowString(343,673,100,20,16,"TEMPERATURE");  
}
void touch_gui_login(){//登陆界面按键 横为x坐标 竖为y坐标
    POINT_COLOR=BROWN;
		
	LCD_DrawLine(190,115,290,115);
    LCD_DrawLine(290,115,290,150);
    LCD_DrawLine(290,150,190,150);
    LCD_DrawLine(190,150,190,115);
		
	LCD_DrawLine(45,200,145,200);
    LCD_DrawLine(145,200,145,250);
    LCD_DrawLine(145,250,45,250);
    LCD_DrawLine(45,250,45,200);
    LCD_ShowString(95,220,100,20,16,"1");
		
	LCD_DrawLine(190,200,290,200);
    LCD_DrawLine(290,200,290,250);
    LCD_DrawLine(290,250,190,250);
    LCD_DrawLine(190,250,190,200);
    LCD_ShowString(240,220,100,20,16,"2");
		
	LCD_DrawLine(335,200,435,200);
    LCD_DrawLine(435,200,435,250);
    LCD_DrawLine(435,250,335,250);
    LCD_DrawLine(335,250,335,200);
    LCD_ShowString(385,220,100,20,16,"3");
		
	LCD_DrawLine(45,270,145,270);
    LCD_DrawLine(145,270,145,320);
    LCD_DrawLine(145,320,45,320);
    LCD_DrawLine(45,320,45,270);
    LCD_ShowString(95,290,100,20,16,"4");
		
	LCD_DrawLine(190,270,290,270);
    LCD_DrawLine(290,270,290,320);
    LCD_DrawLine(290,320,190,320);
    LCD_DrawLine(190,320,190,270);
    LCD_ShowString(240,290,100,20,16,"5");
		
	LCD_DrawLine(335,270,435,270);
    LCD_DrawLine(435,270,435,320);
    LCD_DrawLine(435,320,335,320);
    LCD_DrawLine(335,320,335,270);
    LCD_ShowString(385,290,100,20,16,"6");
		
	LCD_DrawLine(45,340,145,340);
    LCD_DrawLine(145,340,145,390);
    LCD_DrawLine(145,390,45,390);
    LCD_DrawLine(45,390,45,340);
    LCD_ShowString(95,360,100,20,16,"7");
		
	LCD_DrawLine(190,340,290,340);
    LCD_DrawLine(290,340,290,390);
    LCD_DrawLine(290,390,190,390);
    LCD_DrawLine(190,390,190,340);
    LCD_ShowString(240,360,100,20,16,"8");
		
	LCD_DrawLine(335,340,435,340);
    LCD_DrawLine(435,340,435,390);
    LCD_DrawLine(435,390,335,390);
    LCD_DrawLine(335,390,335,340);
    LCD_ShowString(385,360,100,20,16,"9");
		
	LCD_DrawLine(190,410,290,410);
    LCD_DrawLine(290,410,290,460);
    LCD_DrawLine(290,460,190,460);
    LCD_DrawLine(190,460,190,410);
    LCD_ShowString(240,430,100,20,16,"0");
		
	LCD_DrawLine(93,480,198,480);
    LCD_DrawLine(198,480,198,530);
    LCD_DrawLine(198,530,93,530);
    LCD_DrawLine(93,530,93,480);
    LCD_ShowString(130,500,100,20,16,"LOGIN");
		
	LCD_DrawLine(282,480,387,480);
    LCD_DrawLine(387,480,387,530);
    LCD_DrawLine(387,530,282,530);
    LCD_DrawLine(282,530,282,480);
    LCD_ShowString(317,500,100,20,16,"CANCEL");
		
	LCD_ShowString(140,70,300,20,16,"please input your password");
}

void touch_gui_show()
{
    POINT_COLOR=BROWN;
    
    LCD_DrawLine(45,655,145,655);
    LCD_DrawLine(145,655,145,705);
    LCD_DrawLine(145,705,45,705);
    LCD_DrawLine(45,705,45,655);
    LCD_ShowString(80,673,100,20,16,"<<<");
    
    LCD_DrawLine(190,655,290,655);
    LCD_DrawLine(290,655,290,705);
    LCD_DrawLine(290,705,190,705);
    LCD_DrawLine(190,705,190,655);
    LCD_ShowString(225,673,100,20,16,"EXIT");
      
    LCD_DrawLine(335,655,435,655);
    LCD_DrawLine(435,655,435,705);
    LCD_DrawLine(435,705,335,705);
    LCD_DrawLine(335,705,335,655);
    LCD_ShowString(370,673,100,20,16,">>>");  
}

void touch_gui_enter()
{
    POINT_COLOR=BROWN;
    
    LCD_DrawLine(45,655,145,655);
    LCD_DrawLine(145,655,145,705);
    LCD_DrawLine(145,705,45,705);
    LCD_DrawLine(45,705,45,655);
    LCD_ShowString(80,673,100,20,16,"GRAY");
    
    LCD_DrawLine(190,655,290,655);
    LCD_DrawLine(290,655,290,705);
    LCD_DrawLine(290,705,190,705);
    LCD_DrawLine(190,705,190,655);
    LCD_ShowString(225,673,100,20,16,"RGB");
      
    LCD_DrawLine(335,655,435,655);
    LCD_DrawLine(435,655,435,705);
    LCD_DrawLine(435,705,335,705);
    LCD_DrawLine(335,705,335,655);
    LCD_ShowString(370,673,100,20,16,"AUTO");  
}

void is_change_mode(u16 last_color,u16 now_color)//检测模式,1为灰度检测,2为RGB检测,3为自动
{
    u8 now_R,now_G,now_B;
    switch(check_mode)
    {
        case 1:{//基于灰度图像检测
            pixel_R=now_color>>11;
            pixel_G=(now_color&0x07E0)>>5;
            pixel_B=now_color&0x001F;
            now_frame_pixel=(u8)((pixel_R*76+pixel_G*150+pixel_B*30)>>8);//当前帧该像素点的灰度值	
            
            pixel_R=last_color>>11;
            pixel_G=(last_color&0x07E0)>>5;
            pixel_B=last_color&0x001F;
            last_frame_pixel=(u8)((pixel_R*76+pixel_G*150+pixel_B*30)>>8);//上一帧该像素点的灰度值	
            
            if((now_frame_pixel-last_frame_pixel)>=25||(last_frame_pixel-now_frame_pixel)>=25)//两个像素点灰度值变化超过20
                pixel_change_num++;//变化的像素点数目+1
            break;
        }
        
        case 2:{//基于RGB图像检测
            now_R=now_color>>11;
            now_G=(now_color&0x07E0)>>5;
            now_B=now_color&0x001F;
            
            pixel_R=last_color>>11;
            pixel_G=(last_color&0x07E0)>>5;
            pixel_B=last_color&0x001F;
            
            //R、B值变化超过5，G值变化超过7，认定发生变化
            if((now_R-pixel_R>=8||pixel_R-now_R>=8)&&(now_G-pixel_G>=14||pixel_G-now_G>=14)&&(now_B-pixel_B>=8||pixel_B-now_B>=8))
                pixel_change_num++;//变化的像素点数目+1
            break;
        }
        
        case 3:{//自动
            if(light_strong>=20){//如果亮度正常采用灰度图像检测
                pixel_R=now_color>>11;
                pixel_G=(now_color&0x07E0)>>5;
                pixel_B=now_color&0x001F;
                now_frame_pixel=(u8)((pixel_R*76+pixel_G*150+pixel_B*30)>>8);//当前帧该像素点的灰度值	
                
                pixel_R=last_color>>11;
                pixel_G=(last_color&0x07E0)>>5;
                pixel_B=last_color&0x001F;
                last_frame_pixel=(u8)((pixel_R*76+pixel_G*150+pixel_B*30)>>8);//上一帧该像素点的灰度值	
                
                if((now_frame_pixel-last_frame_pixel)>=25||(last_frame_pixel-now_frame_pixel)>=25)//两个像素点灰度值变化超过20
                    pixel_change_num++;//变化的像素点数目+1
                break;
            }else{//如果亮度过暗采用RGB检测
                now_R=now_color>>11;
                now_G=(now_color&0x07E0)>>5;
                now_B=now_color&0x001F;
                
                pixel_R=last_color>>11;
                pixel_G=(last_color&0x07E0)>>5;
                pixel_B=last_color&0x001F;
                
                //R、B值变化超过5，G值变化超过7，认定发生变化
                if((now_R-pixel_R>=5||pixel_R-now_R>=5)&&(now_G-pixel_G>=7||pixel_G-now_G>=7)&&(now_B-pixel_B>=5||pixel_B-now_B>=5))
                    pixel_change_num++;//变化的像素点数目+1
                break;
            }
        }
    }
}

void is_change(u16 last_color,u16 now_color)//判断某像素点是否发生变化
{
	/*pixel_R=last_color>>11;
	pixel_G=(last_color&0x07E0)>>5;
	pixel_B=last_color&0x001F;
	last_frame_pixel=(u8)((pixel_R*76+pixel_G*150+pixel_B*30)>>8);*/
	
	pixel_R=now_color>>11;
	pixel_G=(now_color&0x07E0)>>5;
	pixel_B=now_color&0x001F;
	now_frame_pixel=(u8)((pixel_R*76+pixel_G*150+pixel_B*30)>>8);//当前帧该像素点的灰度值	
	
	pixel_R=last_color>>11;
	pixel_G=(last_color&0x07E0)>>5;
	pixel_B=last_color&0x001F;
	last_frame_pixel=(u8)((pixel_R*76+pixel_G*150+pixel_B*30)>>8);//上一帧该像素点的灰度值	
	
	if((now_frame_pixel-last_frame_pixel)>=20||(last_frame_pixel-now_frame_pixel)>=20)//两个像素点灰度值变化超过20
		pixel_change_num++;//变化的像素点数目+1

}

void write_alert_photo()//将当前图像储存在FLASH中
{
	u32 start_secpos;	   //起始扇区地址
	
	u32 i;
	u16 alert_photo_num;//存储的警报图片的数量
	u32 start_address;//基地址

	STMFLASH_Read(PHOTO_START_ADDRESS,&alert_photo_num,1);//从FLASH中读出存储的警报图片的数量
	if(alert_photo_num>ALERT_PHOTO_MAX_NUM){//如果之前没有存过图片
		alert_photo_num=0;
		STMFLASH_Write(PHOTO_START_ADDRESS,&alert_photo_num,1);
	}
	
	/*LCD_ShowNum(30,130,alert_photo_num,3,16);
	LCD_ShowNum(30,150,ALERT_PHOTO_MAX_NUM,3,16);*/
	
	if(alert_photo_num<ALERT_PHOTO_MAX_NUM){
		start_address=PHOTO_START_ADDRESS+2+alert_photo_num*OV7725_WINDOW_WIDTH*OV7725_WINDOW_HEIGHT*2;
		alert_photo_num++;
		STMFLASH_Write(PHOTO_START_ADDRESS,&alert_photo_num,1);
	}else{
		start_address=PHOTO_START_ADDRESS+2+(past_photo_num%2)*OV7725_WINDOW_WIDTH*OV7725_WINDOW_HEIGHT*2;
		past_photo_num++;
	}
	
	//LCD_ShowNum(100,170,start_address,10,16);
	
	start_secpos=(start_address-STM32_FLASH_BASE)/2048;//算出起始扇区地址
		
	FLASH_Unlock();
	
	//擦除扇区
	for(i=0;i<75;i++){
		FLASH_ErasePage(start_secpos*2048+STM32_FLASH_BASE);
		start_secpos++;
	}
	
	for(i=0;i<OV7725_WINDOW_HEIGHT;i++)
	{
		//LCD_ShowNum(30,170,i,3,16);
		/*for(j=0;j<OV7725_WINDOW_WIDTH;j++)
		{				
			LCD_ShowNum(30,170,i,3,16);
			LCD_ShowNum(30,190,j,3,16);
			
			color=last_frame[i][j];
			STMFLASH_Write_NoCheck(start_address,&color,1);

			start_address+=2;								
		}*/
		//LCD_ShowNum(100,190,start_address,10,16);
		
		//STMFLASH_Write(start_address,last_frame[i],OV7725_WINDOW_WIDTH);
		STMFLASH_Write_NoCheck(start_address,last_frame[i],OV7725_WINDOW_WIDTH);
		//LCD_ShowNum(30,190,i,3,16);
		//FLASH_Lock();
		start_address+=2*OV7725_WINDOW_WIDTH;	
	}
	
	FLASH_Lock();
    
    check_flag=5;
}

void show_alert_photo()//显示存储在FLASH中的警报图片
{
	u32 i,j;
	//u16 color[OV7725_WINDOW_WIDTH];//存储的像素点的值
	u16 color;//存储的像素点的值
	u16 alert_photo_num;//存储的警报图片的数量
	u8 now_num=0;//现在播放第几张警报图片
	u8 key;
	u32 start_address;//基地址
	
	LCD_Clear(BLACK);
	
	STMFLASH_Read(PHOTO_START_ADDRESS,&alert_photo_num,1);//从FLASH中读出存储的警报图片的数量
	
	if(alert_photo_num>ALERT_PHOTO_MAX_NUM){//如果之前没有存过图片
		alert_photo_num=0;
		STMFLASH_Write(PHOTO_START_ADDRESS,&alert_photo_num,1);
	}
	/*LCD_ShowNum(30,170,alert_photo_num,10,16);
	STMFLASH_Write(PHOTO_START_ADDRESS,&alert_photo_num,1);
	STMFLASH_Read(PHOTO_START_ADDRESS,&alert_photo_num,1);//从FLASH中读出存储的警报图片的数量
	LCD_ShowNum(30,190,alert_photo_num,10,16);*/
	while(1)
	{
		if(alert_photo_num==0){//如果没有图片
			POINT_COLOR=RED;			//设置字体为红色 
			LCD_ShowString(30,110,200,16,16,"NO PHOTO"); 
		}else{
			LCD_Scan_Dir(U2D_L2R);//从上到下,从左到右
			LCD_Set_Window((lcddev.width-OV7725_WINDOW_WIDTH)/2,(lcddev.height-OV7725_WINDOW_HEIGHT)/2,OV7725_WINDOW_WIDTH,OV7725_WINDOW_HEIGHT);//将显示区域设置到屏幕中央
			if(lcddev.id==0X1963)
				LCD_Set_Window((lcddev.width-OV7725_WINDOW_WIDTH)/2,(lcddev.height-OV7725_WINDOW_HEIGHT)/2,OV7725_WINDOW_HEIGHT,OV7725_WINDOW_WIDTH);//将显示区域设置到屏幕中央
			LCD_WriteRAM_Prepare();     //开始写入GRAM	
					
			start_address=PHOTO_START_ADDRESS+2+now_num*OV7725_WINDOW_WIDTH*OV7725_WINDOW_HEIGHT*2;
			
			for(i=0;i<OV7725_WINDOW_HEIGHT;i++)
			{
				//STMFLASH_Read(start_address,color,OV7725_WINDOW_WIDTH);
				for(j=0;j<OV7725_WINDOW_WIDTH;j++)
				{		
					//LCD->LCD_RAM=color;	
					STMFLASH_Read(start_address,&color,1);
					LCD->LCD_RAM=color;		
					
					start_address+=2;			
				}
				//start_address+=2*OV7725_WINDOW_WIDTH;
			}
			LCD_Scan_Dir(DFT_SCAN_DIR);	//恢复默认扫描方向 
            POINT_COLOR=BLUE;
			LCD_ShowString(30,110,40,16,16,"PHOTO");
			LCD_ShowNum(70,110,now_num+1,1,16);
            POINT_COLOR=BROWN;
            LCD_ShowString(30,130,100,16,16,"KEY0:NEXT");
            LCD_ShowString(30,150,100,16,16,"KEY2:LAST");
            LCD_ShowString(30,170,100,16,16,"KEY1:EXIT");
		}
        touch_gui_show();
		//KEY1 退出
		//KEY0 下一张图片
		//KEY2 上一张图片
		key=KEY_Scan(0);
		if(key==KEY1_PRES){//退出
			LCD_Clear(BLACK);
            check_flag=3;
			break;
		}else if(key==KEY0_PRES){//下一张
			if(now_num==alert_photo_num-1){
				now_num=0;
			}else{
				now_num++;
			}
		}else if(key==KEY2_PRES){//上一张
			if(now_num==0){
				now_num=alert_photo_num-1;
			}else{
				now_num--;
			}
		}
        
        //触屏检测
        key_index=get_keynum_show();
        if(key_index){
            if(key_index==1){//按下<<<键
                if(now_num==0){
                    now_num=alert_photo_num-1;
                }else{
                    now_num--;
                }
            }else if(key_index==3){//按下>>>键
                if(now_num==alert_photo_num-1){
                    now_num=0;
                }else{
                    now_num++;
                }
            }else if(key_index==2){//按下EXIT键
                LCD_Clear(BLACK);
                check_flag=3;
                break;
            }
        }
        
	}
}

//更新LCD显示(OV7725)
void OV7725_camera_refresh(void)
{
	u32 i,j;
 	u16 color;	
	
	if(ov_sta)//有帧中断更新
	{
		LCD_Scan_Dir(U2D_L2R);//从上到下,从左到右
		LCD_Set_Window((lcddev.width-OV7725_WINDOW_WIDTH)/2,(lcddev.height-OV7725_WINDOW_HEIGHT)/2,OV7725_WINDOW_WIDTH,OV7725_WINDOW_HEIGHT);//将显示区域设置到屏幕中央
		if(lcddev.id==0X1963)
			LCD_Set_Window((lcddev.width-OV7725_WINDOW_WIDTH)/2,(lcddev.height-OV7725_WINDOW_HEIGHT)/2,OV7725_WINDOW_HEIGHT,OV7725_WINDOW_WIDTH);//将显示区域设置到屏幕中央
		LCD_WriteRAM_Prepare();     //开始写入GRAM	
		OV7725_RRST=0;				//开始复位读指针 
		OV7725_RCK_L;
		OV7725_RCK_H;
		OV7725_RCK_L;
		OV7725_RRST=1;				//复位读指针结束 
		OV7725_RCK_H; 
		
		pixel_change_num=0;
		
		for(i=0;i<OV7725_WINDOW_HEIGHT;i++)
		{
			for(j=0;j<OV7725_WINDOW_WIDTH;j++)
			{
				OV7725_RCK_L;
				color=GPIOC->IDR&0XFF;	//读数据
				OV7725_RCK_H; 
				color<<=8;  
				OV7725_RCK_L;
				color|=GPIOC->IDR&0XFF;	//读数据
				OV7725_RCK_H; 
				
				mycolor=color;
				
				/*pixel_R=color>>11;
				pixel_G=(color&0x07E0)>>5;
				pixel_B=color&0x001F;
				now_frame_pixel=(u8)((pixel_R*76+pixel_G*150+pixel_B*30)>>8);//当前帧该像素点的灰度值		*/		
				
				is_change_mode(last_frame[i][j],color);
				last_frame[i][j]=color;
				last_one=last_frame[i][j];
				
				LCD->LCD_RAM=color;										
			}
		}
		
 		ov_sta=0;					//清零帧中断标记
		ov_frame++; 
		LCD_Scan_Dir(DFT_SCAN_DIR);	//恢复默认扫描方向 
	} 
}

//更新LCD显示(OV7670)
void OV7670_camera_refresh(void)
{
	u32 j;
 	u16 color;	 
	if(ov_sta)//有帧中断更新
	{
		LCD_Scan_Dir(U2D_L2R);//从上到下,从左到右  
		if(lcddev.id==0X1963)LCD_Set_Window((lcddev.width-240)/2,(lcddev.height-320)/2,240,320);//将显示区域设置到屏幕中央
		else if(lcddev.id==0X5510||lcddev.id==0X5310)LCD_Set_Window((lcddev.width-320)/2,(lcddev.height-240)/2,320,240);//将显示区域设置到屏幕中央
		LCD_WriteRAM_Prepare();     //开始写入GRAM	
		OV7670_RRST=0;				//开始复位读指针 
		OV7670_RCK_L;
		OV7670_RCK_H;
		OV7670_RCK_L;
		OV7670_RRST=1;				//复位读指针结束 
		OV7670_RCK_H;
		for(j=0;j<76800;j++)
		{
			OV7670_RCK_L;
			color=GPIOC->IDR&0XFF;	//读数据
			OV7670_RCK_H; 
			color<<=8;  
			OV7670_RCK_L;
			color|=GPIOC->IDR&0XFF;	//读数据
			OV7670_RCK_H; 
			LCD->LCD_RAM=color;    
		}   							  
 		ov_sta=0;					//清零帧中断标记
		ov_frame++; 
		LCD_Scan_Dir(DFT_SCAN_DIR);	//恢复默认扫描方向 
	} 
}

//检测是否有人,返回0表示没人,1表示有人
u8 person_check_mode()
{
    switch(check_mode)
    {
        case 1:{//基于灰度图像检测
            if(check_flag==0)//如果不是第一帧
            {
                if(pixel_change_num>=3000)//变化的像素点数目超过3000
                {
                    pixel_change_num=0;
                    return 1;
                }else{
                    pixel_change_num=0;
                    return 0;
                }
            }else//如果是第一帧
            {
                check_flag--;
                pixel_change_num=0;
                return 0;
            }
        }
        
        case 2:{//基于RGB图像检测
            if(check_flag==0)//如果不是第一帧
            {
                if(pixel_change_num>=6000)//变化的像素点数目超过6000
                {
                    pixel_change_num=0;
                    return 1;
                }else{
                    pixel_change_num=0;
                    return 0;
                }
            }else//如果是第一帧
            {
                check_flag--;
                pixel_change_num=0;
                return 0;
            }
        }
        
        case 3:{//自动检测
            if(light_strong>=20){//亮度正常
                if(check_flag==0)//如果不是第一帧
                {
                    if(pixel_change_num>=3000)//变化的像素点数目超过3000
                    {
                        pixel_change_num=0;
                        return 1;
                    }else{
                        pixel_change_num=0;
                        return 0;
                    }
                }else//如果是第一帧
                {
                    check_flag--;
                    pixel_change_num=0;
                    return 0;
                }               
            }else{//亮度过暗
                if(check_flag==0)//如果不是第一帧
                {
                    if(pixel_change_num>=6000)//变化的像素点数目超过6000
                    {
                        pixel_change_num=0;
                        return 1;
                    }else{
                        pixel_change_num=0;
                        return 0;
                    }
                }else//如果是第一帧
                {
                    check_flag--;
                    pixel_change_num=0;
                    return 0;
                }              
            }
        }
    }
    
    return 0;
}

u8 person_check()//检测是否有人,返回0表示没人,1表示有人
{
	if(check_flag==0)//如果不是第一帧
	{
		if(pixel_change_num>=3000)//变化的像素点数目超过19200
		{
			pixel_change_num=0;
			return 1;
		}else{
			pixel_change_num=0;
			return 0;
		}
	}else//如果是第一帧
	{
		check_flag--;
		pixel_change_num=0;
		return 0;
	}
}
//按键状态设置
//x,y:键盘坐标
//key:键值（0~8）
//sta:状态，0，松开；1，按下；
/*void py_key_staset(u16 x,u16 y,u8 keyx,u8 sta)
{		  
	u16 i=keyx/3,j=keyx%3;
	if(keyx>8)return;
	if(sta)LCD_Fill(x+j*kbdxsize+1,y+i*kbdysize+1,x+j*kbdxsize+kbdxsize-1,y+i*kbdysize+kbdysize-1,GREEN);
	else LCD_Fill(x+j*kbdxsize+1,y+i*kbdysize+1,x+j*kbdxsize+kbdxsize-1,y+i*kbdysize+kbdysize-1,WHITE); 
	Show_Str_Mid(x+j*kbdxsize,y+4+kbdysize*i,(u8*)kbd_tbl[keyx],16,kbdxsize);		
	Show_Str_Mid(x+j*kbdxsize,y+kbdysize/2+kbdysize*i,(u8*)kbs_tbl[keyx],16,kbdxsize);		 
}*/
/*void py_key_staset(u16 x1,u16 y1,u16 x2,u16 y2,u8 keyx,u8 sta){//按键状态设置
    if(keyx>12)return;
		if(sta)LCD_Fill(x+j*60+1,y+i*40+1,x+j*60+59,y+i*40+39,GREEN);
		else LCD_Fill(x+j*60+1,y+i*40+1,x+j*60+59,y+i*40+39,WHITE);
}*/
u8 py_get_keynum(){//获得键值

	 //u8 real_key;
	static u8 key_x=255;//0,没有任何按键按下；1~9，1~9号按键按下
	u8 key=0;//表示没按
	tp_dev.scan(0); 
	if(tp_dev.sta&TP_PRES_DOWN){		//触摸屏被按下
		if(tp_dev.x[0]<=145&&tp_dev.x[0]>=45&&tp_dev.y[0]<=250&&tp_dev.y[0]>=200){//按下“1”
				key=1;
				//key_x=key;
				delay_ms(20);
				
			 // break;
		}else if(tp_dev.x[0]<=290&&tp_dev.x[0]>=190&&tp_dev.y[0]<=250&&tp_dev.y[0]>=200){//按下“2”
				 key=2;
				 //key_x=key;
				delay_ms(20);
				//break;
		}else if(tp_dev.x[0]<=435&&tp_dev.x[0]>=335&&tp_dev.y[0]<=250&&tp_dev.y[0]>=200){//按下“3”
				 key=3;
				 //key_x=key;
				delay_ms(20);
				//break;
		}else if(tp_dev.x[0]<=145&&tp_dev.x[0]>=45&&tp_dev.y[0]<=320&&tp_dev.y[0]>=270){//按下“4”
				 key=4;
				 //key_x=key;
				delay_ms(20);
			//  break;
		}else if(tp_dev.x[0]<=290&&tp_dev.x[0]>=190&&tp_dev.y[0]<=320&&tp_dev.y[0]>=270){//按下“5”
				 key=5;
				 //key_x=key;
				delay_ms(20);
			 // break;
			 
		}else if(tp_dev.x[0]<=435&&tp_dev.x[0]>=335&&tp_dev.y[0]<=320&&tp_dev.y[0]>=270){//按下“6”
				 key=6;
				 //key_x=key;
				delay_ms(20);
				//break;
		}else if(tp_dev.x[0]<=145&&tp_dev.x[0]>=45&&tp_dev.y[0]<=390&&tp_dev.y[0]>=340){//按下“7”
					key=7;
				 //key_x=key;
				delay_ms(20);
				//break;
		}else if(tp_dev.x[0]<=290&&tp_dev.x[0]>=190&&tp_dev.y[0]<=390&&tp_dev.y[0]>=340){//按下“8”
				 key=8;
				 //key_x=key;
				delay_ms(20);
			 // break;
		}else if(tp_dev.x[0]<=435&&tp_dev.x[0]>=335&&tp_dev.y[0]<=390&&tp_dev.y[0]>=340){//按下“9”
				 key=9;
				 //key_x=key;
				delay_ms(20);
			 // break;
		}else if(tp_dev.x[0]<=290&&tp_dev.x[0]>=190&&tp_dev.y[0]<=460&&tp_dev.y[0]>=410){//按下“0”
				 key=10;
				 //key_x=key;
				delay_ms(20);
			 // break;
		}else if(tp_dev.x[0]<=198&&tp_dev.x[0]>=93&&tp_dev.y[0]<=530&&tp_dev.y[0]>=480){//按下“LOGIN”
				key=11;
				 //key_x=key;
				delay_ms(20);
				//break;
			
		}else if(tp_dev.x[0]<=387&&tp_dev.x[0]>=282&&tp_dev.y[0]<=530&&tp_dev.y[0]>=480){//按下“CANCEL”
				 key=12;
				 //key_x=key;
				delay_ms(20);
				//break;
		}  
			
		//LCD_ShowNum(200,650,key_x,3,16);
		
		//real_key=key;
		
		if(key){
			if(key_x!=255){
				if(key_x==key){
					key=0;
				}else{
					key_x=key;
				}
			}else{
				key_x=key;
			}
		}
				
						
	}else if(key_x){
		key_x=0;
	}
	
	//LCD_ShowNum(250,650,key_x,3,16);
	//LCD_ShowNum(300,650,key,3,16);
	//LCD_ShowString(15,500,100,20,16,"length:");
	//LCD_ShowNum(50,500,password_length,3,16);
	return key;
}

//1:GRAY
//2:RGB
//3:AUTO
u8 get_keynum_enter()//登录界面获取按键信息
{
   
    static u8 last_key_enter=255;//0
	u8 key=0;//表示没按
    
	tp_dev.scan(0); 
	if(tp_dev.sta&TP_PRES_DOWN){		//触摸屏被按下
		if(tp_dev.x[0]<=145&&tp_dev.x[0]>=45&&tp_dev.y[0]<=705&&tp_dev.y[0]>=655){//按下GRAY键
				key=1;
				//key_x=key;
				delay_ms(20);
				
			 // break;
		}else if(tp_dev.x[0]<=435&&tp_dev.x[0]>=335&&tp_dev.y[0]<=705&&tp_dev.y[0]>=655){//按下AUTO键
				 key=3;
				 //key_x=key;
				delay_ms(20);
				//break;
		}else if(tp_dev.x[0]<=290&&tp_dev.x[0]>=190&&tp_dev.y[0]<=705&&tp_dev.y[0]>=655){//按下RGB键
				 key=2;
				 //key_x=key;
				delay_ms(20);
				//break;
		}
					
		if(key){
			if(last_key_enter!=255){
				if(last_key_enter==key){
					key=0;
				}else{
					last_key_enter=key;
				}
			}else{
				last_key_enter=key;
			}
		}						
	}else if(last_key_enter){
		last_key_enter=0;
	}
	
	return key;
}

//1:LIGHT
//2:PHOTO
//3:TEMPERATURE
u8 get_keynum_main()//主界面获取按键信息
{  
    static u8 last_key_main=255;//0
	u8 key=0;//表示没按
    
	tp_dev.scan(0); 
	if(tp_dev.sta&TP_PRES_DOWN){		//触摸屏被按下
		if(tp_dev.x[0]<=145&&tp_dev.x[0]>=45&&tp_dev.y[0]<=705&&tp_dev.y[0]>=655){//按下LIGHT键
				key=1;
				//key_x=key;
				delay_ms(20);
				
			 // break;
		}else if(tp_dev.x[0]<=435&&tp_dev.x[0]>=335&&tp_dev.y[0]<=705&&tp_dev.y[0]>=655){//按下TEMPERATURE键
				 key=3;
				 //key_x=key;
				delay_ms(20);
				//break;
		}else if(tp_dev.x[0]<=290&&tp_dev.x[0]>=190&&tp_dev.y[0]<=705&&tp_dev.y[0]>=655){//按下PHOTO键
				 key=2;
				 //key_x=key;
				delay_ms(20);
				//break;
		}
					
		if(key){
			if(last_key_main!=255){
				if(last_key_main==key){
					key=0;
				}else{
					last_key_main=key;
				}
			}else{
				last_key_main=key;
			}
		}						
	}else if(last_key_main){
		last_key_main=0;
	}
	
	return key;
}

void passwordIndex(){//图像检测报警系统主界面
    u8 i=0;
		
		u32 input_pwd=0;
		
     while(1){
			 if(i==30){
					LED1=!LED1;
					i=0;
			 }
			 i++;
			 
				//delay_ms(10);
				key_index=py_get_keynum();
				if(key_index){
					
					//LCD_ShowNum(50,650,key_index,3,16);
					//LCD_ShowString(50,700,100,20,16,"sadfsad");
					
					if(key_index==12){//删除
						/*if(password_length)password_length--;
						inputstr[password_length]='\0';*/
						password_length=0;
						inputstr[0]='\0';
						input_pwd=0;
						LCD_Fill(192,118,288,148,WHITE);
						LCD_Fill(137,163,387,180,WHITE);
					}else if(key_index==11){//登录
						if(input_pwd==password){
							LCD_Clear(WHITE);
							break;
						}else{
                            LCD_Fill(137,163,387,180,WHITE);
                            POINT_COLOR=RED;
							LCD_ShowString(140,165,250,20,16,"password is wrong!!");
							POINT_COLOR=BROWN;
                        }
					}else if(key_index==10){//如果按下的是0
						if(password_length<6){
							input_pwd=input_pwd*10+0;
							inputstr[password_length]=key_index-10+'0';
							inputstr[password_length+1]='\0';
							password_length++;
						}else{//输入超过6位
							POINT_COLOR=RED;
							LCD_ShowString(140,165,250,20,16,"password is out-length!!");
							POINT_COLOR=BROWN;
						}
					}else{//如果按下的是1-9
						if(password_length<6){
							input_pwd=input_pwd*10+key_index;
							inputstr[password_length]=key_index+'0';
							inputstr[password_length+1]='\0';
							password_length++;
						}else{//输入超过6位
							POINT_COLOR=RED;
							LCD_ShowString(140,165,250,20,16,"password is out-length!!");
							POINT_COLOR=BROWN;
						}
					}
				}

				//LCD_ShowNum(50,600,password_length,3,16);
				
				
				
				
				
				LCD_ShowString(220,125,100,20,16,inputstr);
				
				/*for(i=0;i<password_length;i++){
					LCD_ShowString(140+i*20,135,20,20,16,&inputstr[i]);
				}*/
				

	}
}

 int main(void)
{	 
    //u8 t=0;
	u16 alert_photo_num;//存储的警报图片的数量
    short temp;//存储温度
	
	u8 sensor=0;
	u8 key;
 	u8 i=0;	    
	u8 msgbuf[15];//消息缓存区
	u8 tm=0;
	u8 lightmode=0,effect=0;
	s8 saturation=0,brightness=0,contrast=0;
	 
	delay_init();	    	 	//延时函数初始化	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	uart_init(115200);	 		//串口初始化为 115200
 	usmart_dev.init(72);		//初始化USMART		
 	LED_Init();		  			//初始化与LED连接的硬件接口
	KEY_Init();					//初始化按键
	LCD_Init();			   		//初始化LCD  
	FSMC_SRAM_Init(); //初始化外部 SRAM
    T_Adc_Init();		  		//ADC初始化	  
    tp_dev.init();              //触摸屏初始化
	BEEP_Init();//初始化蜂鸣器端口
	BEEP=0;
	Lsens_Init(); //初始化光敏传感器
	TPAD_Init(6);				//触摸按键初始化 
 	POINT_COLOR=BLACK;			//设置字体为黑色 
    my_mem_init(SRAMIN);		//初始化内部内存池
	exfuns_init();				//为fatfs相关变量申请内存  
 	f_mount(fs[0],"0:",1); 		//挂载SD卡
	
    show_picture();//显示开机界面
    delay_ms(9000);
    
    
    LCD_Clear(WHITE);
    
	touch_gui_login();//显示登录界面
	passwordIndex();//登录界面交互
	
	LCD_ShowString(30,110,200,16,16,"WELCOME TO USE THE SYSTEM"); 
	LCD_ShowString(100,130,400,16,16,"--MADE BY WangYiFan,LiYuanBo,LuTianYuan");
	LCD_ShowString(30,150,200,16,16,"PRESS KEY0 TO START");
	while(KEY_Scan(0)!=KEY0_PRES){}
	POINT_COLOR=BLACK;
    LCD_ShowString(30,230,200,16,16,"OV7725_OV7670 Init...");
            
    //检测OV7725摄像头
    if(OV7725_Init()==0)
    {                                                                                          
        sensor=OV7725;
        LCD_ShowString(30,230,200,16,16,"OV7725 Init OK       ");
        OV7725_Window_Set(OV7725_WINDOW_WIDTH,OV7725_WINDOW_HEIGHT,0);//QVGA模式输出			
        OV7725_Light_Mode(lightmode);
        OV7725_Color_Saturation(saturation);
        OV7725_Brightness(brightness);
        OV7725_Contrast(contrast);
        OV7725_Special_Effects(effect);
        OV7725_CS=0;
    }else{
        POINT_COLOR=RED;        
        LCD_ShowString(30,230,200,16,16,"OV7725_OV7670 Error!!");
        POINT_COLOR=BROWN;
    }
	
    touch_gui_enter();
    while(1)
    {
        i++;
        if(i==100)LCD_ShowString(30,250,210,16,16,"PLEASE CHOOSE MODE"); //闪烁显示提示信息
        if(i==200)
        {	
            LCD_Fill(30,250,210,250+16,WHITE);
            i=0; 
        }
        delay_ms(5);
        
        //触屏检测
        key_index=get_keynum_enter();
        if(key_index){
            if(key_index==1){//按下GRAY键
                check_mode=1;
                delay_ms(1000);
                break;
            }else if(key_index==3){//按下AUTO键
                check_mode=3;
                delay_ms(1000);
                break;
            }else if(key_index==2){//按下RGB键
                check_mode=2;
                delay_ms(1000);
                break;
            }
        }
        
    }

    
	TIM6_Int_Init(10000,7199);	//10Khz计数频率,1秒钟中断									  
	EXTI8_Init();				//使能定时器捕获				
	LCD_Clear(BLACK);
    BACK_COLOR=BLACK;
        
 	while(1)
	{	
		key=KEY_Scan(0);//不支持连按
		if(key)
		{
			tm=20;
			switch(key)
			{				    
				case KEY0_PRES:	//灯光模式Light Mode
					lightmode++;
					if(sensor==OV7725)
					{
						if(lightmode>5)lightmode=0;
						OV7725_Light_Mode(lightmode);
					}
					else if(sensor==OV7670)
					{
						if(lightmode>4)lightmode=0;
						OV7670_Light_Mode(lightmode);
					}
					sprintf((char*)msgbuf,"%s",LMODE_TBL[lightmode]);
					break;
				case KEY1_PRES:	//饱和度Saturation
					saturation++;
					if(sensor==OV7725)
					{
						if(saturation>4)saturation=-4;
						else if(saturation<-4)saturation=4;
						OV7725_Color_Saturation(saturation);
					}
					else if(sensor==OV7670)
					{
						if(saturation>2)saturation=-2;
						else if(saturation<-2)saturation=2;
						OV7670_Color_Saturation(saturation);
					}
					sprintf((char*)msgbuf,"Saturation:%d",saturation);
					break;
				case KEY2_PRES:	//亮度Brightness				 
					brightness++;
					if(sensor==OV7725)
					{
						if(brightness>4)brightness=-4;
						else if(brightness<-4)brightness=4;
						OV7725_Brightness(brightness);
					}
					else if(sensor==OV7670)
					{
						if(brightness>2)brightness=-2;
						else if(brightness<-2)brightness=2;
						OV7670_Brightness(brightness);
					}
					sprintf((char*)msgbuf,"Brightness:%d",brightness);
					break;
				case WKUP_PRES:	//对比度Contrast			    
					contrast++;
					if(sensor==OV7725)
					{
						if(contrast>4)contrast=-4;
						else if(contrast<-4)contrast=4;
						OV7725_Contrast(contrast);
					}
					else if(sensor==OV7670)
					{
						if(contrast>2)contrast=-2;
						else if(contrast<-2)contrast=2;
						OV7670_Contrast(contrast);
					}
					sprintf((char*)msgbuf,"Contrast:%d",contrast);
					break;
			}
		}	 
		if(TPAD_Scan(0))//检测到触摸按键 
		{
			show_alert_photo();
		}
        
        //触屏检测
        key_index=get_keynum_main();
        if(key_index){
            if(key_index==1){//按下LIGHT键
                light_flag=!light_flag;
                //LCD_ShowNum(20,240,light_flag,1,16);
            }else if(key_index==3){//按下TEMPERATURE键
                temp_flag=!temp_flag;
                 //LCD_ShowNum(20,260,temp_flag,1,16);
            }else if(key_index==2){//按下PHOTO键
                show_alert_photo();
            }
        }
               
        light_strong=Lsens_Get_Val();
		
		if(sensor==OV7725)
			OV7725_camera_refresh();//更新显示
		else if(sensor==OV7670)
			OV7670_camera_refresh();//更新显示
		
        touch_gui_main();//显示触摸按键
        
		POINT_COLOR=WHITE;
        
        //LCD_ShowNum(240,600,check_mode,1,16);
        
        //显示模式
        switch(check_mode){
            case 1:LCD_ShowString(200,50,200,16,16,"MODE:GRAY");break;
            case 2:LCD_ShowString(200,50,200,16,16,"MODE:RGB");break;
            case 3:LCD_ShowString(200,50,200,16,16,"MODE:AUTO");break;
        }
        
        //显示光照强度
        if(light_flag==1){
            LCD_ShowString(200,70,45,16,16,"LIGHT:"); 
            LCD_ShowxNum(245,70,Lsens_Get_Val(),3,16,0);
            if(Lsens_Get_Val()<20)
            {
                LCD_Fill(200,90,400,110+16,BLACK);
                LCD_ShowString(200,90,200,16,16,"TOO DARK!!");
            }else if(Lsens_Get_Val()>80)
            {
                LCD_Fill(200,90,400,110+16,BLACK);
                LCD_ShowString(200,90,200,16,16,"TOO BRIGHT!!");
            }else
            {
                LCD_Fill(200,90,400,110+16,BLACK);
                LCD_ShowString(200,90,200,16,16,"LIGHT IS SUITABLE");
            }
        }else{
            LCD_Fill(200,70,300,86,BLACK);
            LCD_Fill(200,90,400,126,BLACK);
        }
		
        //显示温度
        if(temp_flag==1){
            LCD_ShowString(200,110,100,16,16,"TEMPERATURE:"); 
            temp=Get_Temprate();	//得到温度值 
            if(temp<0)
            {
                temp=-temp;
                LCD_ShowString(300,110,10,16,16,"-");	//显示负号
            }else LCD_ShowString(300,110,10,16,16,"  ");	//无符号		
            LCD_ShowxNum(310,110,temp/100,2,16,0);		//显示整数部分
            LCD_ShowString(325,110,10,16,16,".");
            LCD_ShowxNum(330,110,temp%100,2,16, 0X80);	//显示小数部分
        }else{
            LCD_Fill(200,110,400,110,BLACK);
        }
        
		LCD_ShowString(30,90,48,16,16,"SAVED:"); 
		STMFLASH_Read(PHOTO_START_ADDRESS,&alert_photo_num,1);
		if(alert_photo_num<=ALERT_PHOTO_MAX_NUM){
           LCD_ShowNum(78,90,alert_photo_num,1,16);
        }else{
           LCD_ShowNum(78,90,0,1,16);
        }
		
		
		LCD_Fill(30,70,130,86,BLACK);
		LCD_Fill(30,110,130,126,BLACK);
		if(person_check_mode()==1)//如果检测到有人
		{
			POINT_COLOR=RED;
			LCD_ShowString(30,70,100,16,16,"ALERT!!"); 
			LCD_ShowString(30,110,100,16,16,"WRITING......"); 
			BEEP=1;
            delay_ms(300);
            BEEP=0;
            LED0=!LED0;
			write_alert_photo();	
            LED0=!LED0;           
		}else
		{
			POINT_COLOR=RED;
			LCD_ShowString(30,70,100,16,16,"NO ALERT!!"); 
		}
		
 		if(tm)
		{
			LCD_ShowString((lcddev.width-240)/2+30,(lcddev.height-320)/2+60,200,16,16,msgbuf);
			tm--;
		}
		i++;
		if(i>=15)//DS0闪烁.
		{
			i=0;
			LED1=!LED1;
 		}
	}
	
}













