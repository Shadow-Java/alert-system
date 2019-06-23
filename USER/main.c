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

//����OV7725��������װ��ʽԭ��,OV7725_WINDOW_WIDTH�൱��LCD�ĸ߶ȣ�OV7725_WINDOW_HEIGHT�൱��LCD�Ŀ��
//ע�⣺�˺궨��ֻ��OV7725��Ч
#define  OV7725_WINDOW_WIDTH		320 // <=320
#define  OV7725_WINDOW_HEIGHT		240 // <=240

#define ALERT_PHOTO_MAX_NUM 2//�������FLASH�д漸������ͼƬ
#define PHOTO_START_ADDRESS 0X080347FE//��FLASH�д���Ƭ����ʼ��ַ

const u8*LMODE_TBL[6]={"Auto","Sunny","Cloudy","Office","Home","Night"};//6�ֹ���ģʽ	    
const u8*EFFECTS_TBL[7]={"Normal","Negative","B&W","Redish","Greenish","Bluish","Antique"};	//7����Ч 
extern u8 ov_sta;	//��exit.c�� �涨��
extern u8 ov_frame;	//��timer.c���涨�� 

u8 check_flag=3;//��ʾ�Ƿ�Ϊ��һ�μ��,����֡ǰ��û��֡

u8 pixel_R;//ĳ���ص��Rֵ
u8 pixel_G;//ĳ���ص��Gֵ
u8 pixel_B;//ĳ���ص��Bֵ
u8 last_frame_pixel;//��һ֡ĳ���ص�ĻҶ�ֵ
u8 now_frame_pixel;//��ǰ֡ĳ���ص�ĻҶ�ֵ
u32 pixel_change_num=0;//��¼�仯�����ص���Ŀ

u16 past_photo_num=0;//��¼���γ��������Ѿ����˶�����ͼƬ

u8 check_mode=1;//���ģʽ,1Ϊ�Ҷȼ��,2ΪRGB���,3Ϊ�Զ�

u8 light_flag=0;//1��ʾ����ǿ��,0����ʾ
u8 temp_flag=0;//1��ʾ�¶���Ϣ,0����ʾ
u8 light_strong;//��¼����ǿ��
 
//u8 last_frame[OV7725_WINDOW_HEIGHT][OV7725_WINDOW_WIDTH]__attribute__((at(0X68000000)));//�洢��һ֡ͼ��,������ⲿSRAM��
u16 last_frame[OV7725_WINDOW_HEIGHT][OV7725_WINDOW_WIDTH]__attribute__((at(0X68000000)));//�洢��һ֡ͼ��,������ⲿSRAM��

u16 mycolor;//������
u16 last_one;//������

u8 key_index;//���밴��
u8 password_length=0;//���볤�� ��λ
u8 inputstr[7];//�������6���ַ�
u8 cur_index=120;//����
u32 password=123456;

u8 res;
DIR picdir;	 		//ͼƬĿ¼
FILINFO picfileinfo;//�ļ���Ϣ
u8 *fn;   			//���ļ���
u8 *pname;			//��·�����ļ���
u16 curindey;		//ͼƬ��ǰ����
u16 *picindextbl; //ͼƬ������
u16 totpicnum; //ͼƬ�ļ�����
u8 res; u8 t;u16 temp;
	

