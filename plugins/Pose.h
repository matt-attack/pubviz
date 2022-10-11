
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
	NumberProperty* history_length_;
	FloatProperty* sample_distance_;
	
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
			GetCanvas()->ResetViewOrigin();
		}
	}

	void OnHistoryChange(int length)
	{
		if (messages_.size() > length)
		{
			messages_.resize(length);
		}
	}

	void OnSampleDistanceChange(double d)
	{
		while (messages_.size() > 1)
		{
			messages_.pop_front();
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
			double sample_dist = sample_distance_->GetValue();
			auto sample_dist_sqr = sample_dist*sample_dist;
			while (data = (pubsub::msg::Pose*)ps_sub_deque(&subscriber_))
			{
				if (Paused())
				{
				    free(data);//todo use allocator free
					continue;
				}

                // if sample distance
                if (sample_dist > 0.0)
                {
                    // always have at least two samples
                    if (messages_.size() < 2)
                    {
                        messages_.push_back(*data);
                    }
                    else
                    {
                        // replace the newest sample 
                        auto& other = messages_[messages_.size()-2];
                        double dx = (data->x-other.x); double dy = (data->y-other.y); double dz = (data->z-other.z);
                        double current_distance = dx*dx + dy*dy + dz*dz;
                        if (current_distance < sample_dist_sqr)
                        {
                            messages_.pop_back();
                        }
                        messages_.push_back(*data);
                        // if new sample is > sample distance from last placed sample, insert a new one
                    }
                    
                    // if the first s
                }
                else
                {
				    // always add
				    messages_.push_back(*data);
                }

                while (messages_.size() > history_length_->GetValue())
                {
                    messages_.pop_front();
                }

				// todo is there a better way to do this?
				GetCanvas()->SetLocalXY(data->latitude, data->longitude);

				if (follow_pose_->GetValue())
				{
					// center view on me
					GetCanvas()->SetViewOrigin(data->x, data->y, data->z, data->latitude, data->longitude, 0.0);
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
		for (auto p: messages_)
		{
			// start with just rendering axes
			// 
			// todo handle rotation

			if (GetCanvas()->wgs84_mode_)
			{
				GetCanvas()->local_xy_.FromLatLon(p.latitude, p.longitude, p.x, p.y);
				p.z = 0.0;
				p.odom_yaw = p.yaw;
			}

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
		alpha_ = AddFloatProperty(tree, "Alpha", 1.0, 0.0, 1.0, 0.1, "Pose visualization transparency.");
		
		color_ = AddColorProperty(tree, "Color", Gwen::Color(255,50,50));
		
		topic_ = AddTopicProperty(tree, "Topic", "/pose");
		topic_->onChange = std::bind(&PosePlugin::Subscribe, this, std::placeholders::_1);
		
		line_width_ = AddNumberProperty(tree, "Line Width", 4, 1, 100, 2, "Width in pixels of lines.");

		line_length_ = AddFloatProperty(tree, "Line Length", 5, 0.1, 100, 2, "Length in world units of lines.");

		follow_pose_ = AddBooleanProperty(tree, "Follow", false, "If true, center the view on this pose.");
		follow_pose_->onChange = std::bind(&PosePlugin::OnFollowChange, this, std::placeholders::_1);

		history_length_ = AddNumberProperty(tree, "History Length", 1, 1, 1000000, 2, "");
		history_length_->onChange = std::bind(&PosePlugin::OnHistoryChange, this, std::placeholders::_1);
		sample_distance_ = AddFloatProperty(tree, "Sample Distance", 10.0, 0.0, 100, 1, "");
		sample_distance_->onChange = std::bind(&PosePlugin::OnSampleDistanceChange, this, std::placeholders::_1);
		
		Subscribe(topic_->GetValue());
	}
	
	std::string GetTitle() override
	{
		return "Marker";
	}
};

#endif
