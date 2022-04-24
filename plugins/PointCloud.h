
#ifndef PUBVIZ_PLUGIN_POINTCLOUD_H
#define PUBVIZ_PLUGIN_POINTCLOUD_H

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

#include <pubsub/PointCloud.msg.h>

class PointCloudPlugin: public Plugin
{
	FloatProperty* alpha_;
	ColorProperty* min_color_;
	ColorProperty* max_color_;
	NumberProperty* point_size_;
	
	TopicProperty* topic_;
	
	bool sub_open_ = false;
	ps_sub_t subscriber_;
	
	pubsub::msg::Marker last_msg_;
	
	struct Cloud
	{
		int num_points;
		unsigned int point_vbo;
		unsigned int color_vbo;
	};
	
	std::vector<Cloud> clouds_;
	
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
		
		for (auto cloud: clouds_)
		{
			// delete the buffers
			glDeleteBuffers(1, &cloud.point_vbo);
			glDeleteBuffers(1, &cloud.color_vbo);
		}
		clouds_.clear();
		
		current_topic_ = str;
    	struct ps_subscriber_options options;
    	ps_subscriber_options_init(&options);
    	options.preferred_transport = 1;// tcp yo
    	ps_node_create_subscriber_adv(GetNode(), current_topic_.c_str(), &pubsub__PointCloud_def, &subscriber_, &options);
    	sub_open_ = true;
	}
	
public:

	PointCloudPlugin()
	{	
		// dont use pubsub here
		glewInit();
	}
	
	virtual ~PointCloudPlugin()
	{
		delete min_color_;
		delete max_color_;
		delete alpha_;
		delete point_size_;
		
		if (sub_open_)
		{
			ps_sub_destroy(&subscriber_);
			sub_open_ = false;
		}
	}
	
	struct Point3d
	{
		float x;
		float y;
		float z;
	};
	std::vector<Point3d> point_buf_;
	std::vector<int> color_buf_;
	
	virtual void Render()
	{
		// process any messages
		// our sub has a message definition, so the queue contains real messages
		pubsub::msg::PointCloud* data;
		if (sub_open_)
		{
			while (data = (pubsub::msg::PointCloud*)ps_sub_deque(&subscriber_))
			{
				// user is responsible for freeing the message and its arrays
				
				// make a new cloud with this, reusing the last ones buffers if necessary
				Cloud* cloud = 0;
				if (clouds_.size() > 0)
				{
					cloud = &clouds_[0];
				}
				else
				{
					Cloud c;
					glGenBuffers(1, &c.point_vbo);
					glGenBuffers(1, &c.color_vbo);
					clouds_.push_back(c);
					cloud = &clouds_[clouds_.size()-1];
				}
				
				cloud->num_points = data->num_points;
				
				point_buf_.resize(data->num_points);
				color_buf_.resize(data->num_points);
				
				uint8_t alpha = 255.0*alpha_->GetValue();
				Gwen::Color min_color = min_color_->GetValue();
				Gwen::Color max_color = max_color_->GetValue();
				
				// now make the right kind of points then render
				if (data->point_type == 0)
				{
					// x,y,z as floats
					for (int i = 0; i < data->num_points; i++)
					{
						point_buf_[i].x = ((float*)data->data)[i*3];
						point_buf_[i].y = ((float*)data->data)[i*3+1];
						point_buf_[i].z = ((float*)data->data)[i*3+2];
						color_buf_[i] = (alpha << 24) | (max_color.b << 16) | (max_color.g << 8) | max_color.r;
					}
				}
				else if (data->point_type == 1)
				{
					// x,y,z,intensity as floats
					for (int i = 0; i < data->num_points; i++)
					{
						point_buf_[i].x = ((float*)data->data)[i*4];
						point_buf_[i].y = ((float*)data->data)[i*4+1];
						point_buf_[i].z = ((float*)data->data)[i*4+2];
						float frac = ((float*)data->data)[i*4+3];
						uint8_t r = frac*(max_color.r - min_color.r) + min_color.r;
						uint8_t g = frac*(max_color.g - min_color.g) + min_color.g;
						uint8_t b = frac*(max_color.b - min_color.b) + min_color.b;
																		
						color_buf_[i] = (alpha << 24) | r | (g << 8) | (b << 16);
					}
				}
				
				// upload!
				glBindBuffer(GL_ARRAY_BUFFER, cloud->point_vbo);
				glBufferData(GL_ARRAY_BUFFER, point_buf_.size()*12, point_buf_.data(), GL_STATIC_DRAW);
				
				glBindBuffer(GL_ARRAY_BUFFER, cloud->color_vbo);
				glBufferData(GL_ARRAY_BUFFER, color_buf_.size()*4, color_buf_.data(), GL_STATIC_DRAW);
				
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				
				free(data->data);
				free(data);//todo use allocator free
			}
		}
		
		glPointSize(point_size_->GetValue());
		
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		
		for (auto& cloud: clouds_)
		{
			// render it
			glBindBuffer(GL_ARRAY_BUFFER, cloud.point_vbo);
			glVertexPointer(3, GL_FLOAT, 0, 0);
			glBindBuffer(GL_ARRAY_BUFFER, cloud.color_vbo);
			glColorPointer(4, GL_UNSIGNED_BYTE, 0, 0);
			
			glDrawArrays(GL_POINTS, 0, cloud.num_points);
		}
		
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	
	virtual void Initialize(Gwen::Controls::Properties* tree)
	{
		// add any properties
		alpha_ = new FloatProperty(tree, "Alpha", 1.0, 0.0, 1.0, 0.1);
		
		min_color_ = new ColorProperty(tree, "Max Color", Gwen::Color(255,255,255));
		
		max_color_ = new ColorProperty(tree, "Min Color", Gwen::Color(255,0,0));
		
		topic_ = new TopicProperty(tree, "Topic", "/pointcloud");
		topic_->onChange = std::bind(&PointCloudPlugin::Subscribe, this, std::placeholders::_1);
		
		point_size_ = new NumberProperty(tree, "Point Size", 4, 1, 100, 2);
		
		Subscribe(topic_->GetValue());
	}
	
	std::string GetTitle() override
	{
		return "Point Cloud";
	}
};

#endif
