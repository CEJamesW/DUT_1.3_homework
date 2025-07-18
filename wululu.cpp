#include <conio.h>
#include <graphics.h>
#include <mmsystem.h>
#include <windows.h>
#include <bits/stdc++.h>

#pragma comment(lib, "easyx.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "winmm.lib")

constexpr auto WIN_WIDTH=1600;
constexpr auto WIN_HEIGHT=900;
constexpr auto BUTTON_COUNT=3;
constexpr auto BUTTON_HEIGHT=80;
constexpr auto MAX_SUNFLOWERS=8;
constexpr auto CM=40;
constexpr auto SF1_SIZE=3*CM;
constexpr auto SF2_WIDTH=3*CM;
constexpr auto SF2_HEIGHT=4*CM;
constexpr auto SHUI_SIZE=3*CM;
constexpr auto MARGIN=10;
constexpr auto MAX_TARGETS=5;
constexpr auto TOTAL_TIME=180.0;

HWND hWnd;
using Clock=std::chrono::steady_clock;

enum ButtonState { NORMAL,HOVER,PRESSED,SELECTED };

enum class AmmoType { AP=0,M61,Incendiary,Cruise,Count };

struct Button
{
    RECT rect;
    ButtonState state;
};

struct Sunflower
{
    IMAGE* img;
    RECT rc;
    int type;
};

struct Target
{
    IMAGE* img{};
    RECT rc{};
    double hp{};
    Clock::time_point spawnTime;
    Clock::time_point pendingTime;
    double lifetime{};
    bool pending{};
};

std::vector<Button> buttons;
auto selected_index=0;
auto running=true;
auto mamaCount=100;
auto qjfCount=100;
auto apCount=100;
auto m61Count=100;
auto rsdCount=100;
auto xfdCount=100;
auto currentAmmo=AmmoType::AP;
auto defeatedCount=0;

static IMAGE xrk1,xrk2,shui,bgCao,qjf,AP,M61,rsd,xfd,wll,qiao;
std::vector<Sunflower> sunflowers;
std::vector<Target> targets;
auto shuiFollow=false;
int fixedShuiX=10,fixedShuiY=BUTTON_HEIGHT+10;
Clock::time_point lastAutoGen;
Clock::time_point gameStart,lastFpsTime;
auto frameCount=0;
auto fps=0.0;

inline void putimage_alpha(const int x,const int y,IMAGE* img)
{
    const int w=img->getwidth();
    const int h=img->getheight();
    BLENDFUNCTION bf;
    bf.BlendOp=AC_SRC_OVER;
    bf.BlendFlags=0;
    bf.SourceConstantAlpha=255;
    bf.AlphaFormat=AC_SRC_ALPHA;
    AlphaBlend(GetImageHDC(nullptr),x,y,w,h,GetImageHDC(img),0,0,w,h,bf);
}

void dpi_awareness()
{
    if(const auto lib=LoadLibraryA("user32.dll"))
    {
        using Func=BOOL (WINAPI*)(DPI_AWARENESS_CONTEXT);
        if(const auto fn=reinterpret_cast<Func>(GetProcAddress(lib,"SetProcessDpiAwarenessContext")))
        {
            fn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }
        FreeLibrary(lib);
    }
}

bool isOverlap(const RECT&a,const RECT&b)
{
    return !(a.right<b.left||a.left>b.right||a.bottom<b.top||a.top>b.bottom);
}

void addRandomSunflower()
{
    if(sunflowers.size()>=MAX_SUNFLOWERS)
    {
        return;
    }
    static std::mt19937 rng(static_cast<unsigned>(time(nullptr)));
    Sunflower s{};
    s.type=rng()%2+1;
    if(s.type==1)
    {
        s.img=&xrk1;
    }
    else
    {
        s.img=&xrk2;
    }
    int w=s.type==1?SF1_SIZE:SF2_WIDTH;
    int h=s.type==1?SF1_SIZE:SF2_HEIGHT;
    RECT rc;
    do
    {
        rc.left=MARGIN+rng()%(WIN_WIDTH-2*MARGIN-w);
        rc.top=BUTTON_HEIGHT+MARGIN+rng()%(WIN_HEIGHT-BUTTON_HEIGHT-2*MARGIN-h);
        rc.right=rc.left+w;
        rc.bottom=rc.top+h;
    }
    while(std::any_of(sunflowers.begin(),sunflowers.end(),[&](const Sunflower&o)
    {
        return isOverlap(rc,o.rc);
    }));
    s.rc=rc;
    sunflowers.push_back(s);
}

