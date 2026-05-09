
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
#include <Gwen/Controls/ImagePanel.h>

#include "../FreeImage.h"

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

class ImagePlugin: public pubviz::Plugin
{
	std::unique_ptr<FloatProperty> alpha_;
	std::unique_ptr<ColorProperty> color_;
	std::unique_ptr<BooleanProperty> stretch_;
	std::unique_ptr<BooleanProperty> keep_aspect_;
	
	std::unique_ptr<TopicProperty> topic_;

	Gwen::Controls::ImagePanel* image_panel_;
	Gwen::Controls::TabButton* page_;
	
	std::unique_ptr<pubsub::Subscriber<pubsub::msg::Image>> subscriber_;
	
	pubsub::msg::ImageSharedConstPtr last_msg_;
	
	unsigned int texture_ = -1;//okay, now show image in a new popout

	// returns false if the message is invalid
	bool CheckSize(int bpp)
	{
		int expected = last_msg_->height*last_msg_->width*bpp;

		if (last_msg_->data.size() != expected)
		{
			printf("ERROR: Invalid image data length on topic '%s' got %i but expected %i bytes.\n",
				topic_->GetValue().c_str(), last_msg_->data.size(), expected);

			// mark message as invalid
			last_msg_.reset();
			return false;
		}
		return true;
	}
	
	void UpdateFromMessage()
	{
		image_panel_->Show();
		Redraw();
		
		if (texture_ != -1)
		{
			glDeleteTextures(1, &texture_);
		}
		
		// make the color buffer
		std::vector<uint32_t> pixels;
		pixels.resize(last_msg_->width*last_msg_->height);
		
		// now fill in each pixel
		GLenum texture_format = GL_RGBA;
		if (last_msg_->type == pubsub::msg::Image::R8G8B8A8)
		{
			if (!CheckSize(4)) { return; }
			memcpy(pixels.data(), last_msg_->data.data(), pixels.size());
		}
		else if (last_msg_->type == pubsub::msg::Image::R8G8B8)
		{
			if (!CheckSize(3)) { return; }
			for (int i = 0; i < pixels.size(); i++)
			{
				uint8_t a = 255;
				uint8_t pr = last_msg_->data[i * 3];
				uint8_t pg = last_msg_->data[i * 3 + 1];
				uint8_t pb = last_msg_->data[i * 3 + 2];
				pixels[i] = (a << 24) | (pb << 16) | (pg << 8) | pr;
			}
		}
		else if (last_msg_->type == pubsub::msg::Image::R32)
		{
			if (!CheckSize(4)) { return; }
			for (int i = 0; i < last_msg_->data.size()/4; i++)
			{
				uint8_t px = last_msg_->data[i*4 + 3];// just use the high byte
				uint8_t a = 255;
				pixels[i] = (a << 24) | (px << 16) | (px << 8) | px;
			}
		}
		else if (last_msg_->type == pubsub::msg::Image::R16)
		{
			if (!CheckSize(2)) { return; }
			for (int i = 0; i < last_msg_->data.size()/2; i++)
			{
				uint8_t px = last_msg_->data[i*2];// at the moment use the low byte
				uint8_t a = 255;
				pixels[i] = (a << 24) | (px << 16) | (px << 8) | px;
			}
		}
		else if (last_msg_->type == pubsub::msg::Image::R8)
		{
			if (!CheckSize(1)) { return; }
			for (int i = 0; i < last_msg_->data.size(); i++)
			{
				uint8_t px = last_msg_->data[i];
				uint8_t a = 255;
				pixels[i] = (a << 24) | (px << 16) | (px << 8) | px;
			}
		}
		else if (last_msg_->type == pubsub::msg::Image::YUYV)
		{
			if (!CheckSize(2)) { return; }
			for (int i = 1; i < pixels.size(); i++)
			{
        		auto y = last_msg_->data[i*2];
        		auto u = last_msg_->data[i*2-1];
        		auto v = last_msg_->data[i*2+1];
        		if ((i&0b1) == 0)
          			std::swap(u,v);
        		int8_t c = (int8_t) (y - 16);
        		int8_t d = (int8_t) (u - 128);
        		int8_t e = (int8_t) (v - 128);

				uint8_t a = 255;
        int16_t r = (int16_t)(c + (1.370705f * (e)));
				int16_t g = (int16_t)(c - (0.698001f * (d)) - (0.337633f * (e)));
				int16_t b = (int16_t)(c + (1.732446f * (d)));
				
				if (r < 0)
				  r = 0;
				if (b < 0)
				  b = 0;
				pixels[i] = (a << 24) | (b << 16) | (g << 8) | r;
			}
		}
		else if (last_msg_->type == pubsub::msg::Image::JPEG)
		{
			FIMEMORY* mem = FreeImage_OpenMemory(last_msg_->data.data(), last_msg_->data.size());
			FIBITMAP* img = FreeImage_LoadFromMemory(FIF_JPEG, mem);
			FIBITMAP* bits32 = FreeImage_ConvertTo32Bits( img );
			FreeImage_FlipVertical( bits32 );
			// copy!
			memcpy(pixels.data(), FreeImage_GetBits( bits32 ), 4*last_msg_->height*last_msg_->width);
			FreeImage_Unload( bits32 );
			FreeImage_Unload( img );
			FreeImage_CloseMemory(mem);

#ifdef FREEIMAGE_BIGENDIAN
			texture_format = GL_RGBA;
#else
			texture_format = GL_BGRA;
#endif
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
		  last_msg_->width,
		  last_msg_->height,
		  0,
		  texture_format,
		  GL_UNSIGNED_BYTE,
		  pixels.data());

		glBindTexture(GL_TEXTURE_2D, 0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

		
		Gwen::Texture tex;
		tex.width = last_msg_->width;
		tex.height = last_msg_->height;
		tex.data = (void*)&texture_;
		image_panel_->SetTexture(tex);
	}
	
	std::string current_topic_;
	void Subscribe(std::string str)
	{
		page_->SetText("Image: " + str);
		subscriber_.reset();
		
		Clear();
		
		current_topic_ = str;
    subscriber_.reset(new pubsub::Subscriber<pubsub::msg::Image>(*GetNode(), current_topic_, [](const pubsub::msg::ImageSharedPtr& msg){}, 1, 1));
	}
	
public:

	ImagePlugin()
	{
		// dont use pubsub here
	}
	
	virtual ~ImagePlugin()
	{
		Gwen::Texture tex;
		image_panel_->SetTexture(tex);
		page_->Close();
		
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
			last_msg_.reset();
			texture_ = -1;
		}
	}
		
