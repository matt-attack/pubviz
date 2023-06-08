
#ifndef PUBVIZ_PLUGIN_GPS_H
#define PUBVIZ_PLUGIN_GPS_H

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

#include <pubsub/GPS.msg.h>

#include "../controls/OpenGLCanvas.h"

class GPSPlugin: public Plugin
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
	
	std::deque<pubsub::msg::GPS> messages_;
	
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
    	ps_node_create_subscriber_adv(GetNode(), current_topic_.c_str(), &pubsub__GPS_def, &subscriber_, &options);
    	sub_open_ = true;
	}
	
public:

	GPSPlugin()
	{	
		// dont use pubsub here
	}
	
	virtual ~GPSPlugin()
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
		pubsub::msg::GPS* data;
		if (sub_open_)
		{
			double sample_dist = sample_distance_->GetValue();
			auto sample_dist_sqr = sample_dist*sample_dist;
			while (data = (pubsub::msg::GPS*)ps_sub_deque(&subscriber_))
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
                        /*auto& other = messages_[messages_.size()-2];
                        double dx = (data->x-other.x); double dy = (data->y-other.y); double dz = (data->z-other.z);
                        double current_distance = dx*dx + dy*dy + dz*dz;
                        if (current_distance < sample_dist_sqr)
                        {
                            messages_.pop_back();
                        }
                        messages_.push_back(*data);*/
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

				//if (follow_pose_->GetValue())
				{
					// center view on me
					//GetCanvas()->SetViewOrigin(data->x, data->y, data->z, data->latitude, data->longitude, 0.0);
				}

				free(data);//todo use allocator free
				
				Redraw();
			}
		}
	}
		
	virtual void Render()
	{
		if (messages_.size() == 0)
		{
			return;
		}

		// Draw heading lines
		const double line_length = line_length_->GetValue();
		glLineWidth(line_width_->GetValue());
		glBegin(GL_LINES);
		for (auto p: messages_)
		{
			double x, y;

			if (GetCanvas()->wgs84_mode_)
			{
				GetCanvas()->local_xy_.FromLatLon(p.latitude, p.longitude, x, y);
			}
			else
			{
				continue;
			}

			// draw horizontal accuracy as a circle
			const int num_pts = 10;
			for (int i = 0; i <= num_pts; i++)
			{
				float angle1 = i*2.0*M_PI/num_pts;
				float angle2 = (i+1)*2.0*M_PI/num_pts;
				glColor3f(0, 1, 0);
				glVertex3f(x + p.horizontal_accuracy*cos(angle1), y + p.horizontal_accuracy*sin(angle1), 0);
				glColor3f(0, 1, 0);
				glVertex3f(x + p.horizontal_accuracy*cos(angle2), y + p.horizontal_accuracy*sin(angle2), 0);
			}

			// ignore invalid headings
			if (p.heading < 0)
			{
				continue;
			}

			double yaw = 90 - p.heading;
			double arrow_x = x + line_length*cos(yaw*M_PI/180.0);
			double arrow_y = y + line_length*sin(yaw*M_PI/180.0);

			// draw yaw arrow if its valid
			glColor3f(1, 0, 0);
			glVertex3f(x, y, 0);
			glVertex3f(arrow_x, arrow_y, 0);

			double arrow2_x = arrow_x + line_length*cos((yaw - 135)*M_PI/180.0);
			double arrow2_y = arrow_y + line_length*sin((yaw - 135)*M_PI/180.0);

			glVertex3f(arrow_x, arrow_y, 0);
			glVertex3f(arrow2_x, arrow2_y, 0);

			double arrow3_x = arrow_x + line_length*cos((yaw + 135)*M_PI/180.0);
			double arrow3_y = arrow_y + line_length*sin((yaw + 135)*M_PI/180.0);

			glVertex3f(arrow_x, arrow_y, 0);
			glVertex3f(arrow3_x, arrow3_y, 0);
		}
		glEnd();

		// Draw points
		glPointSize(10);
		glBegin(GL_POINTS);
		for (auto p: messages_)
		{
			double x, y;

			if (GetCanvas()->wgs84_mode_)
			{
				GetCanvas()->local_xy_.FromLatLon(p.latitude, p.longitude, x, y);
				//p.z = 0.0;
				//p.odom_yaw = p.yaw;
			}
			else
			{
				// todo?
				continue;
			}

			glColor3f(1, 0, 0);
			glVertex3f(x, y, 0);
		}
		glEnd();
	}

	virtual void Initialize(Gwen::Controls::Properties* tree)
	{
		// add any properties
		alpha_ = AddFloatProperty(tree, "Alpha", 1.0, 0.0, 1.0, 0.1, "Pose visualization transparency.");
		
		color_ = AddColorProperty(tree, "Color", Gwen::Color(255,50,50));
		
		topic_ = AddTopicProperty(tree, "Topic", "/gps");
		topic_->onChange = std::bind(&GPSPlugin::Subscribe, this, std::placeholders::_1);
		
		line_width_ = AddNumberProperty(tree, "Line Width", 4, 1, 100, 2, "Width in pixels of lines.");

		line_length_ = AddFloatProperty(tree, "Line Length", 5, 0.1, 100, 2, "Length in world units of lines.");

		//follow_pose_ = AddBooleanProperty(tree, "Follow", false, "If true, center the view on this pose.");
		//follow_pose_->onChange = std::bind(&GPSPlugin::OnFollowChange, this, std::placeholders::_1);

		history_length_ = AddNumberProperty(tree, "History Length", 1, 1, 1000000, 2, "");
		history_length_->onChange = std::bind(&GPSPlugin::OnHistoryChange, this, std::placeholders::_1);
		sample_distance_ = AddFloatProperty(tree, "Sample Distance", 10.0, 0.0, 100, 1, "");
		sample_distance_->onChange = std::bind(&GPSPlugin::OnSampleDistanceChange, this, std::placeholders::_1);
		
		Subscribe(topic_->GetValue());
	}
	
	std::string GetTitle() override
	{
		return "GPS";
	}
};

REGISTER_PLUGIN("gps", GPSPlugin)

#endif
