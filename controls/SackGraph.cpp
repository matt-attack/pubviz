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
  SetKeyboardInputEnabled( true );
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

	Focus();

    BaseClass::OnMouseClickLeft(x, y, down);
}

void SackGraph::OnMouseClickRight( int x, int y, bool down )
{
	if (is_2d_)
	{
		return;
	}

    selecting_ = down;

	auto start_x = GraphStartPosition();
    auto graph_width = GraphWidth();

    double x_rel = CanvasPosToLocal(Gwen::Point(x, y)).x;

	if (!selecting_)
	{
		if (selection_start_ == selection_end_)
		{
			min_x_ = 0.0;
			max_x_ = 0.0;

			for (auto& sub : channels_)
			{
				if (sub->data.size())
				{
					max_x_ = std::max(max_x_, sub->data.back().first);
				}
			}
		}
		else
		{
			min_x_ = std::min(selection_start_, selection_end_);
			max_x_ = std::max(selection_start_, selection_end_);
		}

		//update other viewers with our selection
		// todo only do this if we are synched, todo add option for that
		viewer_->UpdateSelection(min_x_, max_x_);
	}

	double rel_time = ((x_rel - start_x)/graph_width)*(max_x_ - min_x_) + min_x_;
	selection_start_ = rel_time;
    OnMouseMoved(x, y, 0, 0);
	Redraw();
}

void SackGraph::OnMouseMoved(int x, int y, int dx, int dy)
{
    // move playhead if we are currently mouse down
    if (mouse_down_ && !is_2d_)
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

	if (selecting_)
	{
        auto start_x = GraphStartPosition();
        auto graph_width = GraphWidth();

        x = CanvasPosToLocal(Gwen::Point(x, y)).x;

		double rel_time = ((x - start_x)/graph_width)*(max_x_ - min_x_) + min_x_;
        selection_end_ = rel_time;
		Redraw();
	}
}

bool SackGraph::OnKeyLeft( bool bDown )
{
	if (!bDown || GetChannels().size() == 0)
	{
		return false;
	}

	auto topic = GetChannels()[0]->topic_name;
	auto current_time = viewer_->GetPlayheadTime();
	auto target_time = current_time;

	// find the first message with a time less than this one
	auto& data = viewer_->GetTopicData(topic);
	for (int i = data.size() - 1; i >= 0; i--)
	{
		if (data[i].time < current_time)
		{
			target_time = data[i].time;
			break;
		}
	}

	viewer_->SetPlayheadTime(target_time);
	Redraw();
	return true;
}

bool SackGraph::OnKeyRight( bool bDown )
{
	if (!bDown || GetChannels().size() == 0)
	{
		return false;
	}

	auto topic = GetChannels()[0]->topic_name;
	auto current_time = viewer_->GetPlayheadTime();
	auto target_time = current_time;

	// find the first message with a time less than this one
	auto& data = viewer_->GetTopicData(topic);
	for (int i = 0; i < data.size(); i++)
	{
		if (data[i].time > current_time)
		{
			target_time = data[i].time;
			break;
		}
	}

	viewer_->SetPlayheadTime(target_time);
	Redraw();
	return true;
}

void SackGraph::SetViewer(SackViewer* viewer)
{
    viewer_ = viewer;
    max_x_ = (viewer->GetEndTime() - viewer->GetStartTime())/1000000.0;
    start_time_ = viewer->GetStartTime();
}

