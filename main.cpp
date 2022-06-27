

#include <pubsub/Node.h>

#include <stdlib.h>
#include <memory.h>

#include "pubviz.h"

#include <Gwen/Gwen.h>
#include <Gwen/Application.h>
#include <Gwen/Renderers/OpenGL.h>

int main(int argc, char** args)
{
	//skin->Init("DefaultSkin.png");
	//skin->SetDefaultFont(L"Open Sans", 14);
	
	Gwen::Application<Gwen::Renderer::OpenGL> app;
	//app.SetDefaultFont("Open Sans", 10);

	auto window = app.AddWindow("Pubviz", 700, 500);
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
