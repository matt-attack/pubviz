

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
	char** args = __argv;
	int argc = __argc;
#else
int main(int argc, char** args)
{
#endif
	Gwen::Application<Gwen::Renderer::OpenGL> app;

	auto window = app.AddWindow("Pubviz", 700, 500);
  window->SetMinimumSize(Gwen::Point(100, 100));
	PubViz* ppUnit = new PubViz(window);
	ppUnit->SetPos(0, 0);
	if (argc > 1)
	{
		ppUnit->LoadConfig(args[1]);
	}
	// Wait for exit, use this instead of spin
	while (app.Okay())
	{
		if (!app.SpinOnce())
		{
			break;
		}
	}

	return 0;
}