void show_picture(){//�������ͼ�ν���
 
	POINT_COLOR=RED;
	 while(f_opendir(&picdir,"0:/PICTURE"))//��ͼƬ�ļ���
 	{	    
		Show_Str(30,170,240,16,"PICTURE IS WRONG!",16,0);
		delay_ms(200);				  
		LCD_Fill(30,170,240,186,WHITE);//�����ʾ	     
		delay_ms(200);				  
	}  
	picfileinfo.lfsize=_MAX_LFN*2+1;						//���ļ�����󳤶�
	picfileinfo.lfname=mymalloc(SRAMIN,picfileinfo.lfsize);	//Ϊ���ļ������������ڴ�
 	pname=mymalloc(SRAMIN,picfileinfo.lfsize);				//Ϊ��·�����ļ��������ڴ�
	//picindextbl=mymalloc(SRAMIN,2*totpicnum);				//����2*totpicnum���ֽڵ��ڴ�,���ڴ��ͼƬ����
	while(picfileinfo.lfname==NULL||pname==NULL)//�ڴ�������
 	{	    
		Show_Str(30,170,240,16,"memory share wrong!",16,0);//�ڴ����ʧ��
		delay_ms(200);				  
		LCD_Fill(30,170,240,186,WHITE);//�����ʾ	     
		delay_ms(200);				  
	} 
	//��¼����
    res=f_opendir(&picdir,"0:/PICTURE"); //��Ŀ¼
	if(res==FR_OK)
	{
		curindey=0;//��ǰ����Ϊ0
		while(1)//ȫ����ѯһ��
		{
			temp=picdir.index;								//��¼��ǰindex
			res=f_readdir(&picdir,&picfileinfo);       		//��ȡĿ¼�µ�һ���ļ�
			if(res!=FR_OK||picfileinfo.fname[0]==0)break;	//������/��ĩβ��,�˳�		  
			fn=(u8*)(*picfileinfo.lfname?picfileinfo.lfname:picfileinfo.fname);			 
			res=f_typetell(fn);	
			if((res&0XF0)==0X50)//ȡ����λ,�����ǲ���ͼƬ�ļ�	
			{
				
			}	    
		} 
	}   
	/*Show_Str(30,170,480,16,"Image detection and alarm system is starting ...",16,0); 
	delay_ms(1500);*/
	piclib_init();										//��ʼ����ͼ	   	   
	curindey=0;											//��0��ʼ��ʾ
	res=f_opendir(&picdir,(const TCHAR*)"0:/PICTURE"); 	//��Ŀ¼
	while(res==FR_OK)//�򿪳ɹ�
	{	
	   //dir_sdi(&picdir,picindextbl[curindey]);			//�ı䵱ǰĿ¼����
		res=f_readdir(&picdir,&picfileinfo);       		//��ȡĿ¼�µ�һ���ļ�
		if(res!=FR_OK||picfileinfo.fname[0]==0)break;	//������/��ĩβ��,�˳�
		fn=(u8*)(*picfileinfo.lfname?picfileinfo.lfname:picfileinfo.fname);
		strcpy((char*)pname,"0:/PICTURE/");				//����·��(Ŀ¼)
		strcat((char*)pname,(const char*)fn);  			//���ļ������ں���
 		LCD_Clear(BLACK);
 		ai_load_picfile(pname,0,0,lcddev.width,lcddev.height,1);//��ʾͼƬ    
		//Show_Str(2,2,240,16,pname,16,1); 				//��ʾͼƬ����
	}
}

