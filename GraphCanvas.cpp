// todo license

#include <Gwen/Platform.h>

#include "GraphCanvas.h"

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


GWEN_CONTROL_CONSTRUCTOR( GraphCanvas )
{
	view_height_m_ = 150.0;
	m_Color = Gwen::Color( 255, 255, 255, 255 );
}

bool GraphCanvas::OnMouseWheeled( int delta )
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

void GraphCanvas::OnMouseClickLeft( int /*x*/, int /*y*/, bool down )
{
	mouse_down_ = down;
}

void GraphCanvas::OnMouseMoved(int x, int y, int dx, int dy)
{
	// now convert to units
	double pixels_per_meter = GetCanvas()->Height()/view_height_m_;
	x_mouse_position_ = (x - GetCanvas()->Width()*0.5)/pixels_per_meter + view_x_;
	y_mouse_position_ = (GetCanvas()->Height()*0.5 - y)/pixels_per_meter + view_y_;
	
	// now apply offset
	if (mouse_down_)
	{
		view_x_ -= dx/pixels_per_meter;
		view_y_ += dy/pixels_per_meter;
	}
}

void GraphCanvas::Render( Skin::Base* skin )
{
	auto r = skin->GetRender();
	
	// do whatever we want here
	r->SetDrawColor( m_Color );// start by clearing to background color
	r->DrawFilledRect( GetRenderBounds() );
	
	r->SetDrawColor(Gwen::Color(0,0,0,255));
	
	// testing, make some fake data
	data_.resize(100);
	
	for (int i = 0; i < 100; i++)
	{
		data_[i] = { (double)i/10.0, sin(i*3.14159/10.0) };
	}
	
	const int left_padding = 80;
	const int other_padding = 50;
	
	// target grid cell size
	const int pixel_interval = 100;
	
	// Calculate size to make the graph
	Gwen::Rect b = GetRenderBounds();
	const int graph_width = b.w - left_padding - other_padding;
	const int graph_height = b.h - other_padding*2.0;
	
	// Now calculate number of cells to make on each axis
	double x_count = (int)std::max(1.0, (double)graph_width/(double)pixel_interval);
	double y_count = (int)std::max(1.0, (double)graph_height/(double)pixel_interval);
	
	const double x_cell_size = graph_width/x_count;
	const double y_cell_size = graph_height/y_count;
		
	const double start_x = left_padding;
	const double start_y = other_padding;
	
	const double x_interval = (max_x_ - min_x_)/x_count;
	const double y_interval = (max_y_ - min_y_)/y_count;
	
	// lets make a grid for the graph
	
	// start with x lines, start with a fixed number of segments that fill the width
	int i = 0;
	for (double x = start_x; x < start_x + (x_count+0.001)*x_cell_size; x += x_cell_size)
	{
		r->RenderText(skin->GetDefaultFont(), Gwen::PointF( x, b.h - 30 ), std::to_string(min_x_ + (i++)*x_interval));
	}
		
	i = 0;
	for (double y = start_y; y < start_y + (y_count+0.001)*y_cell_size; y += y_cell_size)
	{
		r->RenderText(skin->GetDefaultFont(), Gwen::PointF( 10, y ), std::to_string(min_y_ + (y_count-i++)*y_interval));
	}
	
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
	
	// now apply other transforms
	glMatrixMode(GL_MODELVIEW);
	Gwen::Point pos = LocalPosToCanvas(Gwen::Point(0, 0));
	glLoadIdentity();
	//glRotatef(yaw, 0, 0, 1);
	glTranslatef(pos.x, pos.y, 0);// shift it so 0, 0 is _our_ top left corner

	// Mark the window as dirty so it redraws
	// Todo can maybe do this a bit better so it only redraws on message or movement
	Redraw();
		
	glLineWidth(3.0f);
	glBegin(GL_LINES);
	glColor3f(0, 0, 0);
	for (double x = start_x; x < start_x + (x_count+0.001)*x_cell_size; x += x_cell_size)
	{
		glVertex2f(x, start_y);
		glVertex2f(x, start_y + y_count*y_cell_size + 10);
	}
		
	for (double y = start_y; y < start_y + (y_count+0.001)*y_cell_size; y += y_cell_size)
	{
		glVertex2f(start_x - 10, y);
		glVertex2f(start_x + x_count*x_cell_size, y);
	}
	glEnd();
	
	// Draw the graph line
	glLineWidth(4.0f);
	glBegin(GL_LINE_STRIP);
	glColor3f(1, 0, 0);
	for (auto& pt: data_)
	{
		glVertex2f(start_x + graph_width*(pt.first - min_x_)/(max_x_ - min_x_),
		           start_y + y_count*y_cell_size - graph_height*(pt.second - min_y_)/(max_y_ - min_y_));
	}
	glEnd();
	
	// force a flush essentially
	r->EndClip();
	r->StartClip();
	
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
