
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
#include <Gwen/Controls/Property/Numeric.h>


#include <GL/glew.h>

#ifndef _WIN32
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#endif

#include "../Plugin.h"
#include "../properties.h"

class GridPlugin: public Plugin
{
	FloatProperty* start_x_;
	FloatProperty* start_y_;
	
	NumberProperty* x_count_;
	NumberProperty* y_count_;
	
	FloatProperty* cell_size_;
	
public:
	virtual ~GridPlugin()
	{
		delete start_x_;
		delete start_y_;
		delete x_count_;
		delete y_count_;
		delete cell_size_;
	}
	
	virtual void Render()
	{
		glLineWidth(2.0f);
		glBegin(GL_LINES);
		glColor3f(0.5, 0.5, 0.5);
		
		double cell_size = cell_size_->GetValue();
		int x_count = x_count_->GetValue();
		int y_count = y_count_->GetValue();
		
		double start_x = start_x_->GetValue();
		double start_y = start_y_->GetValue();
		
		for (double x = start_x; x < start_x + (x_count+0.001)*cell_size; x += cell_size)
		{
			glVertex2f(x, start_y);
			glVertex2f(x, start_y + y_count*cell_size);
		}
		
		for (double y = start_y; y < start_y + (y_count+0.001)*cell_size; y += cell_size)
		{
			glVertex2f(start_x, y);
			glVertex2f(start_x + x_count*cell_size, y);
		}

		glEnd();
	}
	
	virtual void Initialize(Gwen::Controls::Properties* tree)
	{
		// add any properties
		x_count_ = new NumberProperty(tree, "X Count", 10);
		y_count_ = new NumberProperty(tree, "Y Count", 10);
		
		start_x_ = new FloatProperty(tree, "Start X", -50.0);
		start_y_ = new FloatProperty(tree, "Start Y", -50.0);
		
		cell_size_ = new FloatProperty(tree, "Size", 10.0);
	}
	
	std::string GetTitle() override
	{
		return "Grid";
	}
};

#endif
