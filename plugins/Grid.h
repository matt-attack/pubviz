
#ifndef PUBVIZ_GRID_H
#define PUBVIZ_GRID_H

#include <Gwen/Gwen.h>
#include <Gwen/Align.h>
#include <Gwen/Utility.h>
#include <Gwen/Controls/WindowControl.h>
#include <Gwen/Controls/TabControl.h>
#include <Gwen/Controls/ListBox.h>
#include <Gwen/Controls/DockBase.h>
#include <Gwen/Controls/StatusBar.h>
#include <Gwen/Controls/PropertyTree.h>


#include <GL/glew.h>

#ifndef _WIN32
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#endif

#include "../Plugin.h"

class GridPlugin: public Plugin
{
	double start_x_ = -50;
	double start_y_ = -50;
	
	int grid_count_x_ = 10;
	int grid_count_y_ = 10;
	
	double cell_size_ = 10;
	
public:
	virtual ~GridPlugin()
	{
	
	}
	
	virtual void Render()
	{
		glLineWidth(2.0f);
		glBegin(GL_LINES);
		// Red line to the right
		glColor3f(0.5, 0.5, 0.5);
		for (int i = -150; i <= 150; i += cell_size_)
		{
			glVertex2f(i, -150);
			glVertex2f(i, 150);
 	
			glVertex2f(-150, i);
			glVertex2f(150, i);
		}
		glEnd();
	}
	
	virtual void Initialize(Gwen::Controls::Properties* tree)
	{
		// add any properties
	}
};

#endif
