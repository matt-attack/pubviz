
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

#define GLEW_STATIC
#include <GL/glew.h>

#ifndef _WIN32
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#endif

#include "../Plugin.h"
#include "../properties.h"

class GridPlugin: public pubviz::Plugin
{
	std::unique_ptr<FloatProperty> start_x_;
	std::unique_ptr<FloatProperty> start_y_;
	
	std::unique_ptr<NumberProperty> x_count_;
	std::unique_ptr<NumberProperty> y_count_;
	
	std::unique_ptr<FloatProperty> cell_size_;
	
	std::unique_ptr<ColorProperty> color_;
	
public:
	virtual ~GridPlugin()
	{
	}
	
	virtual void Update()
	{
	
	}


	// Clear out any historical data so the view gets cleared
	virtual void Clear()
	{

	}
	
	virtual void Render()
	{
		glLineWidth(2.0f);
		glBegin(GL_LINES);
		
		Gwen::Color color = color_->GetValue();
		glColor3f(color.r/255.0, color.g/255.0, color.b/255.0);
		
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
		x_count_ = AddNumberProperty(tree, "X Count", 10, 0, 1000, 1, "Number of grid cells along X axis.");
		y_count_ = AddNumberProperty(tree, "Y Count", 10, 0, 1000, 1, "Number of grid cells along Y axis.");
		
		start_x_ = AddFloatProperty(tree, "Start X", -50.0, -100000, 100000, 1, "Left X position of grid.");
		start_y_ = AddFloatProperty(tree, "Start Y", -50.0, -100000, 100000, 1, "Bottom Y position of grid.");
		
		cell_size_ = AddFloatProperty(tree, "Cell Size", 10.0, 0, 1000.0, 1.0, "Side dimension of grid cells.");
		
		color_ = AddColorProperty(tree, "Color", Gwen::Color(125,125,125));
	}
	
	std::string GetTitle() override
	{
		return "Grid";
	}
};

REGISTER_PLUGIN("grid", GridPlugin)

#endif