void auto_generate()
{
    if(const auto now=Clock::now();std::chrono::duration<double>(now-lastAutoGen).count()>=1.0)
    {
        lastAutoGen=now;
        if(rand()%100<5)
        {
            addRandomSunflower();
        }
    }
}

HWND create_window()
{
    initgraph(WIN_WIDTH,WIN_HEIGHT);
    const HWND h=GetHWnd();
    const int sw=GetSystemMetrics(SM_CXSCREEN);
    const int sh=GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(h,nullptr,(sw-WIN_WIDTH)/2,(sh-WIN_HEIGHT)/2,WIN_WIDTH,WIN_HEIGHT, SWP_NOZORDER);
    SetWindowText(h,"ÄñÊÞÊÞÕÒÂèÂè");
    LOGFONT f;
    gettextstyle(&f);
    f.lfQuality=ANTIALIASED_QUALITY;
    f.lfOutPrecision=OUT_TT_PRECIS;
    setbkmode(TRANSPARENT);
    settextstyle(&f);
    return h;
}

void init_sunflowers()
{
    std::mt19937 rng(static_cast<unsigned>(time(nullptr)));
    const int cnt=rng()%3+3;
    for(auto i=0;i<cnt;++i)
    {
        addRandomSunflower();
    }
    lastAutoGen=Clock::now();
}

void load_assets()
{
    loadimage(&xrk1,"ass/1_.png",SF1_SIZE,SF1_SIZE);
    loadimage(&xrk2,"ass/2_.png",SF2_WIDTH,SF2_HEIGHT);
    loadimage(&shui,"ass/sh.png",SHUI_SIZE,SHUI_SIZE);
    loadimage(&bgCao,"ass/cao.png");
    loadimage(&rsd,"ass/rsd.png",3*CM,1.5*CM);
    loadimage(&xfd,"ass/xfd.png",3*CM,1.5*CM);
    loadimage(&AP,"ass/AP.png",3*CM,1.5*CM);
    loadimage(&M61,"ass/M61.png",3*CM,1.5*CM);
    loadimage(&qjf,"ass/qjf.png",3*CM,1.5*CM);
    loadimage(&wll,"ass/wll.png",WIN_WIDTH,WIN_HEIGHT);
    loadimage(&qiao,"ass/qiao.png",WIN_WIDTH,WIN_HEIGHT-BUTTON_HEIGHT*1.3);
    mciSendString(_T("open ass\\dead.mp3 alias dead"),nullptr,0,nullptr);
    mciSendString(_T("open ass\\fumu.mp3 alias fumu"),nullptr,0,nullptr);
    mciSendString(_T("open ass\\naima.mp3 alias naima"),nullptr,0,nullptr);
    mciSendString(_T("open ass\\bgm.mp3 alias bgm"),nullptr,0,nullptr);
    mciSendString(_T("setaudio bgm volume to 400"),nullptr,0,nullptr);
    mciSendString(_T("play bgm repeat from 0"),nullptr,0,nullptr);
}

void setup_buttons()
{
    buttons.resize(BUTTON_COUNT);
    constexpr int bw=WIN_WIDTH/BUTTON_COUNT;
    for(auto i=0;i<BUTTON_COUNT;++i)
    {
        buttons[i].rect={i*bw,0,(i+1)*bw,BUTTON_HEIGHT};
        buttons[i].state=i==selected_index?SELECTED:NORMAL;
    }
}

