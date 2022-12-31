// todo license

#include <Gwen/Platform.h>
#include <Gwen/Controls/CheckBox.h>
#include <Gwen/Controls/Menu.h>
#include <Gwen/Controls/TextBox.h>
#include <Gwen/Controls/WindowControl.h>

#include "SackGraph.h"
#include "SackViewer.h"

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


GWEN_CONTROL_CONSTRUCTOR( SackGraph )
{

}

/*void SackGraph::Layout(Gwen::Skin::Base* skin)
{   
    // handle 
    auto width = Width();
	remove_button_.SetPos(width - 40, 10);
	configure_button_.SetPos(width - 40, 40);

	BaseClass::Layout(skin);
}*/

bool SackGraph::OnMouseWheeled( int delta )
{
	return true;
}

void SackGraph::OnMouseClickLeft( int x, int y, bool down )
{
    mouse_down_ = down;
    OnMouseMoved(x, y, 0, 0);
}

void SackGraph::OnMouseMoved(int x, int y, int dx, int dy)
{
    // move playhead if we are currently mouse down
    if (mouse_down_)
    {
        auto start_x = GraphStartPosition();
        auto graph_width = GraphWidth();

        x = CanvasPosToLocal(Gwen::Point(x, y)).x;

        // now determine click location here
        double rel_time = ((x - start_x)/graph_width)*(max_x_ - min_x_) + min_x_;
        uint64_t abs_time = rel_time*1000000.0 + viewer_->GetStartTime();
        viewer_->SetPlayheadTime(abs_time);
        Redraw();
    }
}

void SackGraph::SetViewer(SackViewer* viewer)
{
    viewer_ = viewer;
    max_x_ = (viewer->GetEndTime() - viewer->GetStartTime())/1000000.0;
    start_time_ = viewer->GetStartTime();
}

void SackGraph::DrawOnGraph(double start_x, double start_y, double graph_width, double graph_height)
{
    auto r = GetSkin()->GetRender();

    // convert timestamp to bag time
    double slider_time = (viewer_->GetPlayheadTime() - viewer_->GetStartTime())/1000000.0;

    // draw the playhead
    glLineWidth(4.0f);
	glBegin(GL_LINE_STRIP);
	glColor3f(1.0, 0.0, 0.0);
    double x = start_x + graph_width*(slider_time/*position here*/ - min_x_)/(max_x_ - min_x_);
    glVertex2f(x, start_y);
    glVertex2f(x, start_y + graph_height);
		/*for (auto& pt: sub->data)
		{
			glVertex2f(start_x + graph_width*(pt.first - min_x_)/(max_x_ - min_x_),
		           start_y + y_count*y_cell_size - graph_height*(pt.second - min_y_)/(max_y_ - min_y_));
		}
		j++;*/
	glEnd();
}

