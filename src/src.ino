#include <Arduino.h>
#include <Ticker.h>

#include <FS.h>
#include <SPI.h>

#include <U8g2lib.h>
//#include <ArduinoOTA.h>        

//OLED显示屏配置(1306-128*64-F)
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ D7, /* data=*/ D6, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display

//定时器
Ticker tickerone;// 建立定时器
unsigned int count=1;
unsigned int face_state = 0;
unsigned int reflow_time_tag = 0;


int flash=0;   //  刷新屏幕

#define FAN 14 //D5--风扇-H
#define HOT 0   //D3--加热-H
#define KEY1 16 //D0--按键1-L   (OMG! D0没有内部上拉，记得加电阻)
#define KEY2 5 //D1--按键2-L
#define KEY3 4 //D2--按键3-L

//温度计参数
#define Rt25 10
#define B	3950

//系统数据结构体
struct system_msg
{
    float tem;  //温度信息
    float old_tem;
    float tag;  //目标温度
    int key;    //按键界面状态
    int work;   //工作状态
    int sec;
}sys_msg;

///计算Len
double myln(double a)
{
   int N = 15;//我们取了前15+1项来估算
   int k,nk;
   double x,xx,y;
   x = (a-1)/(a+1);
   xx = x*x;
   nk = 2*N+1;
   y = 1.0/nk;
   for(k=N;k>0;k--)
   {
     nk = nk - 2;
     y = 1.0/nk+xx*y; 
   }
   return 2.0*x*y;
}

//更新温度信息
void updata_tem()
{
    float Rntc,bat,a;
    for(int i=0;i<5;i++)
        bat+=analogRead(A0);
    a=map(bat/5.0,0,1024,0,3300)/1000.0;
    Rntc=10*a/(3.3-a);

    float N1,N2;
	N1 = (myln(Rt25)-myln(Rntc))/B;
	N2 = 1/298.15 - N1;

    sys_msg.old_tem=sys_msg.tem;
	sys_msg.tem=1/N2-273.15;
}

//开机动画
void open_face()
{
    Serial.println("open_face");
    u8g2.clear();
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawStr(20,20,"Reflow");
    u8g2.drawStr(35,45,"MiNi");
    u8g2.sendBuffer();
    delay(150);

    u8g2.setFont(u8g2_font_open_iconic_weather_1x_t);   //字体字集
    for (int i = 0,k=0; i < 6; i++,k+=20)
    {
        u8g2.drawGlyph(k,60,0x41);u8g2.sendBuffer();
        delay(120);
    }
    delay(200);
}

//系统信息界面
void sys_face()
{
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(45,12,"MSG");

    u8g2.setFont(u8g2_font_courB08_tr);
    u8g2.setCursor(5,25);
    u8g2.print("tag:"); sys_msg.tag==0?u8g2.print("- -"):u8g2.print(sys_msg.tag);
    u8g2.setCursor(5,35);
    u8g2.print("tem:"); u8g2.print(sys_msg.tem);
    u8g2.setCursor(5,45);
    u8g2.print("sta:"); sys_msg.work==0?u8g2.print("close"):(sys_msg.work==1?u8g2.print("cool"):(sys_msg.work==2?u8g2.print("hot"):u8g2.print("reflow")));
    u8g2.setCursor(5,55);
    u8g2.print("sec:"); sys_msg.sec==0?u8g2.print("- -"):u8g2.print(sys_msg.sec);

    u8g2.sendBuffer();
}

//用户给定界面
void sys_usertag()
{
    int a=sys_msg.key<=10?0:sys_msg.key%10;

    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(40,12,"USER");

    u8g2.setFont(u8g2_font_courB08_tr);
    u8g2.setCursor(5,30);
    u8g2.print("tag:"); sys_msg.tag==0?u8g2.print("- -"):u8g2.print(sys_msg.tag);
    u8g2.setCursor(5,45);
    u8g2.print("tem:"); u8g2.print(sys_msg.tem);
    u8g2.setCursor(5,62);
    u8g2.print("  down   back   up");
    u8g2.sendBuffer();
}


