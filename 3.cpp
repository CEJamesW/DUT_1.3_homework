#include <conio.h>
#include <graphics.h>
#include <windows.h>

COLORREF drawColor=WHITE;

int main()
{
	initgraph(1024,768);
	setbkcolor(BLACK);
	cleardevice();

	ExMessage msg{};

	while(true)
	{
		if(peekmessage(&msg, EM_MOUSE|EM_KEY))
		{
			if(msg.message==WM_LBUTTONDOWN||msg.message==WM_RBUTTONDOWN)
			{
				const bool ctrlPressed=GetAsyncKeyState(VK_CONTROL)&0x8000;

				setfillcolor(drawColor);
				setlinecolor(drawColor);

				if(msg.message==WM_LBUTTONDOWN)
				{
					const int size=ctrlPressed?20:10;
					fillrectangle(msg.x-size/2,msg.y-size/2,msg.x+size/2,msg.y+size/2);
				}
				else if(msg.message==WM_RBUTTONDOWN)
				{
					const int radius=ctrlPressed?20:10;
					fillcircle(msg.x,msg.y,radius);
				}
			}

			if(msg.message==WM_KEYDOWN)
			{
				switch(msg.vkcode)
				{
				case 'C':
					cleardevice();
					break;
				case 'R':
					drawColor=RED;
					break;
				case 'G':
					drawColor=GREEN;
					break;
				case 'B':
					drawColor=BLUE;
					break;
				case 'W':
					drawColor=WHITE;
					break;
				case VK_ESCAPE:
					closegraph();
					return 0;
				default:;
				}
			}
		}
	}
}
