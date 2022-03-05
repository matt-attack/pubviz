
#include <stdlib.h>
#include <memory.h>

#include "pubviz.h"
#include <Gwen/Gwen.h>
#include <Gwen/Skins/TexturedBase.h>

#include <Gwen/Renderers/OpenGL.h>

#include <Gwen/Controls/WindowCanvas.h>
#include <Gwen/Platform.h>


Gwen::Skin::TexturedBase* skin = 0;
std::vector<Gwen::Controls::WindowCanvas*> canvases;

Gwen::Controls::WindowCanvas* OpenWindow(const char* name, int w, int h)
{
	Gwen::Renderer::OpenGL* renderer = new Gwen::Renderer::OpenGL();

	auto skin = new Gwen::Skin::TexturedBase(renderer);

	Gwen::Controls::WindowCanvas* window_canvas = new Gwen::Controls::WindowCanvas(-1, -1, w, h, skin, name);
	window_canvas->SetSizable(true);

	skin->Init("DefaultSkin.png");
	skin->SetDefaultFont(L"Segoe UI", 11);
	
	PubViz* ppUnit = new PubViz(window_canvas);
	ppUnit->SetPos(0, 0);

	canvases.push_back(window_canvas);

	return window_canvas;
}

int main(int argc, char** args)
{
	Gwen::Renderer::OpenGL* renderer = new Gwen::Renderer::OpenGL();

	skin = new Gwen::Skin::TexturedBase(renderer);

	auto window_canvas = new Gwen::Controls::WindowCanvas(-1, -1, 700.0f, 500.0f, skin, "Pubviz");
	window_canvas->SetSizable(true);

	skin->Init("DefaultSkin.png");
	//skin->SetDefaultFont(L"Open Sans", 14);

	PubViz* ppUnit = new PubViz(window_canvas);
	ppUnit->SetPos(0, 0);

	while (!window_canvas->WantsQuit())
	{
		//save power if we arent on top
		bool on_top = false;
		if (window_canvas->IsOnTop())
			on_top = true;

		window_canvas->DoThink();

		//update other cavases
		for (int i = 0; i < canvases.size(); i++)
		{
			auto canv = canvases[i];
			if (canv->IsOnTop())
				on_top = true;

			if (canv->WantsQuit())
			{
				printf("window wants cliff\n");
				//remove it
				canvases.erase(canvases.begin() + i);
				auto render = canv->GetSkin()->GetRender();
				auto skin = canv->GetSkin();
				delete canv;
				delete skin;
				delete render;

				i--;
				continue;
			}

			canv->DoThink();
		}

		if (on_top == false)
			Gwen::Platform::Sleep(100);
	}
	delete window_canvas;
	return 0;
}
