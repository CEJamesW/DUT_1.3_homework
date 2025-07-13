#include <conio.h>
#include <graphics.h>
#include <bits/stdc++.h>
using namespace std;

double to_rad(const double deg)
{
    return deg*M_PI/180.0;
}

void draw_clockface(const int cx,const int cy,const int r)
{
    setlinecolor(BLACK);
    setlinestyle(PS_SOLID,2);
    circle(cx,cy,r);

    for(auto i=0;i<60;i++)
    {
        const double ang=to_rad(i*6);
        const int len=i%5==0?25:20;
        const int x1=cx+static_cast<int>((r-10)*sin(ang));
        const int y1=cy-static_cast<int>((r-10)*cos(ang));
        const int x2=cx+static_cast<int>((r-len)*sin(ang));
        const int y2=cy-static_cast<int>((r-len)*cos(ang));
        setlinestyle(PS_SOLID,i%5==0?3:1);
        line(x1,y1,x2,y2);
    }

    settextstyle(30,0,"Î¢ÈíÑÅºÚ");
    LOGFONT f;
    gettextstyle(&f);
    f.lfQuality=ANTIALIASED_QUALITY;
    f.lfOutPrecision=OUT_TT_PRECIS;
    settextstyle(&f);
    setbkmode(TRANSPARENT);
    settextcolor(BLACK);
    for(auto h=1;h<=12;h++)
    {
        const double angDeg=h*30.0-90;
        const double ang=to_rad(angDeg);
        const int tx=cx+static_cast<int>((r-40)*cos(ang));
        const int ty=cy+static_cast<int>((r-40)*sin(ang));

        char buf[3];
        sprintf(buf,"%d",h);

        const int tw=textwidth(buf);
        const int th=textheight(buf);
        outtextxy(tx-tw/2,ty-th/2,buf);
    }
}

void draw_hands(const int cx,const int cy,const int r,
                const double h,const double m,const double s)
{
    const double aH=to_rad(h*30+m*0.5);
    setlinecolor(BLACK);
    setlinestyle(PS_SOLID,6);
    line(cx,cy,
         cx+static_cast<int>((r-80)*sin(aH)),
         cy-static_cast<int>((r-80)*cos(aH)));

    const double aM=to_rad(m*6+s*6.0/60);
    setlinestyle(PS_SOLID,4);
    line(cx,cy,
         cx+static_cast<int>((r-50)*sin(aM)),
         cy-static_cast<int>((r-50)*cos(aM)));

    const double aS=to_rad(s*6);
    setlinecolor(RED);
    setlinestyle(PS_SOLID,2);
    const int sx=cx+static_cast<int>((r-30)*sin(aS));
    const int sy=cy-static_cast<int>((r-30)*cos(aS));
    line(cx,cy,sx,sy);

    const int bx=cx-static_cast<int>(20*sin(aS));
    const int by=cy+static_cast<int>(20*cos(aS));
    line(cx,cy,bx,by);

    setfillcolor(BLACK);
    solidcircle(cx,cy,5);
    setfillcolor(WHITE);
    solidcircle(cx,cy,2);
}

int main()
{
    constexpr auto W=500,H=500;
    initgraph(W,H);
    setbkcolor(WHITE);

    BeginBatchDraw();
    while(!_kbhit())
    {
        constexpr auto R=200;
        SYSTEMTIME st;
        GetLocalTime(&st);
        const HWND hWnd=GetHWnd();
        char timeStr[9];
        sprintf(timeStr,"%02d:%02d:%02d",st.wHour,st.wMinute,st.wSecond);
        SetWindowText(hWnd,timeStr);
        const double smoothSec=st.wSecond+st.wMilliseconds/1000.0;

        cleardevice();

        draw_clockface(W/2,H/2,R);
        draw_hands(W/2,H/2,R,st.wHour%12,st.wMinute,smoothSec);

        FlushBatchDraw();
        Sleep(10);
    }
    EndBatchDraw();
    closegraph();
    return 0;
}