	virtual void Update()
	{
		// process any messages
		// our sub has a message definition, so the queue contains real messages
		pubsub::msg::Image* data;
		if (subscriber_)
		{
			while (auto data = subscriber_->PopOne())
			{
				if (Paused())
				{
					continue;
				}

				last_msg_ = data;
				UpdateFromMessage();
			}
		}
	}
	
	virtual void Render()
	{
		// nothing to do here since we dont render to the world

		// todo allow right clicking to save as
	}

	virtual void Initialize(Gwen::Controls::Properties* tree)
	{
		auto pubviz = (PubViz*)GetCanvas()->GetParent();
		auto page = pubviz->GetRight()->GetTabControl()->AddPage("Image");
		page_ = page;
		image_panel_ = new Gwen::Controls::ImagePanel(page->GetPage());
		image_panel_->Dock(Gwen::Pos::Fill);
		image_panel_->SetKeepAspectRatio(true);
		image_panel_->Hide();

		// add any properties
		topic_ = AddTopicProperty(tree, "Topic", "/image", "", "pubsub__Image");
		topic_->onChange = std::bind(&ImagePlugin::Subscribe, this, std::placeholders::_1);

		stretch_ = AddBooleanProperty(tree, "Stretch", true, "If true, stretch the image to fill the control.");
		stretch_->onChange = std::bind(&ImagePlugin::StretchChanged, this, std::placeholders::_1);
		
		keep_aspect_ = AddBooleanProperty(tree, "Keep Aspect Ratio", true, "If true, keep the aspect ratio when stretching the image.");
		keep_aspect_->onChange = std::bind(&ImagePlugin::KeepAspectChanged, this, std::placeholders::_1);
		
		Subscribe(topic_->GetValue());
	}

	void KeepAspectChanged(bool b)
	{
		image_panel_->SetKeepAspectRatio(b);
	}

	void StretchChanged(bool b)
	{
		image_panel_->SetStretch(b);
	}
	
	std::string GetTitle() override
	{
		return "Image";
	}
};

REGISTER_PLUGIN("image", ImagePlugin)

#endif
