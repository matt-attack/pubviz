
#include <stdlib.h>
#include <memory.h>

#include "../controls/canviz.h"

#include <Gwen/Gwen.h>
#include <Gwen/Application.h>
#include <Gwen/Renderers/OpenGL.h>

#include <pubsub/Node.h>

int main(int argc, char** args)
{
	//skin->Init("DefaultSkin.png");
	//skin->SetDefaultFont(L"Open Sans", 14);
	
	Gwen::Application<Gwen::Renderer::OpenGL> app;
	//app.SetDefaultFont("Open Sans", 10);

	auto window = app.AddWindow("CANViz", 700, 550);
	auto control = new CANViz(window);
	//if (argc > 1)
	//	control->Open(args[1]);
	control->SetPos(0, 0);
	window->SizeToChildren();
	window->SetMinimumSize(window->GetSize());
	
	// Wait for exit, use this instead of spin
	while (ps_okay())
	{
		if (!app.SpinOnce())
		{
			break;
		}
	}

	return 0;
}
