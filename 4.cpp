#include <conio.h>
#include <graphics.h>
#include <windows.h>
#include <bits/stdc++.h>

constexpr auto WIN_WIDTH=1024;
constexpr auto WIN_HEIGHT=768;

constexpr auto BALL_RADIUS=10;
int ball_x=WIN_WIDTH/2;
int ball_y=WIN_HEIGHT/2;
auto ball_vx=4;
auto ball_vy=4;

constexpr auto PADDLE_WIDTH=100;
constexpr auto PADDLE_HEIGHT=10;
int paddle_x=(WIN_WIDTH-PADDLE_WIDTH)/2;
constexpr int PADDLE_Y=WIN_HEIGHT-30;
constexpr auto PADDLE_SPEED=8;

auto isPaused=true;

void draw_frame()
{
    cleardevice();
    solidcircle(ball_x,ball_y,BALL_RADIUS);
    rectangle(paddle_x,PADDLE_Y,
              paddle_x+PADDLE_WIDTH,
              PADDLE_Y+PADDLE_HEIGHT);
}

void update_ball()
{
    ball_x+=ball_vx;
    ball_y+=ball_vy;

    if(ball_x-BALL_RADIUS<=0||ball_x+BALL_RADIUS>=WIN_WIDTH)
    {
        ball_vx=-ball_vx;
        ball_x+=ball_vx;
    }
    if(ball_y-BALL_RADIUS<=0)
    {
        ball_vy=-ball_vy;
        ball_y+=ball_vy;
    }
}

void check()
{
    if(ball_y+BALL_RADIUS>=PADDLE_Y&&
       ball_y+BALL_RADIUS<=PADDLE_Y+PADDLE_HEIGHT&&
       ball_x>=paddle_x&&
       ball_x<=paddle_x+PADDLE_WIDTH)
    {
        ball_vy=-ball_vy;
        ball_y+=ball_vy;
    }
}

void handle_input()
{
    static auto pDown=false;

    if(GetAsyncKeyState('P')&0x8000)
    {
        if(!pDown)
        {
            isPaused=!isPaused;
            pDown=true;
        }
    }
    else
    {
        pDown=false;
    }

    if(!isPaused)
    {
        if(GetAsyncKeyState('A')&0x8000||GetAsyncKeyState(VK_LEFT)&0x8000)
        {
            paddle_x=std::max(paddle_x-PADDLE_SPEED,0);
        }
        if(GetAsyncKeyState('D')&0x8000||GetAsyncKeyState(VK_RIGHT)&0x8000)
        {
            paddle_x=std::min(paddle_x+PADDLE_SPEED,WIN_WIDTH-PADDLE_WIDTH);
        }
    }
}

int main()
{
    initgraph(WIN_WIDTH,WIN_HEIGHT);

    BeginBatchDraw();
    while(true)
    {
        handle_input();
        if(!isPaused)
        {
            update_ball();
            check();
        }
        draw_frame();
        FlushBatchDraw();

        if(ball_y-BALL_RADIUS>WIN_HEIGHT)
        {
            break;
        }

        Sleep(10);
    }
    EndBatchDraw();

    settextstyle(40,0,"Œ¢»Ì—≈∫⁄");
    LOGFONT f;
    gettextstyle(&f);
    f.lfQuality=ANTIALIASED_QUALITY;
    f.lfOutPrecision=OUT_TT_PRECIS;
    setbkmode(TRANSPARENT);
    settextstyle(&f);
    setcolor(WHITE);
    const auto s="”Œœ∑Ω· ¯";
    const int tw=textwidth(s);
    const int th=textheight(s);
    const int tx=(WIN_WIDTH-tw)/2;
    const int ty=(WIN_HEIGHT-th)/2;
    outtextxy(tx,ty,s);

    _getch();
    closegraph();
    return 0;
}
