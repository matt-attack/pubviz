
#ifndef PUBVIZ_PLUGIN_COSTMAP_H
#define PUBVIZ_PLUGIN_COSTMAP_H

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

#include <pubsub/Costmap.msg.h>

class CostmapPlugin: public pubviz::Plugin
{
	FloatProperty* alpha_;
	ColorProperty* color_;
	BooleanProperty* show_outline_;
	NumberProperty* alpha_threshold_;
	
	TopicProperty* topic_;
	
	bool sub_open_ = false;
	ps_sub_t subscriber_;
	
	pubsub::msg::Costmap* last_msg_ = 0;
	
	unsigned int texture_ = -1;
	
	void UpdateFromMessage()
	{
		if (last_msg_ == 0) return;
		Redraw();
		
		if (texture_ != -1)
		{
			glDeleteTextures(1, &texture_);
		}
		
		if (last_msg_->width * last_msg_->height != last_msg_->data.size())
		{
			delete last_msg_;
			last_msg_ = 0;
			printf("ERROR: bad costmap size\n");
			return;
		}
		
		// make the color buffer
		std::vector<uint32_t> pixels;
		pixels.resize(last_msg_->width*last_msg_->height);
		
		int texture_size = pixels.size()*4;

		int32_t alpha_threshold = alpha_threshold_->GetValue();
		
		// now fill in each pixel with the intensity
		for (int i = 0; i < last_msg_->data.size(); i++)
		{
			uint8_t px = last_msg_->data[i];
			uint8_t a = px <= alpha_threshold ? 0 : 255;
			pixels[i] = (a << 24) | (px << 16) | (px << 8) | px;
		}
		
		glGenTextures(1, &texture_);
		
		glBindTexture(GL_TEXTURE_2D, texture_);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

		glTexImage2D(
		  GL_TEXTURE_2D,
		  0,
		  GL_RGBA,
		  last_msg_->width,
		  last_msg_->height,
		  0,
		  GL_RGBA,
		  GL_UNSIGNED_BYTE,
		  pixels.data());

		glBindTexture(GL_TEXTURE_2D, 0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}
	
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
    	ps_node_create_subscriber_adv(GetNode(), current_topic_.c_str(), &pubsub__Costmap_def, &subscriber_, &options);
    	sub_open_ = true;
	}
	
public:

	CostmapPlugin()
	{	
		// dont use pubsub here
	}
	
	virtual ~CostmapPlugin()
	{
		delete color_;
		delete alpha_;
		delete show_outline_;

		if (last_msg_)
		{
			delete last_msg_;
		}
		
		if (sub_open_)
		{
			ps_sub_destroy(&subscriber_);
			sub_open_ = false;
		}
		
		if (texture_ != -1)
		{
			glDeleteTextures(1, &texture_);
		}
	}

	// Clear out any historical data so the view gets cleared
	virtual void Clear()
	{
		if (texture_ != -1)
		{
			glDeleteTextures(1, &texture_);
			delete last_msg_;
			last_msg_ = 0;
			texture_ = -1;
		}
	}
	
	virtual void Update()
	{
		// process any messages
		// our sub has a message definition, so the queue contains real messages
		pubsub::msg::Costmap* data;
		if (sub_open_)
		{
			while (data = (pubsub::msg::Costmap*)ps_sub_deque(&subscriber_))
			{
				if (Paused())
				{
					delete data;
					continue;
				}

				// user is responsible for freeing the message and its arrays
				if (last_msg_)
				{
					delete last_msg_;
				}
				last_msg_ = data;
				UpdateFromMessage();
			}
		}
	}
	
	virtual void Render()
	{		
		// exit early if we dont have a messag
		if (last_msg_ == 0)
		{
			return;
		}
		
		double width = last_msg_->resolution*last_msg_->width;
		double height = last_msg_->resolution*last_msg_->height;

		Vec3d pts[4];
		pts[0] = Vec3d(last_msg_->left, last_msg_->bottom, 0);
		pts[1] = Vec3d(last_msg_->left, last_msg_->bottom + height, 0);
		pts[2] = Vec3d(last_msg_->left + width, last_msg_->bottom + height, 0);
		pts[3] = Vec3d(last_msg_->left + width, last_msg_->bottom, 0);

		// transform to view frame
		for (int i = 0; i < 4; i++)
		{
			GetCanvas()->TransformToView(OpenGLCanvas::Odom, pts[i]);
		}
		
		// draw the bounds of the costmap
		if (show_outline_->GetValue())
		{
			glLineWidth(4.0f);
			glBegin(GL_LINE_STRIP);
			
			Gwen::Color color = color_->GetValue();
			glColor3f(color.r/255.0, color.g/255.0, color.b/255.0);
			
			glVertex2f(pts[0].x, pts[0].y);
			glVertex2f(pts[1].x, pts[1].y);
			glVertex2f(pts[2].x, pts[2].y);
			glVertex2f(pts[3].x, pts[3].y);
			glVertex2f(pts[0].x, pts[0].y);
			
			glEnd();
		}

		glEnable(GL_BLEND);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0);
		
		// Now draw the costmap itself
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture_);
		glBegin(GL_TRIANGLES);

		glColor4f(1.0f, 1.0f, 1.0f, alpha_->GetValue() );

		glTexCoord2d(0, 0);
		glVertex2f(pts[0].x, pts[0].y);
		glTexCoord2d(1.0, 0);
		glVertex2f(pts[3].x, pts[3].y);
		glTexCoord2d(1.0, 1.0);
		glVertex2f(pts[2].x, pts[2].y);

		glTexCoord2d(0, 0);
		glVertex2f(pts[0].x, pts[0].y);
		glTexCoord2d(1.0, 1.0);
		glVertex2f(pts[2].x, pts[2].y);
		glTexCoord2d(0, 1.0);
		glVertex2f(pts[1].x, pts[1].y);

		glEnd();

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		glDisable(GL_ALPHA_TEST);
		glDisable(GL_BLEND);
	}
	
	virtual void Initialize(Gwen::Controls::Properties* tree)
	{
		// add any properties
		alpha_ = AddFloatProperty(tree, "Alpha", 1.0, 0.0, 1.0, 0.1, "Costmap transparency.");
		
		topic_ = AddTopicProperty(tree, "Topic", "/costmap", "", "pubsub__Costmap");
		topic_->onChange = std::bind(&CostmapPlugin::Subscribe, this, std::placeholders::_1);
		
		show_outline_ = AddBooleanProperty(tree, "Show Outline", true, "If true, draw an outline around the costmap.");
		color_ = AddColorProperty(tree, "Outline Color", Gwen::Color(255,50,50), "Color to use to draw border of costmap.");

		alpha_threshold_ = AddNumberProperty(tree, "Alpha Threshold", -1, -1, 255, 1, "Costs at or below which costmap cells are invisible.");
		alpha_threshold_->onChange = [this](int number)
		{
			UpdateFromMessage();
		};
		
		Subscribe(topic_->GetValue());
	}
	
	std::string GetTitle() override
	{
		return "Costmap";
	}
};

REGISTER_PLUGIN("costmap", CostmapPlugin)

#endif