void SackGraph::PaintOnGraph(double start_x, double start_y, double graph_width, double graph_height)
{
	if (is_2d_)
	{
		return;// dont draw the values in 2d mode
	}

	auto r = GetSkin()->GetRender();
	double slider_time = (viewer_->GetPlayheadTime() - viewer_->GetStartTime())/1000000.0;
	pubsub::Time st(viewer_->GetPlayheadTime());
	char buffer[50];
	int num = 0;
	for (int j = 0; j < channels_.size(); j++)
	{
		auto channel = channels_[j];
		if (channel->hidden) { continue; }
		for (int i = channel->data.size() - 1; i >= 0; i--)
		{
			auto& pt = channel->data[i];
			if (pt.time <= st)
			{
				double x = start_x + graph_width*(slider_time - min_x_)/(max_x_ - min_x_);
				double y = start_y + num * 20;

				sprintf(buffer, "%lf", pt.second);
				r->SetDrawColor( Gwen::Color(graph_colors[j][0]*255,graph_colors[j][1]*255,graph_colors[j][2]*255,255) );
				r->RenderText(GetSkin()->GetDefaultFont(), Gwen::PointF(x + 15, y - 15), (std::string)buffer);
				num++;
				break;
			}
		}
	}
}

void SackGraph::DrawOnGraph(double start_x, double start_y, double graph_width, double graph_height)
{
	auto r = GetSkin()->GetRender();

    // convert timestamp to bag time
    double slider_time = (viewer_->GetPlayheadTime() - viewer_->GetStartTime())/1000000.0;
	pubsub::Time st(viewer_->GetPlayheadTime());
    // draw the playhead
	if (!is_2d_)
	{
		glLineWidth(4.0f);
		glBegin(GL_LINE_STRIP);
		glColor3f(1.0, 0.0, 0.0);
		double sx = start_x + graph_width * (slider_time/*position here*/ - min_x_) / (max_x_ - min_x_);
		glVertex2f(sx, start_y);
		glVertex2f(sx, start_y + graph_height);
		glEnd();
	}

	// draw the selection
	if (selecting_)
	{
		glEnable(GL_BLEND);
		glBegin(GL_TRIANGLES);
		glColor4f(1.0, 0.0, 0.0, 0.5);
 		double pt1x = start_x + graph_width*(selection_start_/*position here*/ - min_x_)/(max_x_ - min_x_);
		double pt2x = start_x + graph_width*(selection_end_/*position here*/ - min_x_)/(max_x_ - min_x_);
    	glVertex2f(pt1x, start_y);
    	glVertex2f(pt1x, start_y + graph_height);
		glVertex2f(pt2x, start_y + graph_height);

		glVertex2f(pt2x, start_y);
    	glVertex2f(pt1x, start_y);
		glVertex2f(pt2x, start_y + graph_height);

		glEnd();
		glDisable(GL_BLEND);
	}

	glEnable(GL_BLEND);
	// also draw the nearest value
	// todo use binary search
	glPointSize(20);
	glBegin(GL_POINTS);
    glColor4f(0.0, 1.0, 1.0, 0.5);
	for (auto channel: channels_)
	{
		if (channel->hidden) { continue; }
		for (int i = channel->data.size() - 1; i >= 0; i--)
		{
			auto& pt = channel->data[i];
			if (pt.time <= st)
			{
				double x = start_x + graph_width*(pt.first - min_x_)/(max_x_ - min_x_);
				double y = start_y + graph_height - graph_height*(pt.second - min_y_)/(max_y_ - min_y_);
				glVertex2f(x, y);
				break;
			}
		}
	}
	glEnd();
	glDisable(GL_BLEND);
}

void SackGraph::Render( Skin::Base* skin )
{
    GraphBase::Render(skin);
}

bool SackGraph::DragAndDrop_HandleDrop( Gwen::DragAndDrop::Package* pPackage, int x, int y )
{
	std::string topic = pPackage->drawcontrol->GetParent()->UserData.Get<std::string>("topic");
	std::string field = pPackage->drawcontrol->GetParent()->UserData.Get<std::string>("field");

	printf("dropped %s %s\n", topic.c_str(), field.c_str());

	// make sure its not a duplicate
	for (auto& ch: GetChannels())
	{
		if (ch->topic_name == topic && ch->field_name_y == field)
		{
			return false;
		}
	}

	auto ch = CreateChannel(topic, field);

	auto& data = viewer_->GetTopicData(topic);
	auto def = viewer_->GetTopicDefinition(topic);
	for (auto& msg: data)
	{
   		AddMessageSample(ch, msg.time, msg.msg, def, false, false);
	}

	return true;
}

