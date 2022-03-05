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
	m_Color = Gwen::Color( 40, 40, 40, 255 );
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
	
	return true;
}

void OpenGLCanvas::OnMouseClickLeft( int /*x*/, int /*y*/, bool down )
{
	mouse_down_ = down;
}

void OpenGLCanvas::OnMouseMoved(int x, int y, int dx, int dy)
{
	// now convert to units
	double pixels_per_meter = GetCanvas()->Height()/view_height_m_;
	x_mouse_position_ = (x - GetCanvas()->Width()*0.5)/pixels_per_meter;
	y_mouse_position_ = (GetCanvas()->Height()*0.5 - y)/pixels_per_meter;
	
	// now apply offset
	if (mouse_down_)
	{
		view_x_ -= dx/pixels_per_meter;
		view_y_ += dy/pixels_per_meter;
	}
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
	
	// set up the view matrix for the current zoom level
	float half_height = view_height_m_/2.0;
	float half_width = half_height*(float)GetCanvas()->Width()/(float)GetCanvas()->Height();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho( -half_width + view_x_, half_width + view_x_, -half_height + view_y_, half_height + view_y_, -1.0, 1.0 );
	
	// now apply other transforms
	glMatrixMode(GL_MODELVIEW);
	Gwen::Point pos = GetPos();
	glLoadIdentity();
	//glRotatef(yaw, 0, 0, 1);
	//glTranslatef(pos.x, pos.y, 0);

	// Mark the window as dirty so it redraws
	// Todo can maybe do this a bit better so it only redraws on message or movement
	Redraw();
	
	// Render some test items
	float t = fmod(Gwen::Platform::GetTimeInSeconds(), 3.14159*2.0);
	// now do custom rendering here
	glPointSize(50.0f);
	glBegin(GL_POINTS);// GL_LINE_STRIP
	glColor4d(1.0, 1.0, 1.0, 1.0);
	glVertex2d(500, 500);// in pixels atm
	glVertex2d(800, 800);// in pixels atm
	glVertex2d(60, 60);// in pixels atm
	glVertex2d(50*sin(t), 50*cos(t));
	glEnd();
	
	// Draw axes
	glLineWidth(6.0f);
	glBegin(GL_LINES);
	// Red line to the right (x)
	glColor3f(1, 0, 0);
	glVertex2f(0, 0);
	glVertex2f(200, 0);

	// Green line to the top (y)
	glColor3f(0, 1, 0);
	glVertex2f(0, 0);
	glVertex2f(0, 200);
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
	
	// reset matrices
	r->Begin();
}
