

#include <pubsub/Node.h>

#include <stdlib.h>
#include <memory.h>

#include "../controls/pubviz.h"

#include <Gwen/Gwen.h>
#include <Gwen/Application.h>
#include <Gwen/Renderers/OpenGL.h>

#ifdef WIN32
// disables the console on windows
int WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd
)
{
	int argc;
	wchar_t** args = CommandLineToArgvW(GetCommandLineW(), &argc);
#else
int main(int argc, char** args)
{
#endif
	//skin->Init("DefaultSkin.png");
	//skin->SetDefaultFont(L"Open Sans", 14);
	
	Gwen::Application<Gwen::Renderer::OpenGL> app;
	//app.SetDefaultFont("Open Sans", 10);

	auto window = app.AddWindow("Pubviz", 700, 500);
    window->SetMinimumSize(Gwen::Point(100, 100));
	PubViz* ppUnit = new PubViz(window);
	ppUnit->SetPos(0, 0);
	
	// Wait for exit, use this instead of spin
	while (app.Okay())
	{
		if (!app.SpinOnce())
		{
			break;
		}
		
		//if (!window->NeedsRedraw())
		{
			// If we dont need a redraw, sleep until we get new input
			//Gwen::Platform::WaitForEvent();
		}
	}

	return 0;
}
