#include <reg51.h>
#include <intrins.h>
#include <string.h>

#define delayNOP(); {_nop_();_nop_();_nop_();_nop_();};
#define uchar unsigned char
#define uint  unsigned int
#define LCD_data  P0 

#define AM(X)	X
#define PM(X)	(X+12) 
#define DS1302_SECOND	0x80
#define DS1302_MINUTE	0x82
#define DS1302_HOUR		0x84 
#define DS1302_WEEK		0x8A
#define DS1302_DAY		0x86
#define DS1302_MONTH	0x88
#define DS1302_YEAR		0x8C
#define DS1302_RAM(X)	(0xC0+(X)*2)   	//用于计算 DS1302_RAM 地址的宏 

#define	IR_NUM_1	69
#define	IR_NUM_2	70
#define	IR_NUM_3	71
#define	IR_NUM_4	68
#define	IR_NUM_5	64
#define	IR_NUM_6	67
#define	IR_NUM_7	7
#define	IR_NUM_8	21
#define	IR_NUM_9	8
#define	IR_CMD_STAR	22
#define	IR_NUM_0	25
#define	IR_CMD_SHARP	13
#define	IR_CMD_UP	24
#define	IR_CMD_DOWN	82
#define	IR_CMD_LEFT	8
#define	IR_CMD_RIGHT	90
#define	IR_CMD_OK	28

#define IR_CMDRECV IRCOM[2]


typedef struct __SYSTEMTIME__
{
	unsigned char Second;
	unsigned char Minute;
	unsigned char Hour;
	unsigned char Week;
	unsigned char Day;
	unsigned char Month;
	unsigned char Year;
	unsigned char DateString[13];
	unsigned char TimeString[9];
}SystemTime;

typedef struct __CLIMATE__
{
	int Temp;
  int Humidity;
	unsigned char TempString[4];
	unsigned char HumidityString[5];
}SystemClimate;

sbit LCD_RS  =  P3^5;            //寄存器选择输入 
sbit LCD_RW  =  P3^6;            //液晶读/写控制
sbit LCD_EN  =  P3^4;            //液晶使能控制
sbit LCD_PSB =  P3^1;            //串/并方式控制

sbit  DS1302_CLK = P2^5;              //实时时钟时钟线引脚 
sbit  DS1302_IO  = P2^4;              //实时时钟数据线引脚 
sbit  DS1302_RST = P2^3;              //实时时钟复位线引脚  

sbit BUZZER=P2^0;

sbit Data=P1^0;
sbit IRIN = P3^3;         //红外接收器数据线

uchar GRAM[17];

uchar IRCOM[4];

sbit  ACC0 = ACC^0;
sbit  ACC7 = ACC^7;

unsigned int SysTick=0;

void TIM0_Handler() interrupt 1
{
	SysTick++;
}

void ir_wait(unsigned char x)    //x*0.14MS
{
 unsigned char i;
  while(x--)
 {
  for (i = 0; i<13; i++) {}
 }
}

void ir_wait1(int ms)
{
 unsigned char y;
  while(ms--)
 {
  for(y = 0; y<250; y++)
  {
   _nop_();
   _nop_();
   _nop_();
   _nop_();
  }
 }
}
void DHT11_delay_us(uchar n)
{
    while(--n);
}

void DHT11_delay_ms(uint z)
{
   uint i,j;
   for(i=z;i>0;i--)
      for(j=110;j>0;j--);
}

void delay0(uchar x)    //x*0.14MS
{
  uchar i;
  while(x--)
 {
  for (i = 0; i<13; i++) {}
 }
}

void delay(int ms)
{
    while(ms--)
	{
      uchar i;
	  for(i=0;i<110;i++)  
	   {
				_nop_();			   
				_nop_();
	   }
	}
}		

void Beep()
{
	int length;
	for(length = 0;length < 100;length++){
		BUZZER = 0;
		DHT11_delay_us(150);
		BUZZER = 1;
		DHT11_delay_us(150);
	}
}

void BeepAlarm()
{
	int length;
	for(length = 0;length < 200;length++){
		BUZZER = 0;
		DHT11_delay_us(100);
		BUZZER = 1;
		DHT11_delay_us(100);
	}
}

bit LCD_GetStatus()
{                          
    bit result;
    LCD_RS = 0;
    LCD_RW = 1;
    LCD_EN = 1;
    delayNOP();
    result = (bit)(P0&0x80);
    LCD_EN = 0;
    return(result); 
}

