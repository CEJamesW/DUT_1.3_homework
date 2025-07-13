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

inline void putimage_alpha(const int x,const int y,IMAGE* img)
{
    const int w=img->getwidth();
    const int h=img->getheight();
    AlphaBlend(GetImageHDC(nullptr),x,y,w,h,
               GetImageHDC(img),0,0,w,h,{AC_SRC_OVER,0,255,AC_SRC_ALPHA});
}

static IMAGE mask,door,bg,enemy;
static auto running=true,paused=false,just_unpaused=false;
static std::vector<std::string> mvp_names;
static size_t current_mvp_index=0;

struct HitEvent
{
    std::string text;
    std::chrono::high_resolution_clock::time_point time;
};

static std::vector<HitEvent> hit_events;
static HWND g_hwnd;

void dpi_awareness()
{
    if(const auto lib=LoadLibraryA("user32.dll"))
    {
        using Func=BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
        if(const auto fn=reinterpret_cast<Func>(GetProcAddress(lib,"SetProcessDpiAwarenessContext")))
            fn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        FreeLibrary(lib);
    }
}

HWND create_window()
{
    initgraph(WIN_WIDTH,WIN_HEIGHT);
    const HWND h=GetHWnd();
    const int sw=GetSystemMetrics(SM_CXSCREEN);
    const int sh=GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(h,nullptr,(sw-WIN_WIDTH)/2,(sh-WIN_HEIGHT)/2,
                 WIN_WIDTH,WIN_HEIGHT, SWP_NOZORDER);
    SetWindowText(h,"4399狙击小日本");
    return h;
}

void scan_mvp_assets()
{
    mvp_names.clear();
    WIN32_FIND_DATAA fd;
    if(const auto h=FindFirstFileA("assets\\mvp_*.mp3",&fd);h!=INVALID_HANDLE_VALUE)
    {
        do
        {
            std::string prefix="mvp_",suffix=".mp3";
            if(std::string fn=fd.cFileName;fn.rfind(prefix,0)==0&&fn.size()>prefix.size()+suffix.size())
                mvp_names.push_back(fn.substr(prefix.size(),fn.size()-prefix.size()-suffix.size()));
        }
        while(FindNextFileA(h,&fd));
        FindClose(h);
    }
    mvp_names.insert(mvp_names.begin(),"");
    current_mvp_index=0;
}

void load_assets()
{
    loadimage(&mask,"assets/mask.png",WIN_WIDTH,WIN_HEIGHT);
    loadimage(&door,"assets/door.png",WIN_WIDTH*1.5,WIN_HEIGHT*1.5);
    loadimage(&bg,"assets/bg.png",WIN_WIDTH*1.5,WIN_HEIGHT*1.5);
    loadimage(&enemy,"assets/enemy.png");
    mciSendString(_T("open assets\\shoot.mp3 alias shoot"),nullptr,0,nullptr);
    mciSendString(_T("open assets\\bgm.mp3 alias bgm"),nullptr,0,nullptr);
    mciSendString(_T("open assets\\headshot.mp3 alias headshot"),nullptr,0,nullptr);
    scan_mvp_assets();
    mciSendString(_T("setaudio bgm volume to 400"),nullptr,0,nullptr);
    mciSendString(_T("play bgm repeat from 0"),nullptr,0,nullptr);
}

void switch_mvp()
{
    mciSendString(_T("stop mvp"),nullptr,0,nullptr);
    mciSendString(_T("close mvp"),nullptr,0,nullptr);
    current_mvp_index=(current_mvp_index+1)%mvp_names.size();
    if(!mvp_names[current_mvp_index].empty())
    {
        TCHAR cmd[256];
        _stprintf(cmd, _T("open assets\\mvp_%s.mp3 alias mvp"),mvp_names[current_mvp_index].c_str());
        mciSendString(cmd,nullptr,0,nullptr);
    }
}

void handle_input()
{
    static auto space_down=false,esc_down=false;
    if(GetAsyncKeyState(VK_SPACE)&0x8000)
    {
        if(!space_down)
        {
            paused^=1;
            just_unpaused=!paused;
            space_down=true;
        }
    }
    else
        space_down=false;
    if(GetAsyncKeyState(VK_ESCAPE)&0x8000)
    {
        if(!esc_down)
        {
            running=false;
            esc_down=true;
        }
    }
    else
        esc_down=false;
}

void display_stats(const int fps,const double speed)
{
    settextcolor(GREEN);
    settextstyle(30,0,"微软雅黑");
    LOGFONT f;
    gettextstyle(&f);
    f.lfQuality=ANTIALIASED_QUALITY;
    f.lfOutPrecision=OUT_TT_PRECIS;
    setbkmode(OPAQUE);
    setbkcolor(BLACK);
    settextstyle(&f);
    TCHAR buf[128];
    const char* name=mvp_names[current_mvp_index].empty()?"(无)":mvp_names[current_mvp_index].c_str();
    _stprintf(buf, _T("当前音乐盒:《%s》  灵敏度:%.2f  FPS:%d"),name,speed,fps);
    SIZE sz;
    GetTextExtentPoint32(GetImageHDC(nullptr),buf, _tcslen(buf),&sz);
    outtextxy(WIN_WIDTH-sz.cx-10,10,buf);
    settextcolor(WHITE);
    for(auto i=0,y=10;i<7;++i,y+=30)
    {
        const TCHAR* tips[]={
            _T("(easyx库重置了鼠标状态因此无法隐藏指针)"),
            _T("      提示"),
            _T("空格:暂停"),
            _T("ESC:退出"),
            _T("滚轮:调灵敏度"),
            _T("左键:开枪"),
            _T("右键:切换音乐盒")
        };
        outtextxy(10,y,tips[i]);
    }
}

