
#ifndef PUBVIZ_PLUGIN_PATH_H
#define PUBVIZ_PLUGIN_PATH_H

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

#include <pubsub/Path.msg.h>

class PathPlugin : public pubviz::Plugin
{
	FloatProperty* alpha_;
	ColorProperty* color_;
	NumberProperty* line_width_;
	BooleanProperty* show_points_;
	NumberProperty* point_size_;

	TopicProperty* topic_;

	bool sub_open_ = false;
	ps_sub_t subscriber_;

	pubsub::msg::Path last_msg_;

	std::string current_topic_;
	void Subscribe(std::string str)
	{
		if (sub_open_)
		{
			ps_sub_destroy(&subscriber_);
		}

		Clear();

		current_topic_ = str;
		struct ps_subscriber_options options;
		ps_subscriber_options_init(&options);
		options.preferred_transport = 1;// tcp yo
		ps_node_create_subscriber_adv(GetNode(), current_topic_.c_str(), &pubsub__Path_def, &subscriber_, &options);
		sub_open_ = true;
	}

public:

	PathPlugin()
	{
		// dont use pubsub here
	}

	virtual ~PathPlugin()
	{
		delete color_;
		delete alpha_;
		delete line_width_;
		delete show_points_;
		delete point_size_;

		if (sub_open_)
		{
			ps_sub_destroy(&subscriber_);
			sub_open_ = false;
		}
	}

	// Clear out any historical data so the view gets cleared
	virtual void Clear()
	{
		last_msg_.points_length = 0;
	}

	virtual void Update()
	{
		// process any messages
		// our sub has a message definition, so the queue contains real messages
		pubsub::msg::Path* data;
		if (sub_open_)
		{
			while (data = (pubsub::msg::Path*)ps_sub_deque(&subscriber_))
			{
				if (Paused())
				{
					free(data->points);
					free(data);//todo use allocator free
					continue;
				}

				// user is responsible for freeing the message and its arrays
				last_msg_ = *data;
				free(data->points);
				free(data);//todo use allocator free

				Redraw();
			}
		}
	}

	struct Vertex
	{
		double x;
		double y;
		double z;

		Vertex(double x, double y, double z)
		{
			this->x = x;
			this->y = y;
			this->z = z;
		}
	};

	std::vector<Vertex> points_;
	std::vector<Vertex> transformed_pts_;
	virtual void Render()
	{
		if (last_msg_.points_length == 0)
		{
			return;
		}

		Gwen::Color color = color_->GetValue();

		points_.clear();
		if (last_msg_.path_type == pubsub::msg::Path::PATH_XY)
		{
			for (int i = 0; i < (int)last_msg_.points_length - 1; i += 2)
			{
				points_.push_back({ last_msg_.points[i], last_msg_.points[i + 1], 0.0 });
			}
		}
		if (last_msg_.path_type == pubsub::msg::Path::PATH_XY_Y)
		{
			for (int i = 0; i < (int)last_msg_.points_length - 2; i += 3)
			{
				points_.push_back({ last_msg_.points[i], last_msg_.points[i + 1], 0.0 });
			}
		}
		else if (last_msg_.path_type == pubsub::msg::Path::PATH_XYZ)
		{
			for (int i = 0; i < (int)last_msg_.points_length - 2; i += 3)
			{
				points_.push_back({ last_msg_.points[i], last_msg_.points[i + 1], last_msg_.points[i + 2] });
			}
		}
		else if (last_msg_.path_type == pubsub::msg::Path::PATH_XYZ_Y)
		{
			for (int i = 0; i < (int)last_msg_.points_length - 3; i += 4)
			{
				points_.push_back({ last_msg_.points[i], last_msg_.points[i + 1], last_msg_.points[i + 2] });
			}
		}
		else
		{
			printf("ERROR: Unknown path type\n");
		}

		// Now transform the points
		transformed_pts_.clear();
		transformed_pts_.reserve(points_.size());
		for (auto& pt : points_)
		{
			if (last_msg_.frame == pubsub::msg::Path::FRAME_WGS84)
			{
				if (GetCanvas()->wgs84_mode_)
				{
					double x, y;
					GetCanvas()->local_xy_.FromLatLon(pt.x, pt.y, x, y);

					transformed_pts_.push_back({ x, y, pt.z });
				}
				else
				{
					// todo
				}
			}
			else
			{
				if (GetCanvas()->wgs84_mode_)
				{
					// todo
				}
				else
				{
					transformed_pts_.push_back(pt);
				}
			}
		}

		// Now render the points
		glLineWidth(line_width_->GetValue());
		glBegin(GL_LINE_STRIP);
		glColor3f(color.r / 255.0, color.g / 255.0, color.b / 255.0);
		for (const auto& vert : transformed_pts_)
		{
			glVertex3f(vert.x, vert.y, vert.z);
		}
		glEnd();

		if (show_points_->GetValue())
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

		topic_ = AddTopicProperty(tree, "Topic", "/path", "", "pubsub__Path");
		topic_->onChange = std::bind(&PathPlugin::Subscribe, this, std::placeholders::_1);

		line_width_ = AddNumberProperty(tree, "Line Width", 4, 1, 100, 2);

		show_points_ = AddBooleanProperty(tree, "Show Points", true);

		point_size_ = AddNumberProperty(tree, "Point Size", 6, 1, 100, 2);

		Subscribe(topic_->GetValue());
	}

	std::string GetTitle() override
	{
		return "Path";
	}
};

REGISTER_PLUGIN("path", PathPlugin)

#endif