//菜单界面
void sys_menu()
{
    int a=sys_msg.key<=10?0:sys_msg.key%10;

    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(40,12,"MENU");

    u8g2.setFont(u8g2_font_courB08_tr);
    u8g2.setCursor(8,22);   u8g2.print("cool");
    u8g2.setCursor(8,32);   u8g2.print("hot");
    u8g2.setCursor(8,42);   u8g2.print("ref");
    u8g2.setCursor(8,52);   u8g2.print("close");
    u8g2.setCursor(8,62);   u8g2.print("user");
    u8g2.setCursor(100,22+10*a);   u8g2.print("<");
    u8g2.sendBuffer();
}

//系统初始化
void system_config()
{
    pinMode(LED_BUILTIN, OUTPUT);   digitalWrite(LED_BUILTIN,HIGH);
    pinMode(FAN, OUTPUT);   digitalWrite(FAN,LOW); 
    pinMode(HOT, OUTPUT);   digitalWrite(HOT,LOW);
    pinMode(KEY1, INPUT_PULLUP);
    pinMode(KEY2, INPUT_PULLUP); 
    pinMode(KEY3, INPUT_PULLUP);

    updata_tem();
    sys_msg.tag=0;
    sys_msg.key=0;
    sys_msg.work=0;
    sys_msg.sec=0;
}

void setup() 
{
    Serial.begin(9600); Serial.println("system_config");
    u8g2.begin();
    system_config();

    open_face();

    tickerone.attach_ms(100,heat_tick);//100ms
    u8g2.clear();
}

//按键扫描
void fa_key_scan()
{
    int page=sys_msg.key<10?sys_msg.key:sys_msg.key/10;
    if(page==0)//信息界面0
    {
        if(LOW==digitalRead(KEY1) || LOW==digitalRead(KEY2) || LOW==digitalRead(KEY3))//进入菜单1
        {
            sys_msg.key=10;
            face_state=count; 
        }
    }
    if(page==1)//菜单界面1-0
    {
        if(LOW==digitalRead(KEY1))//向上
        {
            sys_msg.key=(sys_msg.key>10&&sys_msg.key<20)?sys_msg.key-1:10;
            face_state=count;  Serial.println("1-up");
        }
        if(LOW==digitalRead(KEY3))//向下
        {
            sys_msg.key=(sys_msg.key<14&&sys_msg.key>9)?sys_msg.key+1:14;
            face_state=count;  Serial.println("1-down");
        }
        if(LOW==digitalRead(KEY2))//确认
        {
            if(sys_msg.key%10==0)
            {
                sys_msg.work=1; //降温
                sys_msg.tag=0;
            }   
            if(sys_msg.key%10==1)
            {
                sys_msg.work=2; //加热
                sys_msg.tag=250;
            }
            if(sys_msg.key%10==2)
            {
                sys_msg.work=3; //回流焊
                sys_msg.tag=0;
                reflow_time_tag=count/10;
            }
            if(sys_msg.key%10==3)
            {
                sys_msg.work=0; //关闭
                digitalWrite(LED_BUILTIN,HIGH);
                sys_msg.sec=0;
                sys_msg.tag=0;
                sys_msg.key = 0;
            }
            if(sys_msg.key%10==4)
            {
                sys_msg.key=20;
                sys_msg.tag=0;
            }
            face_state=count; Serial.println("1-OK");
        }
    }

    if (page==2)//给定温度界面
    {
        sys_msg.work=4;
        if(LOW==digitalRead(KEY1))//减温
        {
            sys_msg.tag-=10;
        }   
        if(LOW==digitalRead(KEY2))//返回
        {
            sys_msg.key=0;
            sys_msg.work=0;
            sys_msg.tag=0;
        }
        if(LOW==digitalRead(KEY3))//增温
        {
            sys_msg.tag+=10;
        }
        if(sys_msg.tag>250)    sys_msg.tag=250;
        if(sys_msg.tag<30)    sys_msg.tag=30;
    }

    if((count-face_state)>60 && page!=2)
    {
        sys_msg.key=0;
        face_state=count; 
    }
}


