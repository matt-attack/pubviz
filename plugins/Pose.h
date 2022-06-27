
#ifndef PUBVIZ_PLUGIN_POSE_H
#define PUBVIZ_PLUGIN_POSE_H

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

#include <deque>

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

#include <pubsub/Pose.msg.h>

#include "../OpenGLCanvas.h"

class PosePlugin: public Plugin
{
	FloatProperty* alpha_;
	ColorProperty* color_;
	NumberProperty* line_width_;
	FloatProperty* line_length_;
	BooleanProperty* follow_pose_;
	
	TopicProperty* topic_;
	
	bool sub_open_ = false;
	ps_sub_t subscriber_;
	
	std::deque<pubsub::msg::Pose> messages_;
	
	void UpdateFromMessage()
	{

	}

	void OnFollowChange(bool state)
	{
		if (!state)
		{
			GetCanvas()->SetViewOrigin(0.0, 0.0, 0.0);
		}
	}
	
	std::string current_topic_;
	void Subscribe(std::string str)
	{
		if (sub_open_)
		{
			ps_sub_destroy(&subscriber_);
		}
		
		messages_.clear();
		
		current_topic_ = str;
    	struct ps_subscriber_options options;
    	ps_subscriber_options_init(&options);
    	//options.preferred_transport = 1;// tcp yo
    	ps_node_create_subscriber_adv(GetNode(), current_topic_.c_str(), &pubsub__Pose_def, &subscriber_, &options);
    	sub_open_ = true;
	}
	
public:

	PosePlugin()
	{	
		// dont use pubsub here
	}
	
	virtual ~PosePlugin()
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
	
	virtual void Update()
	{
		// process any messages
		// our sub has a message definition, so the queue contains real messages
		pubsub::msg::Pose* data;
		if (sub_open_)
		{
			while (data = (pubsub::msg::Pose*)ps_sub_deque(&subscriber_))
			{
				// user is responsible for freeing the message and its arrays
				messages_.clear();
				messages_.push_back(*data);

				if (follow_pose_->GetValue())
				{
					// center view on me
					GetCanvas()->SetViewOrigin(data->x, data->y, data->z);
				}

				free(data);//todo use allocator free
				
				Redraw();
			}
		}
	}
		
	virtual void Render()
	{
		const double line_length = line_length_->GetValue();
		glLineWidth(line_width_->GetValue());
		for (auto& p: messages_)
		{
			// start with just rendering axes
			// 
			// todo handle rotation

			// first handle yaw
			float x_x = 1.0 * cos(p.odom_yaw) - 0.0 * sin(p.odom_yaw);
			float x_y = 1.0 * sin(p.odom_yaw) + 0.0 * cos(p.odom_yaw);
			float x_z = 0.0;

			float y_x = 0.0 * cos(p.odom_yaw) - 1.0 * sin(p.odom_yaw);
			float y_y = 0.0 * sin(p.odom_yaw) + 1.0 * cos(p.odom_yaw);
			float y_z = 0.0;

			float z_x = 0.0;
			float z_y = 0.0;
			float z_z = 1.0;

			// todo handle pitch and roll later

			// Draw axes
			glBegin(GL_LINES);
			// Red line to the right (x)
			glColor3f(1, 0, 0);
			glVertex3f(p.x + 0, p.y + 0, p.z + 0);
			glVertex3f(p.x + x_x * line_length, p.y + x_y * line_length, p.z + x_z * line_length);

			// Green line to the top (y)
			glColor3f(0, 1, 0);
			glVertex3f(p.x + 0, p.y + 0, p.z + 0);
			glVertex3f(p.x + y_x * line_length, p.y + y_y * line_length, p.z + y_z * line_length);

			// Blue line up (z)
			glColor3f(0, 0, 1);
			glVertex3f(p.x + 0, p.y + 0, p.z + 0);
			glVertex3f(p.x + z_x * line_length, p.y + z_y * line_length, p.z + z_z * line_length);
			glEnd();
		}
	}

	virtual void Initialize(Gwen::Controls::Properties* tree)
	{
		// add any properties
		alpha_ = AddFloatProperty(tree, "Alpha", 1.0, 0.0, 1.0, 0.1);
		
		color_ = AddColorProperty(tree, "Color", Gwen::Color(255,50,50));
		
		topic_ = AddTopicProperty(tree, "Topic", "/pose");
		topic_->onChange = std::bind(&PosePlugin::Subscribe, this, std::placeholders::_1);
		
		line_width_ = AddNumberProperty(tree, "Line Width", 4, 1, 100, 2);

		line_length_ = AddFloatProperty(tree, "Line Length", 5, 0.1, 100, 2);

		follow_pose_ = AddBooleanProperty(tree, "Follow", false);
		follow_pose_->onChange = std::bind(&PosePlugin::OnFollowChange, this, std::placeholders::_1);
		
		Subscribe(topic_->GetValue());
	}
	
	std::string GetTitle() override
	{
		return "Marker";
	}
};

#endif
