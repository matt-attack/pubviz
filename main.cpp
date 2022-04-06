
#include <stdlib.h>
#include <memory.h>

#include "pubviz.h"

#include <Gwen/Gwen.h>
#include <Gwen/Application.h>

#include <pubsub/Node.h>

int main(int argc, char** args)
{
	//skin->Init("DefaultSkin.png");
	//skin->SetDefaultFont(L"Open Sans", 14);
	
	Gwen::Application app;
	app.SetDefaultFont("Open Sans", 10);

	PubViz* ppUnit = new PubViz(app.AddWindow("Pubviz", 700, 500));
	ppUnit->SetPos(0, 0);
	
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
