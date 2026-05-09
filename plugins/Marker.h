
#ifndef PUBVIZ_PLUGIN_MARKER_H
#define PUBVIZ_PLUGIN_MARKER_H

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

#include <pubsub/Marker.msg.h>

class MarkerPlugin: public pubviz::Plugin
{
	std::unique_ptr<FloatProperty> alpha_;
	std::unique_ptr<ColorProperty> color_;
	std::unique_ptr<NumberProperty> line_width_;
	
	std::unique_ptr<TopicProperty> topic_;
	
	pubsub::Subscriber<pubsub::msg::Marker>::Ptr subscriber_;
	
	pubsub::msg::Marker last_msg_;
	
	std::map<int, pubsub::msg::Marker> markers_;
	
	std::string current_topic_;
	void Subscribe(std::string str)
	{
		subscriber_.reset();
		
		Clear();
		
		current_topic_ = str;
    subscriber_.reset(new pubsub::Subscriber<pubsub::msg::Marker>(*GetNode(), current_topic_, [](const pubsub::msg::MarkerSharedPtr& msg){}, 100, 1));
	}
	
public:

	MarkerPlugin()
	{	
		// dont use pubsub here
	}
	
	virtual ~MarkerPlugin()
	{

	}
	
	virtual void Update()
	{
		// process any messages
		if (subscriber_)
		{
			while (auto data = subscriber_->PopOne())
			{
				if (Paused())
				{
					continue;
				}

				// user is responsible for freeing the message and its arrays
				markers_[data->id] = *data;
				
				Redraw();
			}
		}
	}

	// Clear out any historical data so the view gets cleared
	virtual void Clear()
	{
		markers_.clear();
	}
		
	virtual void Render()
	{	
		for (auto& marker: markers_)
		{
			RenderMarker(marker.second);
		}
	}
	
	void RenderMarker(const pubsub::msg::Marker& last_msg_)
	{
		// draw the marker
		Gwen::Color color = color_->GetValue();
		glLineWidth(line_width_->GetValue());
		auto canvas = GetCanvas();
		if (last_msg_.marker_type == pubsub::msg::Marker::LINE_LIST_2D)
		{
			// 2d lines
			glBegin(GL_LINES);
			for (int i = 0; i + 1 < last_msg_.data.size(); i += 2)
			{
				int ci = i / 2;
				if (ci < last_msg_.colors.size())
				{
					uint32_t c = last_msg_.colors[ci];
					uint8_t r = (c & 0xFF0000) >> 16;
					uint8_t g = (c & 0xFF00) >> 8;
					uint8_t b = (c & 0xFF);
					glColor3f(r / 255.0, g / 255.0, b / 255.0);
				}
				else
				{
					glColor3f(color.r / 255.0, color.g / 255.0, color.b / 255.0);
				}

				Vec3d pos(last_msg_.data[i], last_msg_.data[i+1], 0);
				canvas->TransformToView(last_msg_.header.frame, pos);
				glVertex2f(pos.x, pos.y);
			}
			glEnd();
		}
		else if (last_msg_.marker_type == pubsub::msg::Marker::LINE_SEGMENTS_2D)
		{
			// 2d line segments
			int i = 0;
			int ci = 0;
			while (i < last_msg_.data.size())
			{
				int count = last_msg_.data[i];
				int end_index = i + count*2;
				i++;
				// draw a line segment
				glBegin(GL_LINE_STRIP);
				glColor3f(color.r/255.0, color.g/255.0, color.b/255.0);
				for (; i < std::min<int>(end_index, last_msg_.data.size()-1); i += 2)
				{
					if (ci < last_msg_.colors.size())
					{
						uint32_t c = last_msg_.colors[ci];
						uint8_t r = (c & 0xFF0000) >> 16;
						uint8_t g = (c & 0xFF00) >> 8;
						uint8_t b = (c & 0xFF);
						glColor3f(r / 255.0, g / 255.0, b / 255.0);
					}
					Vec3d pos(last_msg_.data[i], last_msg_.data[i+1], 0);
					canvas->TransformToView(last_msg_.header.frame, pos);
					glVertex2f(pos.x, pos.y);
				}
				glEnd();
				ci++;
			}
		}
		else if (last_msg_.marker_type == pubsub::msg::Marker::POLYGON_2D)
		{
			// 2d polygons (just draw outline atm)
			int i = 0;
			while (i < last_msg_.data.size())
			{
				int count = last_msg_.data[i];
				int start_index = i;
				int end_index = i + count*2;
				i++;
				// draw a line segment
				glBegin(GL_LINE_STRIP);
				glColor3f(color.r/255.0, color.g/255.0, color.b/255.0);
				for (; i < std::min<int>(end_index, last_msg_.data.size()-1); i += 2)
				{
					Vec3d pos(last_msg_.data[i], last_msg_.data[i+1], 0);
					canvas->TransformToView(last_msg_.header.frame, pos);
					glVertex2f(pos.x, pos.y);
				}
				Vec3d pos(last_msg_.data[start_index], last_msg_.data[start_index+1], 0);
				canvas->TransformToView(last_msg_.header.frame, pos);
				glVertex2f(pos.x, pos.y);
				glEnd();
			}
		}
		else if (last_msg_.marker_type == pubsub::msg::Marker::POINT_LIST_3D)
		{
			// 3d points with a radius in pixels? maybe negative can be pixels, positive in meters?
			for (int i = 0; i + 3 < last_msg_.data.size(); i += 4)
			{
				const double x = last_msg_.data[i];
				const double y = last_msg_.data[i+1];
				const double z = last_msg_.data[i+2];
				const double size = last_msg_.data[i+3];

				glPointSize(size);
				glBegin(GL_POINTS);
				int ci = i / 4;
				if (ci < last_msg_.colors.size())
				{
					uint32_t c = last_msg_.colors[ci];
					uint8_t r = (c & 0xFF0000) >> 16;
					uint8_t g = (c & 0xFF00) >> 8;
					uint8_t b = (c & 0xFF);
					glColor3f(r / 255.0, g / 255.0, b / 255.0);
				}
				else
				{
					glColor3f(color.r/255.0, color.g/255.0, color.b/255.0);
				}
				Vec3d pos(x, y, z);
				canvas->TransformToView(last_msg_.header.frame, pos);
				glVertex3f(pos.x, pos.y, pos.z);
				glEnd();
			}
		}
		else
		{
			// unknown
			printf("WARNING: unknown marker type\n");
		}
	}
	
	virtual void Initialize(Gwen::Controls::Properties* tree)
	{
		// add any properties
		alpha_ = AddFloatProperty(tree, "Alpha", 1.0, 0.0, 1.0, 0.1, "Marker transparency.");
		
		color_ = AddColorProperty(tree, "Color", Gwen::Color(255,50,50), "Default marker color.");
		
		topic_ = AddTopicProperty(tree, "Topic", "/marker", "", "pubsub__Marker");
		topic_->onChange = std::bind(&MarkerPlugin::Subscribe, this, std::placeholders::_1);
		
		line_width_ = AddNumberProperty(tree, "Line Width", 4, 1, 100, 2, "Width in pixels for marker lines.");
		
		Subscribe(topic_->GetValue());
	}
	
	std::string GetTitle() override
	{
		return "Marker";
	}
};

REGISTER_PLUGIN("marker", MarkerPlugin)

#endif
