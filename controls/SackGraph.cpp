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
}
