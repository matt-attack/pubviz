// todo license

#include <Gwen/Platform.h>

#include "OpenGLCanvas.h"

#include <GL/glew.h>

#include "../Plugin.h"

#ifndef _WIN32
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#endif

#include <cmath>

#undef near

using namespace Gwen;
using namespace Gwen::Controls;

GWEN_CONTROL_CONSTRUCTOR( OpenGLCanvas )
{
	view_height_m_ = 150.0;
	m_Color = Gwen::Color( 50, 50, 50, 255 );
}

bool OpenGLCanvas::OnMouseWheeled( int delta )
{
	if (view_type_->GetValue() == ViewType::FPS)
	{
		// move view along view axis
		double dx = cos(-yaw_->GetValue()*M_PI/180.0)*cos(pitch_->GetValue()*M_PI/180.0);
		double dy = sin(-yaw_->GetValue()*M_PI/180.0)*cos(pitch_->GetValue()*M_PI/180.0);
		double dz = sin(pitch_->GetValue()*M_PI/180.0);
		view_x_->SetValue(view_x_->GetValue() + dx*delta*0.01);
		view_y_->SetValue(view_y_->GetValue() + dy*delta*0.01);
		view_z_->SetValue(view_z_->GetValue() + dz*delta*0.01);

		Redraw();

		return true;
	}

	if (delta < 0)
	{
		view_height_m_ += 0.1*(double)delta;
		view_height_m_ = std::max(1.0, view_height_m_);
	}
	else
	{
		view_height_m_ += 0.1*(double)delta;
	}
	
	// Mark the window as dirty so it redraws
	Redraw();
	
	return true;
}

void OpenGLCanvas::ResetView()
{
	view_height_m_ = 150.0;
	view_x_->SetValue(0.0);
	view_y_->SetValue(0.0);
	view_z_->SetValue(0.0);
	view_abs_x_ = 0.0;
	view_abs_y_ = 0.0;
	view_abs_z_ = 0.0;
	pitch_->SetValue(0.0);
	yaw_->SetValue(0.0);
	
	Redraw();
}

void OpenGLCanvas::OnMouseClickLeft( int /*x*/, int /*y*/, bool down )
{
	mouse_down_ = down;
}

void OpenGLCanvas::OnMouseMoved(int x, int y, int dx, int dy)
{
	// now convert to units
	double pixels_per_meter = GetCanvas()->Height()/view_height_m_;
	x_mouse_position_ = (x - GetCanvas()->Width()*0.5)/pixels_per_meter + view_x_->GetValue();
	y_mouse_position_ = (GetCanvas()->Height()*0.5 - y)/pixels_per_meter + view_y_->GetValue();
	
	// now apply offset
	if (mouse_down_)
	{
		if (view_type_->GetValue() == ViewType::TopDown)
		{
			view_x_->SetValue(view_x_->GetValue() - dx/pixels_per_meter);
			view_y_->SetValue(view_y_->GetValue() + dy/pixels_per_meter);

			view_abs_x_ -= dx / pixels_per_meter;
			view_abs_y_ += dy / pixels_per_meter;
		}
		else
		{
			double new_pitch = pitch_->GetValue() + dy;
			double new_yaw = yaw_->GetValue() + dx;

			pitch_->SetValue(new_pitch);
			yaw_->SetValue(new_yaw);
		}
		
		// Mark the window as dirty so it redraws
		Redraw();
	}
}

#include <pubsub_cpp/Time.h>
#include <Gwen/../../Renderers/OpenGL/FreeImage/FreeImage.h>

