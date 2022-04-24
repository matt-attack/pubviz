
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

class MarkerPlugin: public Plugin
{
	FloatProperty* alpha_;
	ColorProperty* color_;
	NumberProperty* line_width_;
	
	TopicProperty* topic_;
	
	bool sub_open_ = false;
	ps_sub_t subscriber_;
	
	pubsub::msg::Marker last_msg_;
	
	std::map<int, pubsub::msg::Marker> markers_;
	
	void UpdateFromMessage()
	{

	}
	
	std::string current_topic_;
	void Subscribe(std::string str)
	{
		if (sub_open_)
		{
			ps_sub_destroy(&subscriber_);
		}
		
		markers_.clear();
		
		current_topic_ = str;
    	struct ps_subscriber_options options;
    	ps_subscriber_options_init(&options);
    	options.preferred_transport = 1;// tcp yo
    	ps_node_create_subscriber_adv(GetNode(), current_topic_.c_str(), &pubsub__Marker_def, &subscriber_, &options);
    	sub_open_ = true;
	}
	
public:

	MarkerPlugin()
	{	
		// dont use pubsub here
	}
	
	virtual ~MarkerPlugin()
	{
		delete color_;
		delete alpha_;
		delete line_width_;
		
		if (sub_open_)
		{
			ps_sub_destroy(&subscriber_);
			sub_open_ = false;
		}
	}
	
	virtual void Render()
	{
		// process any messages
		// our sub has a message definition, so the queue contains real messages
		pubsub::msg::Marker* data;
		if (sub_open_)
		{
			while (data = (pubsub::msg::Marker*)ps_sub_deque(&subscriber_))
			{
				// user is responsible for freeing the message and its arrays
				markers_[data->id] = *data;
				free(data->data);
				free(data);//todo use allocator free
			}
		}
		
		for (auto& marker: markers_)
		{
			RenderMarker(marker.second);
		}
	}
		
	void RenderMarker(const pubsub::msg::Marker& last_msg_)
	{
		// todo handle frames, lets just ignore them for the moment and assume everything is odom
		if (last_msg_.frame == 0)
		{
			return;// we cant render wgs84 atm
		}
		
		// draw the marker
		Gwen::Color color = color_->GetValue();
		glLineWidth(line_width_->GetValue());
		if (last_msg_.marker_type == 0)
		{
			// 2d lines
			glBegin(GL_LINES);
			glColor3f(color.r/255.0, color.g/255.0, color.b/255.0);
			for (int i = 0; i + 1 < last_msg_.data_length; i += 2)
			{
				glVertex2f(last_msg_.data[i], last_msg_.data[i+1]);
			}
			glEnd();
		}
		else if (last_msg_.marker_type == 1)
		{
			// 2d line segments
			int i = 0;
			while (i < last_msg_.data_length)
			{
				int count = last_msg_.data[i];
				int end_index = i + count*2;
				i++;
				// draw a line segment
				glBegin(GL_LINE_STRIP);
				glColor3f(color.r/255.0, color.g/255.0, color.b/255.0);
				for (; i < std::min<int>(end_index, last_msg_.data_length-1); i += 2)
				{
					glVertex2f(last_msg_.data[i], last_msg_.data[i+1]);
				}
				glEnd();
			}
		}
		else if (last_msg_.marker_type == 2)
		{
			// 2d polygons (just draw outline atm)
			int i = 0;
			while (i < last_msg_.data_length)
			{
				int count = last_msg_.data[i];
				int start_index = i;
				int end_index = i + count*2;
				i++;
				// draw a line segment
				glBegin(GL_LINE_STRIP);
				glColor3f(color.r/255.0, color.g/255.0, color.b/255.0);
				for (; i < std::min<int>(end_index, last_msg_.data_length-1); i += 2)
				{
					glVertex2f(last_msg_.data[i], last_msg_.data[i+1]);
				}
				glVertex2f(last_msg_.data[start_index], last_msg_.data[start_index+1]);
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
		alpha_ = new FloatProperty(tree, "Alpha", 1.0, 0.0, 1.0, 0.1);
		
		color_ = new ColorProperty(tree, "Color", Gwen::Color(255,50,50));
		
		topic_ = new TopicProperty(tree, "Topic", "/marker");
		topic_->onChange = std::bind(&MarkerPlugin::Subscribe, this, std::placeholders::_1);
		
		line_width_ = new NumberProperty(tree, "Line Width", 4, 1, 100, 2);
		
		Subscribe(topic_->GetValue());
	}
	
	std::string GetTitle() override
	{
		return "Marker";
	}
};

#endif
