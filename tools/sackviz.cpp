
#include <stdlib.h>
#include <memory.h>

#include "../controls/sackviz.h"

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

	auto control = new SackViz(app.AddWindow("Sackviz", 700, 500));
	if (argc > 1)
		control->Open(args[1]);
	control->SetPos(0, 0);
	
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
