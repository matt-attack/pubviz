
#ifndef PUBVIZ_PLUGIN_MEASURE_H
#define PUBVIZ_PLUGIN_MEASURE_H

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

#include <vector>

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

#include <pubsub/Publisher.h>

class MeasurePlugin : public pubviz::Plugin
{
	FloatProperty* alpha_;
	ColorProperty* color_;
	NumberProperty* line_width_;
	NumberProperty* point_size_;

	ButtonProperty*	clear_;

public:

	MeasurePlugin()
	{
		// dont use pubsub here
	}

	virtual ~MeasurePlugin()
	{
		delete color_;
		delete alpha_;
		delete line_width_;
		//delete show_points_;
		delete point_size_;
		delete clear_;
	}

	// Clear out any historical data so the view gets cleared
	virtual void Clear()
	{
		points_.clear();
		UpdatePoints();
	}

	virtual void Update()
	{

	}

	struct Vertex
	{
		double x;
		double y;
		double z;

		Vertex() {}
		Vertex(double x, double y, double z)
		{
			this->x = x;
			this->y = y;
			this->z = z;
		}
	};

	std::vector<Vertex> points_;
	std::vector<Vertex> transformed_pts_;

	virtual void Paint()
	{
		// draw labels between each point
		auto r = GetCanvas()->GetSkin()->GetRender();

		for (int i = 0; i < ((int)transformed_pts_.size())-1; i++)
		{
			auto start = transformed_pts_[i];
			auto end = transformed_pts_[i+1];

			Vertex middle;
			middle.x = (start.x + end.x)*0.5;
			middle.y = (start.y + end.y)*0.5;
			middle.z = (start.z + end.z)*0.5;

			int x, y;
			GetCanvas()->WorldToPixel(middle.x, middle.y, middle.z, x, y);
			
			double distance = std::sqrt(std::pow(start.x - end.x, 2.0) + 
										std::pow(start.y - end.y, 2.0) + 
										std::pow(start.z - end.z, 2.0));
			r->SetDrawColor( Gwen::Color(255,255,255,255) );
			char buf[50];
			sprintf(buf, "%g", distance);
			r->RenderText(GetCanvas()->GetSkin()->GetDefaultFont(), Gwen::PointF( x, y ), (std::string)buf);
		}
	}

	void UpdatePoints()
	{
		// Now transform the points
		transformed_pts_.clear();
		transformed_pts_.reserve(points_.size());
		for (auto& pt : points_)
		{

		}
		Redraw();
	}

	virtual void Render()
	{
		if (points_.size() == 0)
		{
			return;
		}

		Gwen::Color color = color_->GetValue();

		//UpdatePoints();

		// Now render the points
		glLineWidth(line_width_->GetValue());
		glBegin(GL_LINE_STRIP);
		glColor3f(color.r / 255.0, color.g / 255.0, color.b / 255.0);
		for (const auto& vert : transformed_pts_)
		{
			glVertex3f(vert.x, vert.y, vert.z);
		}
		glEnd();

		if (true)//show_points_->GetValue())
		{
			glPointSize(point_size_->GetValue());
			glBegin(GL_POINTS);
			glColor3f(color.r / 255.0, color.g / 255.0, color.b / 255.0);
			for (const auto& vert : transformed_pts_)
			{
				glVertex3f(vert.x, vert.y, vert.z);
			}
			glEnd();
		}
	}

	virtual void Initialize(Gwen::Controls::Properties* tree)
	{
		// add any properties
		alpha_ = AddFloatProperty(tree, "Alpha", 1.0, 0.0, 1.0, 0.1);

		color_ = AddColorProperty(tree, "Color", Gwen::Color(255, 50, 50));

		line_width_ = AddNumberProperty(tree, "Line Width", 4, 1, 100, 2);

		point_size_ = AddNumberProperty(tree, "Point Size", 6, 1, 100, 2);

		clear_ = AddButtonProperty(tree, "Clear");
		clear_->onChange = [this]()
		{
			points_.clear();
			UpdatePoints();
		};
	}

	std::string GetTitle() override
	{
		return "Measure";
	}

	// Applies only for 2d
	virtual bool OnMapClick(double x, double y)
	{
		points_.push_back(Vertex(x,y,0));

		UpdatePoints();
		return true;
//okay, need to get the frame for this, then add to list on tap
	}
};

REGISTER_PLUGIN("measure", MeasurePlugin)

#endif