void LCD_WCmd(uchar cmd)
{                          
   while(LCD_GetStatus());
    LCD_RS = 0;
    LCD_RW = 0;
    LCD_EN = 0;
    _nop_();
    _nop_(); 
    P0 = cmd;
    delayNOP();
    LCD_EN = 1;
    delayNOP();
    LCD_EN = 0;  
}

void LCD_SetPos(uchar X,uchar Y)
{                          
   uchar  pos;
   if (X==0)
     {X=0x80;}
   else if (X==1)
     {X=0x90;}
   else if (X==2)
     {X=0x88;}
   else if (X==3)
     {X=0x98;}
   pos = X+Y ;  
   LCD_WCmd(pos);     //显示地址
}


void LCD_WDat(uchar dat)
{                          
   while(LCD_GetStatus());
    LCD_RS = 1;
    LCD_RW = 0;
    LCD_EN = 0;
    P0 = dat;
    delayNOP();
    LCD_EN = 1;
    delayNOP();
    LCD_EN = 0; 
}

void LCD_Init()
{ 

    LCD_PSB = 1;         //并口方式  
    LCD_WCmd(0x34);      //扩充指令操作
    delay(5);
    LCD_WCmd(0x30);      //基本指令操作
    delay(5);
    LCD_WCmd(0x0C);      //显示开，关光标
    delay(5);
    LCD_WCmd(0x01);      //清除LCD的显示内容
    delay(5);
}

void DHT11_start()
{
   Data=1;
   DHT11_delay_us(2);
   Data=0;
   DHT11_delay_ms(20);
   Data=1;
   DHT11_delay_us(30);
}

uchar DHT11_rec_byte() 
{
   uchar i,dat=0;
  for(i=0;i<8;i++) 
   {          
      while(!Data); 
      DHT11_delay_us(8); 
      dat<<=1;   
      if(Data==1)  
         dat+=1;
      while(Data);   
    }  
    return dat;
}

void DHT11_receive(SystemClimate *Climate) 
{
    uchar R_H,R_L,T_H,T_L,RH,RL,TH,TL,revise; 
    DHT11_start();
    if(Data==0)
    {
        while(Data==0);
        DHT11_delay_us(40); 
        R_H=DHT11_rec_byte(); 
        R_L=DHT11_rec_byte();
        T_H=DHT11_rec_byte();  
        T_L=DHT11_rec_byte(); 
        revise=DHT11_rec_byte(); 

        DHT11_delay_us(25); 

        if((R_H+R_L+T_H+T_L)==revise) 
        {
            RH=R_H;
            RL=R_L;
            TH=T_H;
            TL=T_L;
        }
				Climate->Humidity=RH;
				Climate->Temp=TH;
				if(Climate->Temp >= 0)
				{
					Climate->TempString[0]='0'+(TH/10);
					Climate->TempString[1]='0'+(TH%10);
					Climate->TempString[2]='C';
					Climate->TempString[3]='\0';
				}
				else
				{
					Climate->TempString[1]='0'+(TH/10);
					Climate->TempString[2]='0'+(TH%10);
					Climate->TempString[0]='-';
					Climate->TempString[3]='\0';
				}
				if(Climate->Humidity==100)Climate->Humidity=99;
					Climate->HumidityString[0]='0'+(RH/10);
					Climate->HumidityString[1]='0'+(RH%10);
					Climate->HumidityString[2]='R';
					Climate->HumidityString[3]='H';
					Climate->HumidityString[4]='\0';
    }
}

void DS1302InputByte(unsigned char d) 	//实时时钟写入一字节(内部函数)
{ 
    unsigned char i;
    ACC = d;
    for(i=8; i>0; i--)
    {
        DS1302_IO = ACC0;           	//相当于汇编中的 RRC
        DS1302_CLK = 1;
        DS1302_CLK = 0;
        ACC = ACC >> 1; 
    } 
}

unsigned char DS1302OutputByte(void) 	//实时时钟读取一字节(内部函数)
{ 
    unsigned char i;
    for(i=8; i>0; i--)
    {
        ACC = ACC >>1;         			//相当于汇编中的 RRC 
        ACC7 = DS1302_IO;
        DS1302_CLK = 1;
        DS1302_CLK = 0;
    } 
    return(ACC); 
}