void draw_buttons()
{
    for(int i=0;i<BUTTON_COUNT;++i)
    {
        const char* titles[]={"»¨Ô°","ÉÌµê","·¢ÉäÇÅ"};
        const auto&[rect, state]=buttons[i];
        COLORREF c;
        switch(state)
        {
        case NORMAL:
            c=RGB(220,220,220);
            break;
        case HOVER:
            c=RGB(180,180,180);
            break;
        case PRESSED:
            c=RGB(140,140,140);
            break;
        default:
            c=RGB(100,149,237);
            break;
        }
        setfillcolor(c);
        solidrectangle(rect.left,rect.top,rect.right,rect.bottom);
        setlinecolor(BLACK);
        rectangle(rect.left,rect.top,rect.right,rect.bottom);
        settextcolor(BLACK);
        settextstyle(30,0,"Î¢ÈíÑÅºÚ");
        const int tw=textwidth(titles[i]);
        const int th=textheight(titles[i]);
        outtextxy((rect.left+rect.right)/2-tw/2,(rect.top+rect.bottom)/2-th/2,titles[i]);
    }
}

void draw_scene()
{
    setbkcolor(WHITE);
    cleardevice();
    if(selected_index==0)
    {
        putimage_alpha(0,0,&bgCao);
        for(const auto&s:sunflowers)
        {
            putimage_alpha(s.rc.left,s.rc.top,s.img);
        }
        char buf[64];
        sprintf(buf,"ÂèÂèÊýÁ¿:%d",mamaCount);
        settextcolor(BLACK);
        settextstyle(30,0,"Î¢ÈíÑÅºÚ");
        outtextxy(WIN_WIDTH-200,BUTTON_HEIGHT+10,buf);
        setlinecolor(BLACK);
        roundrect(fixedShuiX-2,fixedShuiY-2,fixedShuiX+SHUI_SIZE+2,fixedShuiY+SHUI_SIZE+2,10,10);
        POINT p;
        GetCursorPos(&p);
        ScreenToClient(hWnd,&p);
        int x=shuiFollow?p.x-SHUI_SIZE/2:fixedShuiX;
        const int y=shuiFollow?p.y-SHUI_SIZE/2:fixedShuiY;
        putimage_alpha(x,y,&shui);
    }
    else if(selected_index==1)
    {
        putimage_alpha(0,0,&wll);
        const std::vector<std::string> items={
            "ÂèÂè:"+std::to_string(mamaCount)+"¸ö",
            "È«¼Ò¸£:"+std::to_string(qjfCount)+"¸ö",
            "APµ¯:"+std::to_string(apCount)+"¸ö",
            "M61µ¯:"+std::to_string(m61Count)+"¸ö",
            "È¼ÉÕµ¯:"+std::to_string(rsdCount)+"¸ö",
            "Ñ²·Éµ¯:"+std::to_string(xfdCount)+"¸ö"
        };
        constexpr int spacing=30;
        int totalW=-spacing;
        for(const auto&s:items)
        {
            totalW+=textwidth(s.c_str())+spacing;
        }
        int x=(WIN_WIDTH-totalW)/2;
        int y=BUTTON_HEIGHT+50;
        settextcolor(WHITE);
        settextstyle(24,0,"Î¢ÈíÑÅºÚ");
        for(const auto&s:items)
        {
            outtextxy(x,y,s.c_str());
            x+=textwidth(s.c_str())+spacing;
        }
        const std::vector<std::string> labels={
            "3¸öÂèÂè¶Ò»»1¸öÈ«¼Ò¸£",
            "5¸öÈ«¼Ò¸£¶Ò»»1¸öAPµ¯",
            "1¸öÈ«¼Ò¸£¶Ò»»5¸öM61µ¯",
            "2¸öÈ«¼Ò¸£¶Ò»»1¸öÈ¼ÉÕµ¯",
            "4¸öÈ«¼Ò¸£¶Ò»»1¸öÑ²·Éµ¯"
        };
        const std::vector<IMAGE*> icons={&qjf,&AP,&M61,&rsd,&xfd};
        constexpr int xBtn=WIN_WIDTH/2-300/2;
        int yBtn=BUTTON_HEIGHT+150;
        for(size_t i=0;i<labels.size();++i)
        {
            setfillcolor(RGB(200,230,250));
            solidrectangle(xBtn,yBtn,xBtn+300,yBtn+60);
            settextcolor(BLACK);
            settextstyle(24,0,"Î¢ÈíÑÅºÚ");
            int tw=textwidth(labels[i].c_str());
            int th=textheight(labels[i].c_str());
            outtextxy(WIN_WIDTH/2-tw/2,yBtn+(60-th)/2,labels[i].c_str());
            putimage_alpha(xBtn+300,yBtn,icons[i]);
            yBtn+=60+20;
        }
    }
    else
    {
        putimage_alpha(0,BUTTON_HEIGHT,&qiao);
        const std::vector<std::string> items={
            "È«¼Ò¸£:"+std::to_string(qjfCount)+"¸ö",
            "APµ¯:"+std::to_string(apCount)+"¸ö",
            "M61µ¯:"+std::to_string(m61Count)+"¸ö",
            "È¼ÉÕµ¯:"+std::to_string(rsdCount)+"¸ö",
            "Ñ²·Éµ¯:"+std::to_string(xfdCount)+"¸ö"
        };
        settextcolor(YELLOW);
        settextstyle(30,0,"Î¢ÈíÑÅºÚ");
        int y0=BUTTON_HEIGHT+10;
        for(size_t i=0;i<items.size();++i)
        {
            const int w=textwidth(items[i].c_str());
            outtextxy(WIN_WIDTH-10-w,y0+i*30,items[i].c_str());
        }
        char buf[32];
        sprintf(buf,"FPS:%d",static_cast<int>(fps));
        outtextxy(WIN_WIDTH-150,WIN_HEIGHT-110,buf);
        double rem=TOTAL_TIME-std::chrono::duration<double>(Clock::now()-gameStart).count();
        if(rem<0)
        {
            rem=0;
        }
        auto rs=static_cast<int>(ceil(rem));
        int m=rs/60;
        int s=rs%60;
        sprintf(buf,"¾àÀë³·Àë»¹ÓÐ%02d:%02d",m,s);
        setlinecolor(RED);
        roundrect(10,y0,230,y0+40,5,5);
        outtextxy(20,y0+5,buf);
        if(targets.size()<MAX_TARGETS&&rand()%1000<10)
        {
            Target t;
            const int idx=rand()%7+1;
            t.img=new IMAGE;
            loadimage(t.img,("ass/"+std::to_string(idx)+".png").c_str(),2*CM,3*CM);
            int w=t.img->getwidth();
            int h=t.img->getheight();
            t.rc.left=rand()%(WIN_WIDTH-w);
            t.rc.top=BUTTON_HEIGHT+rand()%(WIN_HEIGHT-BUTTON_HEIGHT-h);
            t.rc.right=t.rc.left+w;
            t.rc.bottom=t.rc.top+h;
            t.hp=100;
            t.spawnTime=Clock::now();
            t.lifetime=rand()%4+1;
            t.pending=false;
            targets.push_back(t);
        }
        for(auto it=targets.begin();it!=targets.end();)
        {
            double lived=std::chrono::duration<double>(Clock::now()-it->spawnTime).count();
            if(lived>it->lifetime||it->hp<=0)
            {
                if(it->hp<=0)
                {
                    defeatedCount++;
                    qjfCount--;
                }
                delete it->img;
                it=targets.erase(it);
            }
            else
            {
                putimage_alpha(it->rc.left,it->rc.top,it->img);
                if(it->pending&&Clock::now()>=it->pendingTime)
                {
                    if(currentAmmo==AmmoType::Incendiary)
                    {
                        it->hp-=15;
                        it->pendingTime+=std::chrono::milliseconds(200);
                        if(std::chrono::duration<double>(it->pendingTime-it->spawnTime).count()>1.0)
                        {
                            it->pending=false;
                        }
                    }
                    else
                    {
                        it->hp-=95;
                        it->pending=false;
                    }
                }
                ++it;
            }
        }
        const char* names[]={"APµ¯","M61µ¯","È¼ÉÕµ¯","Ñ²·Éµ¯"};
        const std::string sel=names[static_cast<int>(currentAmmo)];
        const int tw=textwidth(sel.c_str());
        outtextxy((WIN_WIDTH-tw)/2,WIN_HEIGHT-80,sel.c_str());
        sprintf(buf,"»÷°Ü¸ÉÔ±:%d",defeatedCount);
        outtextxy(WIN_WIDTH-150,WIN_HEIGHT-80,buf);
    }
}

