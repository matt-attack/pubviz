// todo license

#include <Gwen/Platform.h>

#include "OpenGLCanvas.h"

#include <GL/glew.h>

#ifndef _WIN32
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#endif

#include <cmath>

using namespace Gwen;
using namespace Gwen::Controls;

GWEN_CONTROL_CONSTRUCTOR( OpenGLCanvas )
{
	view_height_m_ = 150.0;
	m_Color = Gwen::Color( 50, 50, 50, 255 );
}

bool OpenGLCanvas::OnMouseWheeled( int delta )
{
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
	view_x_ = 0.0;
	view_y_ = 0.0;
	view_z_ = 0.0;
	pitch_ = 0.0;
	yaw_ = 0.0;
	
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
	x_mouse_position_ = (x - GetCanvas()->Width()*0.5)/pixels_per_meter + view_x_;
	y_mouse_position_ = (GetCanvas()->Height()*0.5 - y)/pixels_per_meter + view_y_;
	
	// now apply offset
	if (mouse_down_)
	{
		if (view_type_ == ViewType::TopDown)
		{
			view_x_ -= dx/pixels_per_meter;
			view_y_ += dy/pixels_per_meter;
		}
		else
		{
			pitch_ += dy;
			yaw_ += dx;
		}
		
		// Mark the window as dirty so it redraws
		Redraw();
	}
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

void OpenGLCanvas::Render( Skin::Base* skin )
{
	auto r = skin->GetRender();
	
	// do whatever we want here
	skin->GetRender()->SetDrawColor( m_Color );// start by clearing to background color
	skin->GetRender()->DrawFilledRect( GetRenderBounds() );
	
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
	
	if (view_type_ == ViewType::TopDown)
	{
		// set up the view matrix for the current zoom level (ortho, topdown)
		float half_height = view_height_m_/2.0;
		float half_width = half_height*(float)GetCanvas()->Width()/(float)GetCanvas()->Height();
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho( -half_width + view_x_, half_width + view_x_, -half_height + view_y_, half_height + view_y_, -10000.0, 10000.0 );
	
		// now apply other transforms
		glMatrixMode(GL_MODELVIEW);
		Gwen::Point pos = GetPos();
		glLoadIdentity();
	}
	else if (view_type_ == ViewType::Orbit)
	{
		// set up the view matrix for the current zoom level (orbit)
		float half_height = view_height_m_/2.0;
		float half_width = half_height*(float)GetCanvas()->Width()/(float)GetCanvas()->Height();
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho( -half_width, half_width, -half_height, half_height, -10000.0, 10000.0 );
	
		// now apply other transforms
		glMatrixMode(GL_MODELVIEW);
		Gwen::Point pos = GetPos();
		glLoadIdentity();
	
		glRotatef(-90, 1, 0, 0);
		glRotatef(yaw_, 0, 0, 1);
	
		float ax = cos((-yaw_)*3.14159/180.0);
		float ay = sin((-yaw_)*3.14159/180.0);
		glRotatef(pitch_, ax, ay, 0);
		
		
		glTranslatef(view_x_, view_y_, view_z_);
		
		// we want depth testing in this mode
		glEnable(GL_DEPTH_TEST);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LEQUAL);
	}

	// Draw axes
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
	
	// reset matrices
	r->Begin();
}
