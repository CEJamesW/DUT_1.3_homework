#include <conio.h>
#include <graphics.h>
#include <string>
using namespace std;

int main()
{
    constexpr auto board_lines=15;
    constexpr auto cell_size=40;
    constexpr auto margin=20;
    constexpr int width=margin*2+cell_size*(board_lines-1);
    constexpr int height=width;
    POINT star_points[]={
        {3,3},{11,3},{7,7},{3,11},{11,11}
    };

    initgraph(width,height);
    setbkcolor(RGB(205,133,63));
    cleardevice();
    setlinecolor(BLACK);
    for(auto i=0;i<board_lines;i++)
    {
        const int x=margin+i*cell_size;
        line(x,margin,x,margin+(board_lines-1)*cell_size);
        line(margin,x,margin+(board_lines-1)*cell_size,x);
    }
    for(auto i=0;i<2;i++)
    {
        line(margin+i,margin+i,margin+(board_lines-1)*cell_size-i,margin+i);
        line(margin+i,margin+(board_lines-1)*cell_size-i,
             margin+(board_lines-1)*cell_size-i,margin+(board_lines-1)*cell_size-i);
        line(margin+i,margin+i,margin+i,margin+(board_lines-1)*cell_size-i);
        line(margin+(board_lines-1)*cell_size-i,margin+i,
             margin+(board_lines-1)*cell_size-i,margin+(board_lines-1)*cell_size-i);
    }
    setfillcolor(BLACK);
    for(const auto&[x, y]:star_points)
    {
        constexpr auto starRadius=6;
        const int cx=margin+x*cell_size;
        const int cy=margin+y*cell_size;
        solidcircle(cx,cy,starRadius);
    }

    settextstyle(18,0,"Courier");
    LOGFONT f;
    gettextstyle(&f);
    f.lfQuality=ANTIALIASED_QUALITY;
    f.lfOutPrecision=OUT_TT_PRECIS;
    settextstyle(&f);
    setbkmode(TRANSPARENT);
    settextcolor(BLACK);

    for(auto i=0;i<board_lines;i++)
    {
        const int y_pos=margin+(board_lines-1-i)*cell_size;
        string numStr=to_string(i+1);
        RECT r={margin-25,y_pos-8,margin-5,y_pos+8};
        drawtext(numStr.c_str(),&r,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
    }

    constexpr int boardBottom=margin+(board_lines-1)*cell_size;
    for(auto i=0;i<board_lines;i++)
    {
        const auto letter=static_cast<char>('A'+i);
        string basic_string(1,letter);
        const int x_pos=margin+i*cell_size;
        RECT r={x_pos-10,boardBottom+5,x_pos+10,boardBottom+25};
        drawtext(basic_string.c_str(),&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    }

    _getch();
    closegraph();
    return 0;
}