void update_hover(const POINT p)
{
    for(auto&b:buttons)
    {
        if(PtInRect(&b.rect,p)&&b.state!=SELECTED)
        {
            b.state=HOVER;
        }
        else if(!PtInRect(&b.rect,p)&&b.state!=SELECTED)
        {
            b.state=NORMAL;
        }
    }
}

void handle_input()
{
    if(MouseHit())
    {
        const MOUSEMSG ms=GetMouseMsg();
        if(ms.uMsg==WM_MOUSEMOVE)
        {
            update_hover({ms.x,ms.y});
        }
        else if(ms.uMsg==WM_MOUSEWHEEL&&selected_index==2)
        {
            int dir=ms.wheel>0?1:-1;
            int idx=static_cast<int>(currentAmmo);
            idx=(idx+dir+static_cast<int>(AmmoType::Count))%static_cast<int>(AmmoType::Count);
            currentAmmo=static_cast<AmmoType>(idx);
        }
        else if(ms.uMsg==WM_LBUTTONDOWN)
        {
            for(auto&b:buttons)
            {
                if(PtInRect(&b.rect,{ms.x,ms.y}))
                {
                    b.state=PRESSED;
                }
            }
            if(selected_index==2)
            {
                int* stock=nullptr;
                switch(currentAmmo)
                {
                case AmmoType::AP:
                    stock=&apCount;
                    break;
                case AmmoType::M61:
                    stock=&m61Count;
                    break;
                case AmmoType::Incendiary:
                    stock=&rsdCount;
                    break;
                case AmmoType::Cruise:
                    stock=&xfdCount;
                    break;
                default:
                    break;
                }
                if(stock&&*stock>0)
                {
                    --*stock;
                    const POINT pt={ms.x,ms.y};
                    for(auto it=targets.begin();it!=targets.end();++it)
                    {
                        if(PtInRect(&it->rc,pt))
                        {
                            mciSendString(_T("play dead from 0"),nullptr,0,nullptr);
                            if(currentAmmo==AmmoType::AP)
                            {
                                it->hp-=100;
                            }
                            else if(currentAmmo==AmmoType::M61)
                            {
                                it->hp-=20;
                            }
                            else
                            {
                                it->pending=true;
                                it->pendingTime=Clock::now()+(currentAmmo==AmmoType::Incendiary
                                                                  ?std::chrono::seconds(0)
                                                                  :std::chrono::milliseconds(500));
                            }
                            if(it->hp<=0)
                            {
                                ++defeatedCount;
                                --qjfCount;
                                delete it->img;
                                it=targets.erase(it);
                                mciSendString(_T("play fumu from 0"),nullptr,0,nullptr);
                                if(qjfCount<0)
                                {
                                    running=false;
                                    draw_scene();
                                    draw_buttons();
                                    FlushBatchDraw();
                                    MessageBox(hWnd,("ÓÎÏ·½áÊø£¡È«¼Ò¸£²»×ã\n¹²»÷°Ü¸ÉÔ±:"+std::to_string(defeatedCount)).c_str(),
                                               "GAME OVER", MB_OK|MB_TOPMOST|MB_SETFOREGROUND);
                                }
                            }
                            break;
                        }
                    }
                }
            }
            else if(selected_index==0)
            {
                const RECT rc={fixedShuiX,fixedShuiY,fixedShuiX+SHUI_SIZE,fixedShuiY+SHUI_SIZE};
                if(!shuiFollow&&PtInRect(&rc,{ms.x,ms.y}))
                {
                    shuiFollow=true;
                }
            }
        }
        else if(ms.uMsg==WM_LBUTTONUP)
        {
            const int prev=selected_index;
            for(auto i=0;i<BUTTON_COUNT;++i)
            {
                if(PtInRect(&buttons[i].rect,{ms.x,ms.y}))
                {
                    selected_index=i;
                }
            }
            if(prev!=selected_index)
            {
                shuiFollow=false;
            }
            for(int i=0;i<BUTTON_COUNT;++i)
            {
                buttons[i].state=i==selected_index?SELECTED:NORMAL;
            }
            if(selected_index==0&&shuiFollow)
            {
                for(int k=sunflowers.size()-1;k>=0;--k)
                {
                    if(PtInRect(&sunflowers[k].rc,{ms.x,ms.y}))
                    {
                        mciSendString(_T("play naima from 0"),nullptr,0,nullptr);
                        mamaCount+=sunflowers[k].type==1?1:2;
                        sunflowers.erase(sunflowers.begin()+k);
                        if(rand()%2==0)
                        {
                            addRandomSunflower();
                        }
                        break;
                    }
                }
            }
            else if(selected_index==1)
            {
                struct Exchange
                {
                    int need,give, *from, *to;
                };
                const std::vector<Exchange> exchanges={
                    {3,1,&mamaCount,&qjfCount},
                    {5,1,&qjfCount,&apCount},
                    {1,5,&qjfCount,&m61Count},
                    {2,1,&qjfCount,&rsdCount},
                    {4,1,&qjfCount,&xfdCount}
                };
                int x=WIN_WIDTH/2-300/2;
                int y=BUTTON_HEIGHT+150;
                for(const auto&[need, give, from, to]:exchanges)
                {
                    RECT rc={x,y,x+300,y+60};
                    if(PtInRect(&rc,{ms.x,ms.y})&&*from>=need)
                    {
                        *from-=need;
                        *to+=give;
                    }
                    y+=60+20;
                }
            }
        }
        else if(ms.uMsg==WM_RBUTTONDOWN)
        {
            shuiFollow=false;
        }
    }
    if(_kbhit())
    {
        const char c=_getch();
        if(c=='1')
        {
            currentAmmo=AmmoType::AP;
        }
        else if(c=='2')
        {
            currentAmmo=AmmoType::M61;
        }
        else if(c=='3')
        {
            currentAmmo=AmmoType::Incendiary;
        }
        else if(c=='4')
        {
            currentAmmo=AmmoType::Cruise;
        }
    }
}

