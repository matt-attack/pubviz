
#ifndef PUBVIZ_PLUGIN_IMAGE_H
#define PUBVIZ_PLUGIN_IMAGE_H

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

#include <pubsub/Image.msg.h>

class ImagePlugin: public Plugin
{
	FloatProperty* alpha_;
	ColorProperty* color_;
	BooleanProperty* show_outline_;
	
	TopicProperty* topic_;
	
	bool sub_open_ = false;
	ps_sub_t subscriber_;
	
	pubsub::msg::Image last_msg_;
	
	unsigned int texture_ = -1;//okay, now show image in a new popout

	// returns false if the message is invalid
	bool CheckSize(const pubsub::msg::Image& image, int bpp)
	{
		int expected = image.height*image.width*bpp;

		if (image.data_length != expected)
		{
			printf("ERROR: Invalid image data length on topic '%s' got %i but expected %i\n",
				topic_->GetValue().c_str(), image.data_length, expected);

			// mark message as invalid
			if (last_msg_.data)
			{
				free(last_msg_.data);
				last_msg_.data = 0;
				last_msg_.data_length = 0;
			}
			return false;
		}
		return true;
	}
	
	void UpdateFromMessage()
	{
		Redraw();
		
		if (texture_ != -1)
		{
			glDeleteTextures(1, &texture_);
		}
		
		// make the color buffer
		std::vector<uint32_t> pixels;
		pixels.resize(last_msg_.width*last_msg_.height);
		
		// now fill in each pixel
		if (last_msg_.type == pubsub::msg::Image::R8G8B8A8)
		{
			if (!CheckSize(last_msg_, 4)) { return; }
			memcpy(pixels.data(), last_msg_.data, pixels.size());
		}
		else if (last_msg_.type == pubsub::msg::Image::R8G8B8)
		{
			if (!CheckSize(last_msg_, 3)) { return; }
			for (int i = 0; i < pixels.size(); i++)
			{
				uint8_t a = 255;
				uint8_t pr = last_msg_.data[i * 3];
				uint8_t pg = last_msg_.data[i * 3 + 1];
				uint8_t pb = last_msg_.data[i * 3 + 2];
				pixels[i] = (a << 24) | (pb << 16) | (pg << 8) | pr;
			}
		}
		else if (last_msg_.type == pubsub::msg::Image::R32)
		{
			if (!CheckSize(last_msg_, 4)) { return; }
			for (int i = 0; i < last_msg_.data_length; i++)
			{
				uint8_t px = last_msg_.data[i*4 + 3];// just use the high byte
				uint8_t a = 255;
				pixels[i] = (a << 24) | (px << 16) | (px << 8) | px;
			}
		}
		else if (last_msg_.type == pubsub::msg::Image::R16)
		{
			if (!CheckSize(last_msg_, 2)) { return; }
			for (int i = 0; i < last_msg_.data_length; i++)
			{
				uint8_t px = last_msg_.data[i*2 + 1];// just use the high byte
				uint8_t a = 255;
				pixels[i] = (a << 24) | (px << 16) | (px << 8) | px;
			}
		}
		else if (last_msg_.type == pubsub::msg::Image::R8)
		{
			if (!CheckSize(last_msg_, 1)) { return; }
			for (int i = 0; i < last_msg_.data_length; i++)
			{
				uint8_t px = last_msg_.data[i];
				uint8_t a = 255;
				pixels[i] = (a << 24) | (px << 16) | (px << 8) | px;
			}
		}
		else
		{
			printf("ERROR: Unhandled image type\n");
			// just blank
			for (int i = 0; i < pixels.size(); i++)
			{
				pixels[i] = 0;
			}
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
		  last_msg_.width,
		  last_msg_.height,
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
		
		if (texture_ != -1)
		{
			glDeleteTextures(1, &texture_);
			last_msg_.data_length = 0;
			free(last_msg_.data);
			last_msg_.data = 0;
			texture_ = -1;
		}
		
		current_topic_ = str;
    	struct ps_subscriber_options options;
    	ps_subscriber_options_init(&options);
    	options.preferred_transport = 1;// tcp yo
    	ps_node_create_subscriber_adv(GetNode(), current_topic_.c_str(), &pubsub__Image_def, &subscriber_, &options);
    	sub_open_ = true;
	}
	
public:

	ImagePlugin()
	{
		// just make a quick test costmap
		last_msg_.width = 0;
		last_msg_.height = 0;
		last_msg_.data_length = 0;
	}
	
	virtual ~ImagePlugin()
	{
		//delete color_;
		delete alpha_;
		//delete show_outline_;
		
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
	
		
	virtual void Update()
	{
		// process any messages
		// our sub has a message definition, so the queue contains real messages
		pubsub::msg::Image* data;
		if (sub_open_)
		{
			while (data = (pubsub::msg::Image*)ps_sub_deque(&subscriber_))
			{
				if (Paused())
				{
					free(data->data);
					free(data);//todo use allocator free
					continue;
				}

				// user is responsible for freeing the message and its arrays
				last_msg_ = *data;
				free(data->data);
				free(data);//todo use allocator free
				UpdateFromMessage();
			}
		}
	}
	
	virtual void Render()
	{		
		// exit early if we dont have a messag
		if (last_msg_.data_length == 0)
		{
			return;
		}

		if (GetCanvas()->wgs84_mode_)
		{
			return;// not supported for the moment
		}
		
		double width = 50;// last_msg_.resolution* last_msg_.width;
		double height = 50;// last_msg_.resolution* last_msg_.height;
		double left = 0;
		double bottom = 0;
		
		// draw the bounds of the costmap
		/*if (show_outline_->GetValue())
		{
			glLineWidth(4.0f);
			glBegin(GL_LINE_STRIP);
			
			Gwen::Color color = color_->GetValue();
			glColor3f(color.r/255.0, color.g/255.0, color.b/255.0);
			
			glVertex2f(left, bottom);
			glVertex2f(left, bottom + height);
			glVertex2f(left + width, bottom + height);
			glVertex2f(left + width, bottom);
			glVertex2f(left, bottom);
			
			glEnd();
		}*/
		
		// Now draw the costmap itself
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture_);
		glBegin(GL_TRIANGLES);

		glColor4f(1.0f, 1.0f, 1.0f, alpha_->GetValue() );

		glTexCoord2d(0, 0);
		glVertex2d(left + 0, bottom + 0);
		glTexCoord2d(1.0, 0);
		glVertex2d(left + width, bottom + 0);
		glTexCoord2d(1.0, 1.0);
		glVertex2d(left + width, bottom + height);

		glTexCoord2d(0, 0);
		glVertex2d(left + 0, bottom + 0);
		glTexCoord2d(1.0, 1.0);
		glVertex2d(left + width, bottom + height);
		glTexCoord2d(0, 1.0);
		glVertex2d(left + 0, bottom + height);

		glEnd();

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}
	
	virtual void Initialize(Gwen::Controls::Properties* tree)
	{
		// add any properties
		alpha_ = AddFloatProperty(tree, "Alpha", 1.0, 0.0, 1.0, 0.1, "Costmap transparency.");
		
		topic_ = AddTopicProperty(tree, "Topic", "/image", "");
		topic_->onChange = std::bind(&ImagePlugin::Subscribe, this, std::placeholders::_1);
		
		//show_outline_ = AddBooleanProperty(tree, "Show Outline", true, "If true, draw an outline around the costmap.");
		//color_ = AddColorProperty(tree, "Outline Color", Gwen::Color(255,50,50), "Color to use to draw border of costmap.");
		
		Subscribe(topic_->GetValue());
	}
	
	std::string GetTitle() override
	{
		return "Image";
	}
};

REGISTER_PLUGIN("image", ImagePlugin)

#endif
