
#ifndef PUBVIZ_PLUGIN_PLAN_PATH_H
#define PUBVIZ_PLUGIN_PLAN_PATH_H

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

#include <pubsub/Path.msg.h>

class PlanPathPlugin : public pubviz::Plugin
{
	std::unique_ptr<FloatProperty> alpha_;
	std::unique_ptr<ColorProperty> color_;
	std::unique_ptr<NumberProperty> line_width_;
	std::unique_ptr<NumberProperty> point_size_;

	std::unique_ptr<EnumProperty> frame_;

	std::unique_ptr<TopicProperty> topic_;

	std::unique_ptr<ButtonProperty> publish_;
	std::unique_ptr<ButtonProperty>	clear_;

	std::unique_ptr<pubsub::Publisher<pubsub::msg::Path>> publisher_;

	std::string current_topic_;
	void ChangeTopic(std::string str)
	{
	  publisher_.reset();

		current_topic_ = str;
		publisher_.reset(GetNode()->advertise<pubsub::msg::Path>(current_topic_, true));
	}

	void FrameChanged(std::string str)
	{
		
	}

public:

	std::string frame_str_ = "odom";
	PlanPathPlugin()
	{
		// dont use pubsub here

	}

	virtual ~PlanPathPlugin()
	{

	}

	// Clear out any historical data so the view gets cleared
	virtual void Clear()
	{
		points_.clear();
	}

	virtual void Update()
	{

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
		if (points_.size() == 0)
		{
			return;
		}

		Gwen::Color color = color_->GetValue();

		// Now transform the points
		transformed_pts_.clear();
		transformed_pts_.reserve(points_.size());
		for (auto& pt : points_)
		{
			Vec3d pos(pt.x, pt.y, 0);
			GetCanvas()->TransformToView(frame_str_.c_str(), pos);
			transformed_pts_.push_back({pos.x, pos.y, pos.z});
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

		if (true)
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

		topic_ = AddTopicProperty(tree, "Topic", "/path", "", "pubsub__Path", true, false);
		topic_->onChange = std::bind(&PlanPathPlugin::ChangeTopic, this, std::placeholders::_1);

		line_width_ = AddNumberProperty(tree, "Line Width", 4, 1, 100, 2);

		frame_ = AddEnumProperty(tree, "Frame", "Odom", {"Odom", "WGS84"}, "Frame to publish path in.");
		frame_->onChange = [this](std::string frame) 
		{
			//clear the path and set the frame
			points_.clear();
			frame_str_ = (frame == "Odom" ? "odom" : "wgs84");
		};

		point_size_ = AddNumberProperty(tree, "Point Size", 6, 1, 100, 2);

		publish_ = AddButtonProperty(tree, "Publish");
		publish_->onChange = [this]()
		{
			Publish();
		};
		clear_ = AddButtonProperty(tree, "Clear");
		clear_->onChange = [this]()
		{
			points_.clear();
			Redraw();
		};

		ChangeTopic(topic_->GetValue());
	}

	void Publish()
	{
		std::vector<double> points;
		for (const auto& pt: points_)
		{
			points.push_back(pt.x);
			points.push_back(pt.y);
			points.push_back(pt.z);
		}
		pubsub::msg::Path msg;
		msg.header.frame = frame_str_;
		msg.path_type = pubsub::msg::Path::PATH_XY_Y;
		msg.points = points;
		publisher_->publish(msg);
	}

	std::string GetTitle() override
	{
		return "Plan Path";
	}

	virtual std::vector<std::pair<std::string, std::function<void()>>> ContextMenu(double x, double y) override
	{
		return {{"Add Path Point", [this, x, y]() { OnMapDoubleClick(x, y); }}, 
                {"Clear Path", [this]() {points_.clear(); Redraw();}},
				{"Publish Path", [this]() { Publish(); }}};
	}

	// Applies only for 2d
	virtual bool OnMapDoubleClick(double x, double y) override
	{
		//printf("map_click %f %f\n", x, y);
		Vec3d pos(x, y, 0);
		auto src = GetCanvas()->wgs84_mode() ? OpenGLCanvas::Map : OpenGLCanvas::Odom;
		auto dst = frame_str_ == "wgs84" ? OpenGLCanvas::WGS84 : OpenGLCanvas::Odom;
		GetCanvas()->TransformToFrame(src, dst, pos);
		// transform to correct frame
		points_.push_back(Vertex(pos.x, pos.y, std::numeric_limits<double>::quiet_NaN()));

		Redraw();
		return true;
	}
};

REGISTER_PLUGIN("plan_path", PlanPathPlugin)

#endif