int main()
{
    dpi_awareness();
    hWnd=create_window();
    setup_buttons();
    load_assets();
    init_sunflowers();
    srand(static_cast<unsigned>(time(nullptr)));
    gameStart=lastFpsTime=Clock::now();
    BeginBatchDraw();
    while(running)
    {
        handle_input();
        auto_generate();
        draw_scene();
        draw_buttons();
        FlushBatchDraw();
        frameCount++;
        const auto now=Clock::now();
        if(std::chrono::duration<double>(now-lastFpsTime).count()>=1.0)
        {
            fps=frameCount/std::chrono::duration<double>(now-lastFpsTime).count();
            frameCount=0;
            lastFpsTime=now;
        }
        double rem=TOTAL_TIME-std::chrono::duration<double>(Clock::now()-gameStart).count();
        const auto rs=static_cast<int>(ceil(rem));
        char buf[64];
        sprintf(buf,"ÄñÊÞÊÞÕÒÂèÂè - ¾àÀë³·Àë»¹ÓÐ%02d:%02d",rs/60,rs%60);
        SetWindowText(hWnd,buf);
        if(rem<=0)
        {
            running=false;
            MessageBox(hWnd,("ÓÎÏ·½áÊø£¡\n¹²»÷°Ü¸ÉÔ±:"+std::to_string(defeatedCount)).c_str(),"GAME OVER",
                       MB_OK|MB_TOPMOST|MB_SETFOREGROUND);
        }
    }
    EndBatchDraw();
    closegraph();
    return 0;
}