void Write1302(unsigned char ucAddr, unsigned char ucDa)	//ucAddr: DS1302地址, ucData: 要写的数据
{
    DS1302_RST = 0;
    DS1302_CLK = 0;
    DS1302_RST = 1;
    DS1302InputByte(ucAddr);       	// 地址，命令 
    DS1302InputByte(ucDa);       	// 写1Byte数据
//    DS1302_CLK = 1;
    DS1302_RST = 0;
} 

unsigned char Read1302(unsigned char ucAddr)	//读取DS1302某地址的数据
{
    unsigned char ucData;
    DS1302_RST = 0;
    DS1302_CLK = 0;
    DS1302_RST = 1;
    DS1302InputByte(ucAddr|0x01);        // 地址，命令 
    ucData = DS1302OutputByte();         // 读1Byte数据
//    DS1302_CLK = 1;
    DS1302_RST = 0;
    return(ucData);
}

void DS1302_SetProtect(bit flag)        //是否写保护
{
	if(flag)
		Write1302(0x8E,0x10);
	else
		Write1302(0x8E,0x00);
}

void DS1302_SetTime(unsigned char Address, unsigned char Value)        // 设置时间函数
{
	DS1302_SetProtect(0);
	Write1302(Address, ((Value/10)<<4 | (Value%10))); 
}

void DS1302_GetTime(SystemTime *Time)
{
	unsigned char ReadValue;
	ReadValue = Read1302(DS1302_SECOND);
	Time->Second = ((ReadValue&0x70)>>4)*10 + (ReadValue&0x0F);	//八进制转换成十进制
	ReadValue = Read1302(DS1302_MINUTE);
	Time->Minute = ((ReadValue&0x70)>>4)*10 + (ReadValue&0x0F);
	ReadValue = Read1302(DS1302_HOUR);
	Time->Hour = ((ReadValue&0x70)>>4)*10 + (ReadValue&0x0F);
	ReadValue = Read1302(DS1302_DAY);
	Time->Day = ((ReadValue&0x70)>>4)*10 + (ReadValue&0x0F);	
	ReadValue = Read1302(DS1302_WEEK);
	Time->Week = ((ReadValue&0x70)>>4)*10 + (ReadValue&0x0F);
	ReadValue = Read1302(DS1302_MONTH);
	Time->Month = ((ReadValue&0x70)>>4)*10 + (ReadValue&0x0F);
	ReadValue = Read1302(DS1302_YEAR);
	Time->Year = ((ReadValue&0x70)>>4)*10 + (ReadValue&0x0F);	
}

void DateToStr(SystemTime *Time)	 //
{
	Time->DateString[0] = Time->Year/10+0x30 ;	 //分离出个位和十位
	Time->DateString[1] = Time->Year%10+0x30 ;
	Time->DateString[2] = 0xC4;
	Time->DateString[3] = 0xEA;
	Time->DateString[4] = Time->Month/10+0x30;
	Time->DateString[5] = Time->Month%10+0x30;
	Time->DateString[6] = 0xD4;
	Time->DateString[7] = 0xC2;
	Time->DateString[8] = Time->Day/10+0x30;
	Time->DateString[9] = Time->Day%10+0x30;
	Time->DateString[10] = 0xC8;
	Time->DateString[11] = 0xD5;
	Time->DateString[12] = '\0';
}

void TimeToStr(SystemTime *Time)
{
	Time->TimeString[0] = Time->Hour/10+0x30;
	Time->TimeString[1] = Time->Hour%10+0x30;
	Time->TimeString[2] = ':';
	Time->TimeString[3] = Time->Minute/10+0x30;
	Time->TimeString[4] = Time->Minute%10+0x30;
	Time->TimeString[5] = ':';
	Time->TimeString[6] = Time->Second/10+0x30;
	Time->TimeString[7] = Time->Second%10+0x30;
	Time->TimeString[8] = '\0';
}

void Initial_DS1302(void)
{
	unsigned char Second=Read1302(DS1302_SECOND);
	if(Second&0x80)		  
		DS1302_SetTime(DS1302_SECOND,0);
}



void LCD_WriteAt(unsigned char x,unsigned char y)
{
		uchar i;    
    LCD_SetPos(x,y);             //设置显示位置为第一行的第1个字符
     i = 0;
    while(GRAM[i] != '\0')
     {                         //显示字符
       LCD_WDat(GRAM[i]);
       i++;
     }
}

void LCD_Clear()
{
	strcpy(GRAM,"                ");
	LCD_WriteAt(0,0);
	LCD_WriteAt(1,0);
	LCD_WriteAt(2,0);
	LCD_WriteAt(3,0);
}

