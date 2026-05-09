
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
	std::unique_ptr<FloatProperty> alpha_;
	std::unique_ptr<ColorProperty> color_;
	std::unique_ptr<NumberProperty> line_width_;
	std::unique_ptr<BooleanProperty> show_points_;
	std::unique_ptr<NumberProperty> point_size_;

	std::unique_ptr<TopicProperty> topic_;

	pubsub::Subscriber<pubsub::msg::Path>::Ptr subscriber_;

	pubsub::msg::PathSharedConstPtr last_msg_;

	std::string current_topic_;
	void Subscribe(std::string str)
	{
		subscriber_.reset();

		Clear();

		current_topic_ = str;
		subscriber_.reset(new pubsub::Subscriber<pubsub::msg::Path>(*GetNode(), current_topic_, [](const pubsub::msg::PathSharedPtr& msg) {}, 1, 1));
	}

public:

	PathPlugin()
	{
		// dont use pubsub here
	}

	virtual ~PathPlugin()
	{

	}

	// Clear out any historical data so the view gets cleared
	virtual void Clear()
	{
		last_msg_.reset();
	}

	virtual void Update()
	{
	  if (!subscriber_)
	  {
	    return;
	  }
		// process any messages
		while (auto data = subscriber_->PopOne())
		{
			if (Paused())
			{
				continue;
			}

			last_msg_ = data;

			Redraw();
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
		if (last_msg_ == 0)
		{
			return;
		}

		Gwen::Color color = color_->GetValue();

		points_.clear();
		if (last_msg_->path_type == pubsub::msg::Path::PATH_XY)
		{
			for (int i = 0; i < (int)last_msg_->points.size() - 1; i += 2)
			{
				points_.push_back({ last_msg_->points[i], last_msg_->points[i + 1], 0.0 });
			}
		}
		else if (last_msg_->path_type == pubsub::msg::Path::PATH_XY_Y)
		{
			for (int i = 0; i < (int)last_msg_->points.size() - 2; i += 3)
			{
				points_.push_back({ last_msg_->points[i], last_msg_->points[i + 1], 0.0 });
			}
		}
		else if (last_msg_->path_type == pubsub::msg::Path::PATH_XYZ)
		{
			for (int i = 0; i < (int)last_msg_->points.size() - 2; i += 3)
			{
				points_.push_back({ last_msg_->points[i], last_msg_->points[i + 1], last_msg_->points[i + 2] });
			}
		}
		else if (last_msg_->path_type == pubsub::msg::Path::PATH_XYZ_Y)
		{
			for (int i = 0; i < (int)last_msg_->points.size() - 3; i += 4)
			{
				points_.push_back({ last_msg_->points[i], last_msg_->points[i + 1], last_msg_->points[i + 2] });
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
			Vec3d p(pt.x, pt.y, pt.z);
			GetCanvas()->TransformToView(last_msg_->header.frame, p);
			transformed_pts_.push_back({p.x,p.y,p.z});
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
