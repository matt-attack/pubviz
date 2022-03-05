
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
		glColor3f(0.5, 0.5, 0.5);
		
		for (double x = start_x_; x < start_x_ + (grid_count_x_+0.001)*cell_size_; x += cell_size_)
		{
			glVertex2f(x, start_y_);
			glVertex2f(x, start_y_ + grid_count_y_*cell_size_);
		}
		
		for (double y = start_y_; y < start_y_ + (grid_count_y_+0.001)*cell_size_; y += cell_size_)
		{
			glVertex2f(start_x_, y);
			glVertex2f(start_x_ + grid_count_y_*cell_size_, y);
		}

		glEnd();
	}
	
	virtual void Initialize(Gwen::Controls::Properties* tree)
	{
		// add any properties
		tree->Add("X Count");
		tree->Add("Y Count");
		tree->Add("Resolution");
	}
	
	std::string GetTitle() override
	{
		return "Grid";
	}
};

#endif