void IR_IN() interrupt 2 using 0
{
  unsigned char j,k,N=0;
     EX1 = 0;   
	 ir_wait(15);
	 if (IRIN==1) 
     { EX1 =1;
	   return;
	  } 
                           //确认IR信号出现
  while (!IRIN)            //等IR变为高电平，跳过9ms的前导低电平信号。
    {ir_wait(1);}

 for (j=0;j<4;j++)         //收集四组数据
 { 
  for (k=0;k<8;k++)        //每组数据有8位
  {
   while (IRIN)            //等 IR 变为低电平，跳过4.5ms的前导高电平信号。
     {ir_wait(1);}
    while (!IRIN)          //等 IR 变为高电平
     {ir_wait(1);}
     while (IRIN)           //计算IR高电平时长
      {
    ir_wait(1);
    N++;           
    if (N>=30)
	 { EX1=1;
	 return;}                  //0.14ms计数过长自动离开。
      }                        //高电平计数完毕                
     IRCOM[j]=IRCOM[j] >> 1;                  //数据最高位补“0”
     if (N>=8) {IRCOM[j] = IRCOM[j] | 0x80;}  //数据最高位补“1”
     N=0;
  }//end for k
 }//end for j
   
   if (IRCOM[2]!=~IRCOM[3])
   { EX1=1;
     goto LOOP; }

//	GRAM[0]=IRCOM[2]/100+0x30;
//	GRAM[1]=IRCOM[2]%100/10+0x30;
//	GRAM[2]=IRCOM[2]%10+0x30;
//	GRAM[3]="\0";
//	LCD_WriteAt(3,6);
		 
	Beep();
  EX1 = 1;
	LOOP:; 
} 

void GUI_Index()
{
	unsigned char ptr;
	SystemTime CurrentTime;
	SystemClimate CurrentClimate;
	if(SysTick >= 5)
	{
		DS1302_GetTime(&CurrentTime);
		DateToStr(&CurrentTime);
		TimeToStr(&CurrentTime);
		DHT11_receive(&CurrentClimate);
	for(ptr=0;ptr<=8;ptr++)
		GRAM[ptr] = CurrentTime.TimeString[ptr];
	LCD_WriteAt(0,0);
	strcpy(GRAM,"闹钟??");
	LCD_WriteAt(0,5);
	strcpy(GRAM,"公元");
	LCD_WriteAt(1,0);
	for(ptr=0;ptr<=12;ptr++)
		GRAM[ptr] = CurrentTime.DateString[ptr];
	LCD_WriteAt(1,2);
	strcpy(GRAM,"温度    湿度");
	LCD_WriteAt(2,0);		
	for(ptr=0;ptr<=3;ptr++)
		GRAM[ptr] = CurrentClimate.TempString[ptr];
	LCD_WriteAt(2,2);
	for(ptr=0;ptr<=4;ptr++)
		GRAM[ptr] = CurrentClimate.HumidityString[ptr];
	LCD_WriteAt(2,6);
	SysTick = 0;
	}
}

void GUI_MainMenu()
{
	LCD_Clear();
	strcpy(GRAM,"1.本地时间设置");
	LCD_WriteAt(0,0);
	strcpy(GRAM,"2.闹钟设置");
	LCD_WriteAt(1,0);
	strcpy(GRAM,"3.温度阈值设置");
	LCD_WriteAt(2,0);
	strcpy(GRAM,"4.其他选项");
	LCD_WriteAt(3,0);
	while(1)
	{
		
	}
}

void main()
{	
	IE = 0x84;                 //允许总中断中断,使能 INT1 外部中断
	TCON = 0x10;               //触发方式为脉冲负边沿触发  
	IRIN=1;                    //I/O口初始化

	TMOD=0x01;
	TH0=(65536-46080)/256;// 由于晶振为11.0592,故所记次数应为46080，计时器每隔50000微秒发起一次中断。
	TL0=(65536-46080)%256;//46080的来历，为50000*11.0592/12
	ET0=1;
	EA=1;
	
	LCD_Init();                //初始化LCD  
	Initial_DS1302();
	strcpy(GRAM,"按[OK]键打开菜单");
	LCD_WriteAt(1,0);
	delay(500);
	LCD_Clear();
	Beep();
	BeepAlarm();
	while(1)
	{
		GUI_Index();
		if(IR_CMDRECV == IR_CMD_OK)
		{
			GUI_MainMenu();
		}
	}
}
