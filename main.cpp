
#include <stdlib.h>
#include <memory.h>

#include "pubviz.h"

#include <Gwen/Gwen.h>
#include <Gwen/Application.h>

int main(int argc, char** args)
{
	//skin->Init("DefaultSkin.png");
	//skin->SetDefaultFont(L"Open Sans", 14);
	
	Gwen::Application app;
	app.SetDefaultFont("Open Sans", 10);

	PubViz* ppUnit = new PubViz(app.AddWindow("Pubviz", 700, 500));
	ppUnit->SetPos(0, 0);
	
	app.Spin();

	return 0;
}