//回流焊温控
int reftime=0;
int restage=-1;
void reflow_prg()
{
    sys_msg.sec=count/10-reflow_time_tag;

    //升温阶段
    if(restage==-1)
    {
        sys_msg.tag=150;
        if(sys_msg.tem<150)  {digitalWrite(HOT,HIGH);digitalWrite(FAN,LOW);}
        if(sys_msg.tem>150) restage=0;
    }
    //吸热阶段
    if(restage==0 && sys_msg.sec-reftime<100)
    {
        sys_msg.tag=180;
        if(sys_msg.tem<180-10)  {digitalWrite(HOT,HIGH);digitalWrite(FAN,LOW);}
        if(sys_msg.tem>180+10 && sys_msg.sec-reftime>10)    {digitalWrite(FAN,HIGH);digitalWrite(HOT,LOW);}
        if(sys_msg.tem>180+10 && sys_msg.sec-reftime<10)    {digitalWrite(FAN,LOW);digitalWrite(HOT,LOW);}
    }
    if(restage==0 && sys_msg.sec-reftime>100)   {restage=1;reftime=0;}

    //焊接阶段
    if(restage==1)
    {
        sys_msg.tag=250;
        if(sys_msg.tem<250)  {digitalWrite(HOT,HIGH);digitalWrite(FAN,LOW);}
        if(reftime==0 && sys_msg.tem>230)   reftime=sys_msg.sec;
        if(reftime!=0 && sys_msg.sec-reftime>60)    restage=2;
    }

    //降温阶段
    if(restage==2 && sys_msg.tem>50)    {digitalWrite(FAN,HIGH);digitalWrite(HOT,LOW);}
    if(restage==2 && sys_msg.tem<50)    {digitalWrite(FAN,LOW);digitalWrite(HOT,LOW);sys_msg.key=13;}
}

//user-tag-work
void user_work()
{
    if(sys_msg.tag > sys_msg.tem)
    {
        //user_tag_time;
        if((sys_msg.tag - sys_msg.tem >=2.5) && (sys_msg.tem - sys_msg.old_tem <= -0.05))
        {
            digitalWrite(HOT,HIGH);digitalWrite(FAN,LOW);//hot
        }
        else
        {
            digitalWrite(FAN,LOW);digitalWrite(HOT,LOW);//close
        }
    }
    else if(sys_msg.tag < sys_msg.tem)
    {
        if(sys_msg.tem - sys_msg.tag >=1)
        {
            digitalWrite(HOT,LOW);digitalWrite(FAN,HIGH);//cool
        }
        else
        {
            digitalWrite(FAN,LOW);digitalWrite(HOT,LOW);//close
        }
    }
}


#define FACE_PAGE   3
void (*func_table[FACE_PAGE])() = {sys_face,sys_menu,sys_usertag};//指向函数的指针数组

void loop() 
{
    fa_key_scan();
    (*func_table[sys_msg.key<10?sys_msg.key:sys_msg.key/10])(); //主取十位
    if(flash!=sys_msg.key)  {flash=sys_msg.key; u8g2.clear();}
    if(count%100==0) u8g2.clear();

    if(sys_msg.work==0) {digitalWrite(FAN,LOW);digitalWrite(HOT,LOW);}  //close
    if(sys_msg.work==1) //cool
    {
        digitalWrite(FAN,HIGH);digitalWrite(HOT,LOW);
        if(sys_msg.tem<40) {sys_msg.work=0; sys_msg.key=0;}
    }
    if(sys_msg.work==2) {digitalWrite(HOT,HIGH);digitalWrite(FAN,LOW);} //hot
    if(sys_msg.work==3) reflow_prg();   //reflow
    if(sys_msg.work==4) user_work();  //user-tag

    //if(count%2==0)updata_tem();//温度信息更新
    updata_tem();//温度信息更新
}

//定时器(100ms)
void heat_tick()
{
    count++;
    Serial.println(sys_msg.tem);
    if(sys_msg.work!=0 && count%10==0) digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
}