void OpenGLCanvas::Screenshot()
{
	auto scale = GetCanvas()->Scale();
	auto origin = LocalPosToCanvas();
	origin.y -= 20;// skip past the menu bar
	int width = Width()*scale;
	int height = Height()*scale;
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	uint8_t* pixels = new uint8_t[3*width*height];

	glReadPixels(origin.x * scale, origin.y * scale - 1, width, height - 1, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	FIBITMAP* image = FreeImage_ConvertFromRawBits(pixels, width, height, 3 * width, 24, 0x0000FF, 0xFF0000, 0x00FF00, false);
	char str[500];
	int len = snprintf(str, 500, "Screenshot %s.bmp", pubsub::Time::now().toString().c_str());
	for (int i = 0; i < len; i++)
	{
		if (str[i] == ':' || str[i] == ' ')
			str[i] = '_';
	}

	FreeImage_Save(FIF_BMP, image, str, 0);
	delete[] pixels;
}

void OpenGLCanvas::Layout( Gwen::Skin::Base* skin )
{
	// Update each plugin here to see if they want a redraw
	for (auto plugin: plugins_)
	{
		if (plugin->Enabled())
		{
			plugin->Update();
		}
	}
	Invalidate();// we are being hacky, just always invalidate so we keep laying out
}

std::map<std::string, PropertyBase*> OpenGLCanvas::CreateProperties(Gwen::Controls::Properties* tree)
{
	std::map<std::string, PropertyBase*> props;
	view_type_ = new EnumProperty(tree, "View Type", "Orbit", {"Orbit", "Top Down", "FPS"});
	props["view"] = view_type_;
	yaw_ = new FloatProperty(tree, "Yaw", 0, -100000, 100000);
	props["yaw"] = yaw_;
	pitch_ = new FloatProperty(tree, "Pitch", 0, -100000, 100000);
	props["pitch"] = pitch_;
	view_x_ = new FloatProperty(tree, "View X", 0, -100000, 100000);
	props["View X"] = view_x_;
	view_y_ = new FloatProperty(tree, "View Y", 0, -100000, 100000);
	props["View Y"] = view_y_;
	view_z_ = new FloatProperty(tree, "View Z", 0, -100000, 100000);
	props["View Z"] = view_z_;

	view_type_->onChange = [this](std::string value)
	{
		pitch_->Hide();
		yaw_->Hide();
		view_z_->Hide();
		if (value == ViewType::Orbit)
		{
			pitch_->Show();
			yaw_->Show();
			view_z_->Show();
		}
		else if (value == ViewType::FPS)
		{
			pitch_->Show();
			yaw_->Show();
			view_z_->Show();
		}
		else if (value == ViewType::TopDown)
		{
			
		}
	};

	return props;
}

void OpenGLCanvas::Render( Skin::Base* skin )
{
	auto r = skin->GetRender();
	
	// do whatever we want here
	skin->GetRender()->SetDrawColor( m_Color );// start by clearing to background color
	skin->GetRender()->DrawFilledRect( GetRenderBounds() );

    auto origin = LocalPosToCanvas();
    auto width = Width();
    auto height = Height();
	
	// force a flush essentially
	r->EndClip();
	r->StartClip();
        
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	double view_x = view_x_->GetValue();
	double view_y = view_y_->GetValue();
	double view_z = view_z_->GetValue();
	if (wgs84_mode_)
	{
		view_x = view_abs_x_;
		view_y = view_abs_y_;
		view_z = view_abs_z_;
		//local_xy_.FromLatLon(view_lat_, view_lon_, view_x, view_y);
	}
	origin.y -= 20;// skip past menu bar

    float vp[4];
    glGetFloatv(GL_VIEWPORT, vp);
	auto scale = GetCanvas()->Scale();
	glViewport(origin.x * scale, origin.y * scale, width * scale, height * scale);
	
	double yaw = yaw_->GetValue();
	double pitch = pitch_->GetValue();
	auto view_type = view_type_->GetValue();
	if (view_type == ViewType::TopDown)
	{
		// set up the view matrix for the current zoom level (ortho, topdown)
		float half_height = view_height_m_/2.0;
		float half_width = half_height*((float)width/(float)height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho( -half_width + view_x, half_width + view_x, -half_height + view_y, half_height + view_y, -10000.0, 10000.0 );
	
		// now apply other transforms
		glMatrixMode(GL_MODELVIEW);
		Gwen::Point pos = GetPos();
		glLoadIdentity();
	}
	else if (view_type == ViewType::FPS)
	{
		double near = 1.0;
		double fov = 45*M_PI/180.0;// horizontal
		double aspect_ratio = ((double)width)/((double)height);
		double width = near*tan(fov);
		double height = width/aspect_ratio;
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		// x, y, z
		glFrustum( -width/2, width/2, -height/2, height/2, near, 10000.0 );
	
		// now apply other transforms
		glMatrixMode(GL_MODELVIEW);
		Gwen::Point pos = GetPos();
		glLoadIdentity();
		
		// eh, I dislike this but whatever
		gluLookAt(view_x, view_y, view_z, /* look from camera XYZ */
             view_x - cos(-yaw*M_PI/180.0)*cos(pitch*M_PI/180.0), view_y - sin(-yaw*M_PI/180.0)*cos(pitch*M_PI/180.0), view_z - sin(pitch*M_PI/180.0), /* look at the origin */
             0, 0, 1); /* positive Z up vector */
		//glRotatef(-90, 1, 0, 0);
		//glTranslatef(-view_x, -view_y, -view_z);
	
		//glRotatef(-90, 1, 0, 0);
		//glRotatef(-90, 0, 1, 0);
		//glRotatef(yaw_, 0, 0, 1);
	
		
		float ax = cos((-yaw)*3.14159/180.0);
		float ay = sin((-yaw)*3.14159/180.0);
		//glRotatef(pitch_, 0,1,0);//ax, ay, 0);
//glRotatef(yaw_, 0, 0, 1);
		
		// we want depth testing in this mode
		glEnable(GL_DEPTH_TEST);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LEQUAL);
	}
	else if (view_type == ViewType::Orbit)
	{
		// set up the view matrix for the current zoom level (orbit)
		float half_height = view_height_m_/2.0;
		float half_width = half_height*((float)width/(float)height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho( -half_width, half_width, -half_height, half_height, -10000.0, 10000.0 );
	
		// now apply other transforms
		glMatrixMode(GL_MODELVIEW);
		Gwen::Point pos = GetPos();
		glLoadIdentity();
	
		glRotatef(-90, 1, 0, 0);
		glRotatef(yaw, 0, 0, 1);
	
		float ax = cos((-yaw)*3.14159/180.0);
		float ay = sin((-yaw)*3.14159/180.0);
		glRotatef(pitch, ax, ay, 0);
		
		
		glTranslatef(-view_x, -view_y, -view_z);
		
		// we want depth testing in this mode
		glEnable(GL_DEPTH_TEST);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LEQUAL);
	}

	// Draw axes at origin
	if (show_origin_)
	{
		glLineWidth(6.0f);
		glBegin(GL_LINES);
		// Red line to the right (x)
		glColor3f(1, 0, 0);
		glVertex3f(0, 0, 0);
		glVertex3f(55, 0, 0);

		// Green line to the top (y)
		glColor3f(0, 1, 0);
		glVertex3f(0, 0, 0);
		glVertex3f(0, 55, 0);
	
		// Blue line up (z)
		glColor3f(0, 0, 1);
		glVertex3f(0, 0, 0);
		glVertex3f(0, 0, 55);
		glEnd();
	}
	
	// Now draw all the plugins
	for (auto plugin: plugins_)
	{
		if (plugin->Enabled())
		{
			plugin->Render();
		}
	}
	
	glPopAttrib();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	
	glDisable(GL_DEPTH_TEST);

    glViewport(vp[0], vp[1], vp[2], vp[3]);
	
	// reset matrices
	r->Begin();
}