void run_game()
{
    srand(static_cast<unsigned>(time(nullptr)));
    BeginBatchDraw();
    auto cursorVisible=false;
    SetCursor(nullptr);
    while(ShowCursor(FALSE)>=0) {}
    POINT center{WIN_WIDTH/8,WIN_HEIGHT/8};
    ClientToScreen(g_hwnd,&center);
    SetCursorPos(center.x,center.y);

    int off_x=-361,off_y=-227;
    int wx=800-off_x,wy=900+off_y;
    auto ewx_pos=1370;
    using Clock=std::chrono::high_resolution_clock;
    auto last_time=Clock::now(),last_frame=last_time;
    auto frame_count=0,fps=0;
    auto speed=1.0;
    const int BG_H=bg.getheight();
    const int EN_W=enemy.getwidth(),EN_H=enemy.getheight();

    while(true)
    {
        handle_input();
        if(!running)
        {
            break;
        }

        constexpr auto ewy_pos=800;
        auto now=Clock::now();
        const float dt=std::chrono::duration<float>(now-last_frame).count();
        last_frame=now;

        ExMessage em{};
        while(peekmessage(&em, EM_MOUSE))
        {
            if(em.message==WM_MOUSEWHEEL)
            {
                speed=std::clamp(speed+em.wheel/120*0.1,0.1,5.0);
            }
            else if(em.message==WM_RBUTTONDOWN)
            {
                switch_mvp();
            }
            else if(em.message==WM_LBUTTONDOWN&&!paused)
            {
                mciSendString(_T("play shoot from 0"),nullptr,0,nullptr);
                mciSendString(_T("setaudio mvp volume to 800"),nullptr,0,nullptr);
                const bool hitX=wx>=ewx_pos&&wx<=ewx_pos+EN_W;
                const bool head=wy<=ewy_pos&&wy>=ewy_pos-EN_W;
                if(const bool body=wy<=ewy_pos-EN_W&&wy>=ewy_pos-EN_H;hitX&&(head||body))
                {
                    if(head)
                    {
                        mciSendString(_T("play headshot from 0"),nullptr,0,nullptr);
                        hit_events.push_back({"击中头部",Clock::now()});
                    }
                    else
                    {
                        hit_events.push_back({"击中身体",Clock::now()});
                    }
                    mciSendString(_T("play mvp from 0"),nullptr,0,nullptr);
                }
            }
        }

        if(paused&&!cursorVisible)
        {
            while(ShowCursor(TRUE)<0) {}
            cursorVisible=true;
        }
        else if(!paused&&cursorVisible)
        {
            while(ShowCursor(FALSE)>=0) {}
            cursorVisible=false;
        }

        if(++frame_count,std::chrono::duration_cast<std::chrono::milliseconds>(now-last_time).count()>=1000)
        {
            fps=frame_count;
            frame_count=0;
            last_time=now;
        }

        if(!paused)
        {
            cleardevice();
            if(just_unpaused)
            {
                SetCursorPos(center.x,center.y);
                just_unpaused=false;
                continue;
            }
            POINT p;
            GetCursorPos(&p);
            ScreenToClient(g_hwnd,&p);
            off_x-=static_cast<int>((p.x-WIN_WIDTH/8)*speed);
            off_y-=static_cast<int>((p.y-WIN_HEIGHT/8)*speed);
            wx=std::clamp(800-off_x,450,1920);
            wy=std::clamp(900+off_y,340,1000);
            SetCursorPos(center.x,center.y);
            off_x=800-wx;
            off_y=wy-900;
            ewx_pos=ewx_pos<=450?1370+rand()%4000:ewx_pos-static_cast<int>(1000*dt);
            const int ex=off_x+ewx_pos;
            const int ey=off_y+(BG_H-ewy_pos);
            putimage(off_x,off_y,&bg);
            putimage_alpha(ex,ey,&enemy);
            putimage_alpha(off_x,off_y,&door);
            putimage_alpha(0,0,&mask);
        }

        display_stats(fps,speed);

        auto now2=Clock::now();
        hit_events.erase(std::remove_if(hit_events.begin(),hit_events.end(),
                                        [&](auto&e)
                                        {
                                            return std::chrono::duration<double>(now2-e.time).count()>2.0;
                                        }),
                         hit_events.end());
        constexpr int startX=WIN_WIDTH-200;
        setlinecolor(YELLOW);
        setbkmode(TRANSPARENT);
        settextstyle(30,0,"微软雅黑");
        for(size_t i=0;i<hit_events.size();++i)
        {
            constexpr auto startY=40;
            auto&[text, time]=hit_events[i];
            const char* txt=text.c_str();
            const size_t len=text.length();
            SIZE sz;
            GetTextExtentPoint32(GetImageHDC(nullptr),txt,len,&sz);
            const int boxW=sz.cx+12;
            const int boxH=sz.cy+12;
            const int y=startY+static_cast<int>(i)*(boxH+4);
            rectangle(startX,y,startX+boxW,y+boxH);
            outtextxy(startX+6,y+6,txt);
        }
        setbkmode(OPAQUE);

        FlushBatchDraw();
    }

    EndBatchDraw();
    if(!cursorVisible)
    {
        while(ShowCursor(TRUE)<0) {}
    }
}

void init()
{
    dpi_awareness();
    g_hwnd=create_window();
    load_assets();
}

int main()
{
    init();
    run_game();
    closegraph();
    return 0;
}
