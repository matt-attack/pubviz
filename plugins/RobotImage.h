
#ifndef PUBVIZ_PLUGIN_ROBOT_IMAGE_H
#define PUBVIZ_PLUGIN_ROBOT_IMAGE_H

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

class RobotImagePlugin: public pubviz::Plugin
{
	std::unique_ptr<FloatProperty> alpha_;
	std::unique_ptr<ColorProperty> color_;
	std::unique_ptr<BooleanProperty> show_outline_;

	std::unique_ptr<FloatProperty> width_, length_, x_offset_;
	
	std::unique_ptr<FileProperty> image_;
	
	unsigned int texture_ = -1;
	
	void OnImageChange(std::string file)
	{
		if (texture_ != -1)
		{
			glDeleteTextures(1, &texture_);
		}

		int width = 10;
		int height = 10;
		
		// make the color buffer
		std::vector<uint32_t> pixels;

		FREE_IMAGE_FORMAT imageFormat = FreeImage_GetFileType( file.c_str() );

		if ( imageFormat == FIF_UNKNOWN )
		{ imageFormat = FreeImage_GetFIFFromFilename( file.c_str() ); }

		FIBITMAP* bits = FreeImage_Load( imageFormat, file.c_str() );
		if (bits)
		{
			// Convert to 32bit
			FIBITMAP* bits32 = FreeImage_ConvertTo32Bits( bits );
			FreeImage_Unload( bits );

			auto rotated = FreeImage_Rotate(bits32, -90);
			FreeImage_Unload(bits32);
			const uint32_t* data = (const uint32_t*)FreeImage_GetBits( rotated );
			width = FreeImage_GetWidth( rotated );
			height = FreeImage_GetHeight( rotated );
			pixels.resize(width*height);
			for (int i = 0; i < pixels.size(); i++)
			{
				pixels[i] = data[i];
			}

			FreeImage_Unload( rotated );
		}
		else
		{
			printf("file '%s' not found\n", file.c_str());
			pixels.resize(width*height);
			for (int i = 0; i < pixels.size(); i++)
			{
				pixels[i] = 0xffffffff;
			}
		}

#ifdef FREEIMAGE_BIGENDIAN
		GLenum format = GL_RGBA;
#else
		GLenum format = GL_BGRA;
#endif
		
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
		  width,
		  height,
		  0,
		  format,
		  GL_UNSIGNED_BYTE,
		  pixels.data());

		glBindTexture(GL_TEXTURE_2D, 0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}
	
public:

	RobotImagePlugin()
	{	
		// dont use pubsub here
	}
	
	virtual ~RobotImagePlugin()
	{	
		if (texture_ != -1)
		{
			glDeleteTextures(1, &texture_);
		}
	}

	// Clear out any historical data so the view gets cleared
	virtual void Clear()
	{

	}
	
	virtual void Update()
	{

	}

	std::vector<std::pair<Plugin::ErrorSeverity, std::string>> errors_;
	std::vector<std::pair<Plugin::ErrorSeverity, std::string>> GetErrors() override
	{
	  return errors_;
	}
	
	virtual void Render()
	{		
		double width = length_->GetValue();
		double height = width_->GetValue();
		double x_offset = x_offset_->GetValue();

		Vec3d pts[4];
		pts[0] = Vec3d(-width/2 + x_offset, -height/2, 0);
		pts[1] = Vec3d(-width/2 + x_offset, height/2, 0);
		pts[2] = Vec3d(width/2 + x_offset, height/2, 0);
		pts[3] = Vec3d(width/2 + x_offset, -height/2, 0);

		// transform to view frame
		errors_.clear();
		try
		{
		  for (int i = 0; i < 4; i++)
		  {
			  GetCanvas()->TransformToView(OpenGLCanvas::Vehicle, pts[i]);
		  }
		}
		catch (const TransformException& e)
		{
		  errors_.emplace_back(Plugin::ERROR, e.what());
		  return;
		}
		
		// draw the bounds of the costmap
		if (show_outline_->GetValue())
		{
			glLineWidth(4.0f);
			glBegin(GL_LINE_STRIP);
			
			glColor3f(0, 0, 0);
			
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
		alpha_ = AddFloatProperty(tree, "Alpha", 1.0, 0.0, 1.0, 0.1, "Image transparency.");
		
		image_ = AddFileProperty(tree, "Image", "");
		image_->onChange = std::bind(&RobotImagePlugin::OnImageChange, this, std::placeholders::_1);
		
		show_outline_ = AddBooleanProperty(tree, "Show Outline", true, "If true, draw an outline around the costmap.");
		
		length_ = AddFloatProperty(tree, "Length", 1.0, 0.0, 20.0, 0.1, "Width of robot.");
		width_ = AddFloatProperty(tree, "Width", 1.0, 0.0, 20.0, 0.1, "Width of robot");
		x_offset_ = AddFloatProperty(tree, "X Offset", 0.0, 0.0, 20.0, 0.1, "X offset from center.");
		
		OnImageChange(image_->GetValue());
	}
	
	std::string GetTitle() override
	{
		return "Robot Image";
	}
};

REGISTER_PLUGIN("robot_image", RobotImagePlugin)

#endif