//1:<<<��
//2:EXIT��
//3:>>>��
u8 get_keynum_show()//��ʾ�����ȡ������Ϣ
{  
    static u8 last_key_show=255;//0
	u8 key=0;//��ʾû��
    
	tp_dev.scan(0); 
	if(tp_dev.sta&TP_PRES_DOWN){		//������������
		if(tp_dev.x[0]<=145&&tp_dev.x[0]>=45&&tp_dev.y[0]<=705&&tp_dev.y[0]>=655){//����<<<��
				key=1;
				//key_x=key;
				delay_ms(20);
				
			 // break;
		}else if(tp_dev.x[0]<=435&&tp_dev.x[0]>=335&&tp_dev.y[0]<=705&&tp_dev.y[0]>=655){//����>>>��
				 key=3;
				 //key_x=key;
				delay_ms(20);
				//break;
		}else if(tp_dev.x[0]<=290&&tp_dev.x[0]>=190&&tp_dev.y[0]<=705&&tp_dev.y[0]>=655){//����EXIT��
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
void touch_gui_login(){//��½���水�� ��Ϊx���� ��Ϊy����
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

void is_change_mode(u16 last_color,u16 now_color)//���ģʽ,1Ϊ�Ҷȼ��,2ΪRGB���,3Ϊ�Զ�
{
    u8 now_R,now_G,now_B;
    switch(check_mode)
    {
        case 1:{//���ڻҶ�ͼ����
            pixel_R=now_color>>11;
            pixel_G=(now_color&0x07E0)>>5;
            pixel_B=now_color&0x001F;
            now_frame_pixel=(u8)((pixel_R*76+pixel_G*150+pixel_B*30)>>8);//��ǰ֡�����ص�ĻҶ�ֵ	
            
            pixel_R=last_color>>11;
            pixel_G=(last_color&0x07E0)>>5;
            pixel_B=last_color&0x001F;
            last_frame_pixel=(u8)((pixel_R*76+pixel_G*150+pixel_B*30)>>8);//��һ֡�����ص�ĻҶ�ֵ	
            
            if((now_frame_pixel-last_frame_pixel)>=25||(last_frame_pixel-now_frame_pixel)>=25)//�������ص�Ҷ�ֵ�仯����20
                pixel_change_num++;//�仯�����ص���Ŀ+1
            break;
        }
        
        case 2:{//����RGBͼ����
            now_R=now_color>>11;
            now_G=(now_color&0x07E0)>>5;
            now_B=now_color&0x001F;
            
            pixel_R=last_color>>11;
            pixel_G=(last_color&0x07E0)>>5;
            pixel_B=last_color&0x001F;
            
            //R��Bֵ�仯����5��Gֵ�仯����7���϶������仯
            if((now_R-pixel_R>=8||pixel_R-now_R>=8)&&(now_G-pixel_G>=14||pixel_G-now_G>=14)&&(now_B-pixel_B>=8||pixel_B-now_B>=8))
                pixel_change_num++;//�仯�����ص���Ŀ+1
            break;
        }
        
        case 3:{//�Զ�
            if(light_strong>=20){//��������������ûҶ�ͼ����
                pixel_R=now_color>>11;
                pixel_G=(now_color&0x07E0)>>5;
                pixel_B=now_color&0x001F;
                now_frame_pixel=(u8)((pixel_R*76+pixel_G*150+pixel_B*30)>>8);//��ǰ֡�����ص�ĻҶ�ֵ	
                
                pixel_R=last_color>>11;
                pixel_G=(last_color&0x07E0)>>5;
                pixel_B=last_color&0x001F;
                last_frame_pixel=(u8)((pixel_R*76+pixel_G*150+pixel_B*30)>>8);//��һ֡�����ص�ĻҶ�ֵ	
                
                if((now_frame_pixel-last_frame_pixel)>=25||(last_frame_pixel-now_frame_pixel)>=25)//�������ص�Ҷ�ֵ�仯����20
                    pixel_change_num++;//�仯�����ص���Ŀ+1
                break;
            }else{//������ȹ�������RGB���
                now_R=now_color>>11;
                now_G=(now_color&0x07E0)>>5;
                now_B=now_color&0x001F;
                
                pixel_R=last_color>>11;
                pixel_G=(last_color&0x07E0)>>5;
                pixel_B=last_color&0x001F;
                
                //R��Bֵ�仯����5��Gֵ�仯����7���϶������仯
                if((now_R-pixel_R>=5||pixel_R-now_R>=5)&&(now_G-pixel_G>=7||pixel_G-now_G>=7)&&(now_B-pixel_B>=5||pixel_B-now_B>=5))
                    pixel_change_num++;//�仯�����ص���Ŀ+1
                break;
            }
        }
    }
}

void is_change(u16 last_color,u16 now_color)//�ж�ĳ���ص��Ƿ����仯
{
	/*pixel_R=last_color>>11;
	pixel_G=(last_color&0x07E0)>>5;
	pixel_B=last_color&0x001F;
	last_frame_pixel=(u8)((pixel_R*76+pixel_G*150+pixel_B*30)>>8);*/
	
	pixel_R=now_color>>11;
	pixel_G=(now_color&0x07E0)>>5;
	pixel_B=now_color&0x001F;
	now_frame_pixel=(u8)((pixel_R*76+pixel_G*150+pixel_B*30)>>8);//��ǰ֡�����ص�ĻҶ�ֵ	
	
	pixel_R=last_color>>11;
	pixel_G=(last_color&0x07E0)>>5;
	pixel_B=last_color&0x001F;
	last_frame_pixel=(u8)((pixel_R*76+pixel_G*150+pixel_B*30)>>8);//��һ֡�����ص�ĻҶ�ֵ	
	
	if((now_frame_pixel-last_frame_pixel)>=20||(last_frame_pixel-now_frame_pixel)>=20)//�������ص�Ҷ�ֵ�仯����20
		pixel_change_num++;//�仯�����ص���Ŀ+1

}

void write_alert_photo()//����ǰͼ�񴢴���FLASH��
{
	u32 start_secpos;	   //��ʼ������ַ
	
	u32 i;
	u16 alert_photo_num;//�洢�ľ���ͼƬ������
	u32 start_address;//����ַ

	STMFLASH_Read(PHOTO_START_ADDRESS,&alert_photo_num,1);//��FLASH�ж����洢�ľ���ͼƬ������
	if(alert_photo_num>ALERT_PHOTO_MAX_NUM){//���֮ǰû�д��ͼƬ
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
	
	start_secpos=(start_address-STM32_FLASH_BASE)/2048;//�����ʼ������ַ
		
	FLASH_Unlock();
	
	//��������
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

void show_alert_photo()//��ʾ�洢��FLASH�еľ���ͼƬ
{
	u32 i,j;
	//u16 color[OV7725_WINDOW_WIDTH];//�洢�����ص��ֵ
	u16 color;//�洢�����ص��ֵ
	u16 alert_photo_num;//�洢�ľ���ͼƬ������
	u8 now_num=0;//���ڲ��ŵڼ��ž���ͼƬ
	u8 key;
	u32 start_address;//����ַ
	
	LCD_Clear(BLACK);
	
	STMFLASH_Read(PHOTO_START_ADDRESS,&alert_photo_num,1);//��FLASH�ж����洢�ľ���ͼƬ������
	
	if(alert_photo_num>ALERT_PHOTO_MAX_NUM){//���֮ǰû�д��ͼƬ
		alert_photo_num=0;
		STMFLASH_Write(PHOTO_START_ADDRESS,&alert_photo_num,1);
	}
	/*LCD_ShowNum(30,170,alert_photo_num,10,16);
	STMFLASH_Write(PHOTO_START_ADDRESS,&alert_photo_num,1);
	STMFLASH_Read(PHOTO_START_ADDRESS,&alert_photo_num,1);//��FLASH�ж����洢�ľ���ͼƬ������
	LCD_ShowNum(30,190,alert_photo_num,10,16);*/
	while(1)
	{
		if(alert_photo_num==0){//���û��ͼƬ
			POINT_COLOR=RED;			//��������Ϊ��ɫ 
			LCD_ShowString(30,110,200,16,16,"NO PHOTO"); 
		}else{
			LCD_Scan_Dir(U2D_L2R);//���ϵ���,������
			LCD_Set_Window((lcddev.width-OV7725_WINDOW_WIDTH)/2,(lcddev.height-OV7725_WINDOW_HEIGHT)/2,OV7725_WINDOW_WIDTH,OV7725_WINDOW_HEIGHT);//����ʾ�������õ���Ļ����
			if(lcddev.id==0X1963)
				LCD_Set_Window((lcddev.width-OV7725_WINDOW_WIDTH)/2,(lcddev.height-OV7725_WINDOW_HEIGHT)/2,OV7725_WINDOW_HEIGHT,OV7725_WINDOW_WIDTH);//����ʾ�������õ���Ļ����
			LCD_WriteRAM_Prepare();     //��ʼд��GRAM	
					
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
			LCD_Scan_Dir(DFT_SCAN_DIR);	//�ָ�Ĭ��ɨ�跽�� 
            POINT_COLOR=BLUE;
			LCD_ShowString(30,110,40,16,16,"PHOTO");
			LCD_ShowNum(70,110,now_num+1,1,16);
            POINT_COLOR=BROWN;
            LCD_ShowString(30,130,100,16,16,"KEY0:NEXT");
            LCD_ShowString(30,150,100,16,16,"KEY2:LAST");
            LCD_ShowString(30,170,100,16,16,"KEY1:EXIT");
		}
        touch_gui_show();
		//KEY1 �˳�
		//KEY0 ��һ��ͼƬ
		//KEY2 ��һ��ͼƬ
		key=KEY_Scan(0);
		if(key==KEY1_PRES){//�˳�
			LCD_Clear(BLACK);
            check_flag=3;
			break;
		}else if(key==KEY0_PRES){//��һ��
			if(now_num==alert_photo_num-1){
				now_num=0;
			}else{
				now_num++;
			}
		}else if(key==KEY2_PRES){//��һ��
			if(now_num==0){
				now_num=alert_photo_num-1;
			}else{
				now_num--;
			}
		}
        
        //�������
        key_index=get_keynum_show();
        if(key_index){
            if(key_index==1){//����<<<��
                if(now_num==0){
                    now_num=alert_photo_num-1;
                }else{
                    now_num--;
                }
            }else if(key_index==3){//����>>>��
                if(now_num==alert_photo_num-1){
                    now_num=0;
                }else{
                    now_num++;
                }
            }else if(key_index==2){//����EXIT��
                LCD_Clear(BLACK);
                check_flag=3;
                break;
            }
        }
        
	}
}

//����LCD��ʾ(OV7725)
void OV7725_camera_refresh(void)
{
	u32 i,j;
 	u16 color;	
	
	if(ov_sta)//��֡�жϸ���
	{
		LCD_Scan_Dir(U2D_L2R);//���ϵ���,������
		LCD_Set_Window((lcddev.width-OV7725_WINDOW_WIDTH)/2,(lcddev.height-OV7725_WINDOW_HEIGHT)/2,OV7725_WINDOW_WIDTH,OV7725_WINDOW_HEIGHT);//����ʾ�������õ���Ļ����
		if(lcddev.id==0X1963)
			LCD_Set_Window((lcddev.width-OV7725_WINDOW_WIDTH)/2,(lcddev.height-OV7725_WINDOW_HEIGHT)/2,OV7725_WINDOW_HEIGHT,OV7725_WINDOW_WIDTH);//����ʾ�������õ���Ļ����
		LCD_WriteRAM_Prepare();     //��ʼд��GRAM	
		OV7725_RRST=0;				//��ʼ��λ��ָ�� 
		OV7725_RCK_L;
		OV7725_RCK_H;
		OV7725_RCK_L;
		OV7725_RRST=1;				//��λ��ָ����� 
		OV7725_RCK_H; 
		
		pixel_change_num=0;
		
		for(i=0;i<OV7725_WINDOW_HEIGHT;i++)
		{
			for(j=0;j<OV7725_WINDOW_WIDTH;j++)
			{
				OV7725_RCK_L;
				color=GPIOC->IDR&0XFF;	//������
				OV7725_RCK_H; 
				color<<=8;  
				OV7725_RCK_L;
				color|=GPIOC->IDR&0XFF;	//������
				OV7725_RCK_H; 
				
				mycolor=color;
				
				/*pixel_R=color>>11;
				pixel_G=(color&0x07E0)>>5;
				pixel_B=color&0x001F;
				now_frame_pixel=(u8)((pixel_R*76+pixel_G*150+pixel_B*30)>>8);//��ǰ֡�����ص�ĻҶ�ֵ		*/		
				
				is_change_mode(last_frame[i][j],color);
				last_frame[i][j]=color;
				last_one=last_frame[i][j];
				
				LCD->LCD_RAM=color;										
			}
		}
		
 		ov_sta=0;					//����֡�жϱ��
		ov_frame++; 
		LCD_Scan_Dir(DFT_SCAN_DIR);	//�ָ�Ĭ��ɨ�跽�� 
	} 
}

//����LCD��ʾ(OV7670)
void OV7670_camera_refresh(void)
{
	u32 j;
 	u16 color;	 
	if(ov_sta)//��֡�жϸ���
	{
		LCD_Scan_Dir(U2D_L2R);//���ϵ���,������  
		if(lcddev.id==0X1963)LCD_Set_Window((lcddev.width-240)/2,(lcddev.height-320)/2,240,320);//����ʾ�������õ���Ļ����
		else if(lcddev.id==0X5510||lcddev.id==0X5310)LCD_Set_Window((lcddev.width-320)/2,(lcddev.height-240)/2,320,240);//����ʾ�������õ���Ļ����
		LCD_WriteRAM_Prepare();     //��ʼд��GRAM	
		OV7670_RRST=0;				//��ʼ��λ��ָ�� 
		OV7670_RCK_L;
		OV7670_RCK_H;
		OV7670_RCK_L;
		OV7670_RRST=1;				//��λ��ָ����� 
		OV7670_RCK_H;
		for(j=0;j<76800;j++)
		{
			OV7670_RCK_L;
			color=GPIOC->IDR&0XFF;	//������
			OV7670_RCK_H; 
			color<<=8;  
			OV7670_RCK_L;
			color|=GPIOC->IDR&0XFF;	//������
			OV7670_RCK_H; 
			LCD->LCD_RAM=color;    
		}   							  
 		ov_sta=0;					//����֡�жϱ��
		ov_frame++; 
		LCD_Scan_Dir(DFT_SCAN_DIR);	//�ָ�Ĭ��ɨ�跽�� 
	} 
}

//����Ƿ�����,����0��ʾû��,1��ʾ����
u8 person_check_mode()
{
    switch(check_mode)
    {
        case 1:{//���ڻҶ�ͼ����
            if(check_flag==0)//������ǵ�һ֡
            {
                if(pixel_change_num>=3000)//�仯�����ص���Ŀ����3000
                {
                    pixel_change_num=0;
                    return 1;
                }else{
                    pixel_change_num=0;
                    return 0;
                }
            }else//����ǵ�һ֡
            {
                check_flag--;
                pixel_change_num=0;
                return 0;
            }
        }
        
        case 2:{//����RGBͼ����
            if(check_flag==0)//������ǵ�һ֡
            {
                if(pixel_change_num>=6000)//�仯�����ص���Ŀ����6000
                {
                    pixel_change_num=0;
                    return 1;
                }else{
                    pixel_change_num=0;
                    return 0;
                }
            }else//����ǵ�һ֡
            {
                check_flag--;
                pixel_change_num=0;
                return 0;
            }
        }
        
        case 3:{//�Զ����
            if(light_strong>=20){//��������
                if(check_flag==0)//������ǵ�һ֡
                {
                    if(pixel_change_num>=3000)//�仯�����ص���Ŀ����3000
                    {
                        pixel_change_num=0;
                        return 1;
                    }else{
                        pixel_change_num=0;
                        return 0;
                    }
                }else//����ǵ�һ֡
                {
                    check_flag--;
                    pixel_change_num=0;
                    return 0;
                }               
            }else{//���ȹ���
                if(check_flag==0)//������ǵ�һ֡
                {
                    if(pixel_change_num>=6000)//�仯�����ص���Ŀ����6000
                    {
                        pixel_change_num=0;
                        return 1;
                    }else{
                        pixel_change_num=0;
                        return 0;
                    }
                }else//����ǵ�һ֡
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

u8 person_check()//����Ƿ�����,����0��ʾû��,1��ʾ����
{
	if(check_flag==0)//������ǵ�һ֡
	{
		if(pixel_change_num>=3000)//�仯�����ص���Ŀ����19200
		{
			pixel_change_num=0;
			return 1;
		}else{
			pixel_change_num=0;
			return 0;
		}
	}else//����ǵ�һ֡
	{
		check_flag--;
		pixel_change_num=0;
		return 0;
	}
}
//����״̬����
//x,y:��������
//key:��ֵ��0~8��
//sta:״̬��0���ɿ���1�����£�
/*void py_key_staset(u16 x,u16 y,u8 keyx,u8 sta)
{		  
	u16 i=keyx/3,j=keyx%3;
	if(keyx>8)return;
	if(sta)LCD_Fill(x+j*kbdxsize+1,y+i*kbdysize+1,x+j*kbdxsize+kbdxsize-1,y+i*kbdysize+kbdysize-1,GREEN);
	else LCD_Fill(x+j*kbdxsize+1,y+i*kbdysize+1,x+j*kbdxsize+kbdxsize-1,y+i*kbdysize+kbdysize-1,WHITE); 
	Show_Str_Mid(x+j*kbdxsize,y+4+kbdysize*i,(u8*)kbd_tbl[keyx],16,kbdxsize);		
	Show_Str_Mid(x+j*kbdxsize,y+kbdysize/2+kbdysize*i,(u8*)kbs_tbl[keyx],16,kbdxsize);		 
}*/
/*void py_key_staset(u16 x1,u16 y1,u16 x2,u16 y2,u8 keyx,u8 sta){//����״̬����
    if(keyx>12)return;
		if(sta)LCD_Fill(x+j*60+1,y+i*40+1,x+j*60+59,y+i*40+39,GREEN);
		else LCD_Fill(x+j*60+1,y+i*40+1,x+j*60+59,y+i*40+39,WHITE);
}*/
u8 py_get_keynum(){//��ü�ֵ

	 //u8 real_key;
	static u8 key_x=255;//0,û���κΰ������£�1~9��1~9�Ű�������
	u8 key=0;//��ʾû��
	tp_dev.scan(0); 
	if(tp_dev.sta&TP_PRES_DOWN){		//������������
		if(tp_dev.x[0]<=145&&tp_dev.x[0]>=45&&tp_dev.y[0]<=250&&tp_dev.y[0]>=200){//���¡�1��
				key=1;
				//key_x=key;
				delay_ms(20);
				
			 // break;
		}else if(tp_dev.x[0]<=290&&tp_dev.x[0]>=190&&tp_dev.y[0]<=250&&tp_dev.y[0]>=200){//���¡�2��
				 key=2;
				 //key_x=key;
				delay_ms(20);
				//break;
		}else if(tp_dev.x[0]<=435&&tp_dev.x[0]>=335&&tp_dev.y[0]<=250&&tp_dev.y[0]>=200){//���¡�3��
				 key=3;
				 //key_x=key;
				delay_ms(20);
				//break;
		}else if(tp_dev.x[0]<=145&&tp_dev.x[0]>=45&&tp_dev.y[0]<=320&&tp_dev.y[0]>=270){//���¡�4��
				 key=4;
				 //key_x=key;
				delay_ms(20);
			//  break;
		}else if(tp_dev.x[0]<=290&&tp_dev.x[0]>=190&&tp_dev.y[0]<=320&&tp_dev.y[0]>=270){//���¡�5��
				 key=5;
				 //key_x=key;
				delay_ms(20);
			 // break;
			 
		}else if(tp_dev.x[0]<=435&&tp_dev.x[0]>=335&&tp_dev.y[0]<=320&&tp_dev.y[0]>=270){//���¡�6��
				 key=6;
				 //key_x=key;
				delay_ms(20);
				//break;
		}else if(tp_dev.x[0]<=145&&tp_dev.x[0]>=45&&tp_dev.y[0]<=390&&tp_dev.y[0]>=340){//���¡�7��
					key=7;
				 //key_x=key;
				delay_ms(20);
				//break;
		}else if(tp_dev.x[0]<=290&&tp_dev.x[0]>=190&&tp_dev.y[0]<=390&&tp_dev.y[0]>=340){//���¡�8��
				 key=8;
				 //key_x=key;
				delay_ms(20);
			 // break;
		}else if(tp_dev.x[0]<=435&&tp_dev.x[0]>=335&&tp_dev.y[0]<=390&&tp_dev.y[0]>=340){//���¡�9��
				 key=9;
				 //key_x=key;
				delay_ms(20);
			 // break;
		}else if(tp_dev.x[0]<=290&&tp_dev.x[0]>=190&&tp_dev.y[0]<=460&&tp_dev.y[0]>=410){//���¡�0��
				 key=10;
				 //key_x=key;
				delay_ms(20);
			 // break;
		}else if(tp_dev.x[0]<=198&&tp_dev.x[0]>=93&&tp_dev.y[0]<=530&&tp_dev.y[0]>=480){//���¡�LOGIN��
				key=11;
				 //key_x=key;
				delay_ms(20);
				//break;
			
		}else if(tp_dev.x[0]<=387&&tp_dev.x[0]>=282&&tp_dev.y[0]<=530&&tp_dev.y[0]>=480){//���¡�CANCEL��
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
u8 get_keynum_enter()//��¼�����ȡ������Ϣ
{
   
    static u8 last_key_enter=255;//0
	u8 key=0;//��ʾû��
    
	tp_dev.scan(0); 
	if(tp_dev.sta&TP_PRES_DOWN){		//������������
		if(tp_dev.x[0]<=145&&tp_dev.x[0]>=45&&tp_dev.y[0]<=705&&tp_dev.y[0]>=655){//����GRAY��
				key=1;
				//key_x=key;
				delay_ms(20);
				
			 // break;
		}else if(tp_dev.x[0]<=435&&tp_dev.x[0]>=335&&tp_dev.y[0]<=705&&tp_dev.y[0]>=655){//����AUTO��
				 key=3;
				 //key_x=key;
				delay_ms(20);
				//break;
		}else if(tp_dev.x[0]<=290&&tp_dev.x[0]>=190&&tp_dev.y[0]<=705&&tp_dev.y[0]>=655){//����RGB��
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
u8 get_keynum_main()//�������ȡ������Ϣ
{  
    static u8 last_key_main=255;//0
	u8 key=0;//��ʾû��
    
	tp_dev.scan(0); 
	if(tp_dev.sta&TP_PRES_DOWN){		//������������
		if(tp_dev.x[0]<=145&&tp_dev.x[0]>=45&&tp_dev.y[0]<=705&&tp_dev.y[0]>=655){//����LIGHT��
				key=1;
				//key_x=key;
				delay_ms(20);
				
			 // break;
		}else if(tp_dev.x[0]<=435&&tp_dev.x[0]>=335&&tp_dev.y[0]<=705&&tp_dev.y[0]>=655){//����TEMPERATURE��
				 key=3;
				 //key_x=key;
				delay_ms(20);
				//break;
		}else if(tp_dev.x[0]<=290&&tp_dev.x[0]>=190&&tp_dev.y[0]<=705&&tp_dev.y[0]>=655){//����PHOTO��
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

void passwordIndex(){//ͼ���ⱨ��ϵͳ������
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
					
					if(key_index==12){//ɾ��
						/*if(password_length)password_length--;
						inputstr[password_length]='\0';*/
						password_length=0;
						inputstr[0]='\0';
						input_pwd=0;
						LCD_Fill(192,118,288,148,WHITE);
						LCD_Fill(137,163,387,180,WHITE);
					}else if(key_index==11){//��¼
						if(input_pwd==password){
							LCD_Clear(WHITE);
							break;
						}else{
                            LCD_Fill(137,163,387,180,WHITE);
                            POINT_COLOR=RED;
							LCD_ShowString(140,165,250,20,16,"password is wrong!!");
							POINT_COLOR=BROWN;
                        }
					}else if(key_index==10){//������µ���0
						if(password_length<6){
							input_pwd=input_pwd*10+0;
							inputstr[password_length]=key_index-10+'0';
							inputstr[password_length+1]='\0';
							password_length++;
						}else{//���볬��6λ
							POINT_COLOR=RED;
							LCD_ShowString(140,165,250,20,16,"password is out-length!!");
							POINT_COLOR=BROWN;
						}
					}else{//������µ���1-9
						if(password_length<6){
							input_pwd=input_pwd*10+key_index;
							inputstr[password_length]=key_index+'0';
							inputstr[password_length+1]='\0';
							password_length++;
						}else{//���볬��6λ
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
	u16 alert_photo_num;//�洢�ľ���ͼƬ������
    short temp;//�洢�¶�
	
	u8 sensor=0;
	u8 key;
 	u8 i=0;	    
	u8 msgbuf[15];//��Ϣ������
	u8 tm=0;
	u8 lightmode=0,effect=0;
	s8 saturation=0,brightness=0,contrast=0;
	 
	delay_init();	    	 	//��ʱ������ʼ��	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	uart_init(115200);	 		//���ڳ�ʼ��Ϊ 115200
 	usmart_dev.init(72);		//��ʼ��USMART		
 	LED_Init();		  			//��ʼ����LED���ӵ�Ӳ���ӿ�
	KEY_Init();					//��ʼ������
	LCD_Init();			   		//��ʼ��LCD  
	FSMC_SRAM_Init(); //��ʼ���ⲿ SRAM
    T_Adc_Init();		  		//ADC��ʼ��	  
    tp_dev.init();              //��������ʼ��
	BEEP_Init();//��ʼ���������˿�
	BEEP=0;
	Lsens_Init(); //��ʼ������������
	TPAD_Init(6);				//����������ʼ�� 
 	POINT_COLOR=BLACK;			//��������Ϊ��ɫ 
    my_mem_init(SRAMIN);		//��ʼ���ڲ��ڴ��
	exfuns_init();				//Ϊfatfs��ر��������ڴ�  
 	f_mount(fs[0],"0:",1); 		//����SD��
	
    show_picture();//��ʾ��������
    delay_ms(9000);
    
    
    LCD_Clear(WHITE);
    
	touch_gui_login();//��ʾ��¼����
	passwordIndex();//��¼���潻��
	
	LCD_ShowString(30,110,200,16,16,"WELCOME TO USE THE SYSTEM"); 
	LCD_ShowString(100,130,400,16,16,"--MADE BY WangYiFan,LiYuanBo,LuTianYuan");
	LCD_ShowString(30,150,200,16,16,"PRESS KEY0 TO START");
	while(KEY_Scan(0)!=KEY0_PRES){}
	POINT_COLOR=BLACK;
    LCD_ShowString(30,230,200,16,16,"OV7725_OV7670 Init...");
            
    //���OV7725����ͷ
    if(OV7725_Init()==0)
    {                                                                                          
        sensor=OV7725;
        LCD_ShowString(30,230,200,16,16,"OV7725 Init OK       ");
        OV7725_Window_Set(OV7725_WINDOW_WIDTH,OV7725_WINDOW_HEIGHT,0);//QVGAģʽ���			
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
        if(i==100)LCD_ShowString(30,250,210,16,16,"PLEASE CHOOSE MODE"); //��˸��ʾ��ʾ��Ϣ
        if(i==200)
        {	
            LCD_Fill(30,250,210,250+16,WHITE);
            i=0; 
        }
        delay_ms(5);
        
        //�������
        key_index=get_keynum_enter();
        if(key_index){
            if(key_index==1){//����GRAY��
                check_mode=1;
                delay_ms(1000);
                break;
            }else if(key_index==3){//����AUTO��
                check_mode=3;
                delay_ms(1000);
                break;
            }else if(key_index==2){//����RGB��
                check_mode=2;
                delay_ms(1000);
                break;
            }
        }
        
    }

    
	TIM6_Int_Init(10000,7199);	//10Khz����Ƶ��,1�����ж�									  
	EXTI8_Init();				//ʹ�ܶ�ʱ������				
	LCD_Clear(BLACK);
    BACK_COLOR=BLACK;
        
 	while(1)
	{	
		key=KEY_Scan(0);//��֧������
		if(key)
		{
			tm=20;
			switch(key)
			{				    
				case KEY0_PRES:	//�ƹ�ģʽLight Mode
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
				case KEY1_PRES:	//���Ͷ�Saturation
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
				case KEY2_PRES:	//����Brightness				 
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
				case WKUP_PRES:	//�Աȶ�Contrast			    
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
		if(TPAD_Scan(0))//��⵽�������� 
		{
			show_alert_photo();
		}
        
        //�������
        key_index=get_keynum_main();
        if(key_index){
            if(key_index==1){//����LIGHT��
                light_flag=!light_flag;
                //LCD_ShowNum(20,240,light_flag,1,16);
            }else if(key_index==3){//����TEMPERATURE��
                temp_flag=!temp_flag;
                 //LCD_ShowNum(20,260,temp_flag,1,16);
            }else if(key_index==2){//����PHOTO��
                show_alert_photo();
            }
        }
               
        light_strong=Lsens_Get_Val();
		
		if(sensor==OV7725)
			OV7725_camera_refresh();//������ʾ
		else if(sensor==OV7670)
			OV7670_camera_refresh();//������ʾ
		
        touch_gui_main();//��ʾ��������
        
		POINT_COLOR=WHITE;
        
        //LCD_ShowNum(240,600,check_mode,1,16);
        
        //��ʾģʽ
        switch(check_mode){
            case 1:LCD_ShowString(200,50,200,16,16,"MODE:GRAY");break;
            case 2:LCD_ShowString(200,50,200,16,16,"MODE:RGB");break;
            case 3:LCD_ShowString(200,50,200,16,16,"MODE:AUTO");break;
        }
        
        //��ʾ����ǿ��
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
		
        //��ʾ�¶�
        if(temp_flag==1){
            LCD_ShowString(200,110,100,16,16,"TEMPERATURE:"); 
            temp=Get_Temprate();	//�õ��¶�ֵ 
            if(temp<0)
            {
                temp=-temp;
                LCD_ShowString(300,110,10,16,16,"-");	//��ʾ����
            }else LCD_ShowString(300,110,10,16,16,"  ");	//�޷���		
            LCD_ShowxNum(310,110,temp/100,2,16,0);		//��ʾ��������
            LCD_ShowString(325,110,10,16,16,".");
            LCD_ShowxNum(330,110,temp%100,2,16, 0X80);	//��ʾС������
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
		if(person_check_mode()==1)//�����⵽����
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
		if(i>=15)//DS0��˸.
		{
			i=0;
			LED1=!LED1;
 		}
	}
	
}