void SackGraph::Render( Skin::Base* skin )
{
    GraphBase::Render(skin);

    auto r = skin->GetRender();

    // todo draw vertical line where the sack "time" is
    r->SetDrawColor(Gwen::Color(255,0,0,255));

	/*auto r = skin->GetRender();
	
	// do whatever we want here
	r->SetDrawColor( m_Color );// start by clearing to background color
	r->DrawFilledRect( GetRenderBounds() );
	
	r->SetDrawColor(Gwen::Color(0,0,0,255));

	//AddSample(sin(fmod(pubsub::Time::now().toSec(), 3.14159*2.0)), pubsub::Time::now());
	
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

    // do autoscale
    if (autoscale_y_)
    {
        bool data_point = false;
        double current_min_y = std::numeric_limits<double>::max();
        double current_max_y = -std::numeric_limits<double>::max();
       	for (auto& sub: channels_)
	    {
		    for (auto& pt: sub->data)
		    {
                current_min_y = std::min(current_min_y, pt.second);
                current_max_y = std::max(current_max_y, pt.second);
                data_point = true;
            }
        }

        if (redo_scale_ && data_point)
        {
            redo_scale_ = false;
            min_y_ = std::numeric_limits<double>::max();
            max_y_ = -std::numeric_limits<double>::max();
        }
        
        if (redo_scale_)
        {
            min_y_ = -1.0;
            max_y_ = 1.0;
        }
        else
        {
            // add a bit of a buffer on each edge (5% of difference)
            const double difference = std::abs(current_min_y - current_max_y);
            double buffer = difference * 0.05;
            current_max_y += buffer;
            current_min_y -= buffer;

            max_y_ = std::max(max_y_, current_max_y);
            min_y_ = std::min(min_y_, current_min_y);
        }

        // handle max and min being identical resulting in bad graphs
        if (std::abs(min_y_ - max_y_) < 0.0001)
        {
            max_y_ += 0.0001;
            min_y_ -= 0.0001;
        }
    }
	
	const double x_cell_size = graph_width/x_count;
	const double y_cell_size = graph_height/y_count;
		
	const double start_x = left_padding;
	const double start_y = other_padding;
	
	const double x_interval = (max_x_ - min_x_)/x_count;
	const double y_interval = (max_y_ - min_y_)/y_count;
	
	// lets make a grid for the graph
	char buffer[50];
	
	// start with x lines, start with a fixed number of segments that fill the width
	int i = 0;
	for (double x = start_x; x < start_x + (x_count+0.001)*x_cell_size; x += x_cell_size)
	{
		double val = min_x_ + (i++)*x_interval;
		if (std::abs(val) > 1.0 || val == 0.0)
		{
			sprintf(buffer, "%0.1lf", val);
		}
		else
		{
			sprintf(buffer, "%lf", val);
		}
		r->RenderText(skin->GetDefaultFont(), Gwen::PointF( x, b.h - 30 ), (std::string)buffer);
	}
		
	i = 0;
	for (double y = start_y; y < start_y + (y_count+0.001)*y_cell_size; y += y_cell_size)
	{
		double val = min_y_ + (y_count-i++)*y_interval;
		if (std::abs(val) > 1.0 || val == 0.0)
		{
			sprintf(buffer, "%0.1lf", val);
		}
		else
		{
			sprintf(buffer, "%lf", val);
		}
		r->RenderText(skin->GetDefaultFont(), Gwen::PointF( 10, y ), (std::string)buffer);
	}
	
	// force a flush essentially
	r->StartClip();
	
	// Mark the window as dirty so it redraws
	// Todo can maybe do this a bit better so it only redraws on message or movement
	//Redraw();
        
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	
	// now apply other transforms
	glMatrixMode(GL_MODELVIEW);
	Gwen::Point pos = r->GetRenderOffset();
	glLoadIdentity();
	//glRotatef(yaw, 0, 0, 1);
	glTranslatef(pos.x, pos.y, 0);// shift it so 0, 0 is _our_ top left corner
	
	// Draw graph grid lines
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
	
	// Set a clip region around the graph to crop anything out of range
	Gwen::Rect old_clip_region = r->ClipRegion();
	Gwen::Point start_pos = LocalPosToCanvas(Gwen::Point(start_x, start_y));
	r->SetClipRegion(Gwen::Rect(start_pos.x, start_pos.y, graph_width, graph_height));
	r->StartClip();
	
	// Draw the graph line
	glLineWidth(4.0f);
	int j = 0;
	for (auto& sub: channels_)
	{
		glBegin(GL_LINE_STRIP);
		glColor3f(colors[j%6][0], colors[j%6][1], colors[j%6][2]);
		for (auto& pt: sub->data)
		{
			glVertex2f(start_x + graph_width*(pt.first - min_x_)/(max_x_ - min_x_),
		           start_y + y_count*y_cell_size - graph_height*(pt.second - min_y_)/(max_y_ - min_y_));
		}
		j++;
		glEnd();
	}
	
	// Set the clip region back to old
	r->SetClipRegion(old_clip_region);
	r->StartClip();// this must stay here to force a flush (and update the above)
	
	glPopAttrib();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	
	// reset matrices
	r->Begin();
	
    // return if we dont actually have any channels to plot so we dont draw a silly empty box
    if (channels_.size() == 0)
    {
        return;
    }

	// do whatever we want here
	Rect rr;
	rr.x = b.w - 200;
	rr.w = 150;
	rr.y = 100;
	rr.h = channels_.size()*20;
	r->SetDrawColor( Gwen::Color(0,0,0,255) );// start by clearing to background color
	r->DrawFilledRect( rr );
	rr.x += 2;
	rr.y += 2;
	rr.w -= 4;
	rr.h -= 4;
	r->SetDrawColor( Gwen::Color(255,255,255,255) );// start by clearing to background color
	r->DrawFilledRect( rr );
	
	int q = 0;
	for (auto& sub: channels_)
	{
		r->SetDrawColor(Gwen::Color(colors[q%6][0]*255,colors[q%6][1]*255,colors[q%6][2]*355,255));
		std::string str = sub->topic_name + "." + sub->field_name;
		r->RenderText(skin->GetDefaultFont(), Gwen::PointF( b.w - 195, 104 + q*20), str);
		q++;
	}*/
}
