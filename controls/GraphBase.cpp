// todo license

#include <Gwen/Platform.h>
#include <Gwen/Controls/CheckBox.h>
#include <Gwen/Controls/Menu.h>
#include <Gwen/Controls/TextBox.h>
#include <Gwen/Controls/WindowControl.h>

#include "GraphBase.h"

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


GWEN_CONTROL_CONSTRUCTOR( GraphBase ) , remove_button_(this), configure_button_(this)
{
	m_Color = Gwen::Color( 255, 255, 255, 255 );
	
	auto remove_button = &remove_button_;//new Gwen::Controls::Button( this );
	remove_button->SetText("-");
	remove_button->SetPos(520, 10);
    remove_button->SetFont(L"", 20, false);
	remove_button->SetWidth(30);
	remove_button->onPress.Add(this, &ThisClass::OnRemove);

	auto configure_button = &configure_button_;//new Gwen::Controls::Button( this );
	configure_button->SetText("*");
	configure_button->SetPos(520, 40);
    configure_button->SetFont(L"", 20, false);
	configure_button->SetWidth(30);
	configure_button->onPress.Add(this, &ThisClass::OnConfigure);
}

void GraphBase::OnConfigure(Base* control)
{
	Controls::WindowControl* pWindow = new Controls::WindowControl( GetCanvas() );
	pWindow->SetTitle( L"Configure Graph" );
	pWindow->SetSize( 200, 250 );
	pWindow->MakeModal( true );
	//pWindow->Position( Pos::Center );// doesnt work if we have no inner space left
    auto pos = GetCanvas()->GetRenderBounds();
    pWindow->SetPos(Gwen::Point(pos.w/2 - 100, pos.h/2 - 100));
	pWindow->SetDeleteOnClose( true );
    pWindow->DisableResizing();

    // add settings
    // todo fill out
    {
        Gwen::Controls::Label* label = new Gwen::Controls::Label( pWindow );
		label->SetText( "X-Axis" );
		label->SizeToContents();
		label->SetPos( 10, 10 );
    }
    {
		Gwen::Controls::Label* label = new Gwen::Controls::Label( pWindow );
		label->SetText( "Left" );
		label->SizeToContents();
		label->SetPos( 20, 10 + 25 );
        Gwen::Controls::TextBoxNumeric* box = new Gwen::Controls::TextBoxNumeric( pWindow );
		box->SetText( "0.0" );
		box->SetPos( 110, 10 + 25 );
        box->SetWidth(70);
    }
    {
		Gwen::Controls::Label* label = new Gwen::Controls::Label( pWindow );
		label->SetText( "Right" );
		label->SizeToContents();
		label->SetPos( 20, 10 + 25*2 );
        Gwen::Controls::TextBoxNumeric* box = new Gwen::Controls::TextBoxNumeric( pWindow );
		box->SetText( "10.0" );
		box->SetPos( 110, 10 + 25*2 );
        box->SetWidth(70);
    }


    {
        Gwen::Controls::Label* label = new Gwen::Controls::Label( pWindow );
		label->SetText( "Y-Axis" );
		label->SizeToContents();
		label->SetPos( 10, 20 + 25*3 );
    }
    {
		Gwen::Controls::Label* label = new Gwen::Controls::Label( pWindow );
		label->SetText( "Auto Scale" );
		label->SizeToContents();
		label->SetPos( 20, 20 + 25*4 );
		Gwen::Controls::CheckBox* check = new Gwen::Controls::CheckBox( pWindow );
		check->SetPos( 110, 20 + 25*4 );
        check->SetChecked(true);
		//check->onChecked.Add( this, &Checkbox::OnChecked );
		//check->onUnChecked.Add( this, &Checkbox::OnUnchecked );
		//check->onCheckChanged.Add( this, &Checkbox::OnCheckChanged );
    }
    {
		Gwen::Controls::Label* label = new Gwen::Controls::Label( pWindow );
		label->SetText( "Bottom" );
		label->SizeToContents();
		label->SetPos( 20, 20 + 25*5 );
        Gwen::Controls::TextBoxNumeric* box = new Gwen::Controls::TextBoxNumeric( pWindow );
		box->SetText( std::to_string(min_y_) );
		box->SetPos( 110, 20 + 25*5 );
        box->SetWidth(70);
    }
    {
		Gwen::Controls::Label* label = new Gwen::Controls::Label( pWindow );
		label->SetText( "Top" );
		label->SizeToContents();
		label->SetPos( 20, 20 + 25*6 );
        Gwen::Controls::TextBoxNumeric* box = new Gwen::Controls::TextBoxNumeric( pWindow );
		box->SetText( std::to_string(max_y_) );
		box->SetPos( 110, 20 + 25*6 );
        box->SetWidth(70);
    }

}

GraphBase::Channel* GraphBase::GetChannel(const std::string& topic, const std::string& field)
{
    for (const auto& channel: channels_)
    {
        if (channel->topic_name == topic && channel->field_name == field)
        {
            return channel;
        }
    }

    // add it!
    auto ch = new GraphBase::Channel();
    ch->topic_name = topic;
    ch->field_name = field;
    channels_.push_back(ch);
    return ch;
}

void GraphBase::Layout(Gwen::Skin::Base* skin)
{   
    // handle 
    auto width = Width();
	remove_button_.SetPos(width - 40, 10);
	configure_button_.SetPos(width - 40, 40);

	BaseClass::Layout(skin);
}

void GraphBase::OnRemove(Base* control)
{
    // todo show menu with list of current topics
    auto menu = new Gwen::Controls::Menu(this);
    for (int i = 0; i < channels_.size(); i++)
    {
        auto sub = channels_[i];
        menu->AddItem(sub->topic_name + "." + sub->field_name)->SetAction(this, &ThisClass::OnRemoveSelect);
    }
	menu->AddDivider();

	auto p = control->GetPos();
	p.y += control->Height();
	menu->SetPos(p);
	//menu->SetSize(100, 200);
	menu->Show();
}

void GraphBase::OnRemoveSelect(Gwen::Controls::Base* pControl)
{
	Gwen::Controls::MenuItem* pMenuItem = (Gwen::Controls::MenuItem*) pControl;
    auto name = pMenuItem->GetText();

    for (int i = 0; i < channels_.size(); i++)
    {
        std::string current_name = channels_[i]->topic_name + "." + channels_[i]->field_name;
        printf("%s vs %s\n", name.c_str(), current_name.c_str());
        if (name == current_name)
        {
            channels_.erase(channels_.begin() + i);
            break;
        }
    }
}

void GraphBase::AddMessageSample(Channel* channel, pubsub::Time msg_time, const void* message, const ps_message_definition_t* definition, double max_graph_width)
{
	std::string field_name = channel->field_name;
	
	struct ps_deserialize_iterator iter = ps_deserialize_start((const char*)message, definition);
	const struct ps_msg_field_t* field; uint32_t length; const char* ptr;
	while (ptr = ps_deserialize_iterate(&iter, &field, &length))
	{
		if (field_name.length() && strcmp(field_name.c_str(), field->name) != 0)
		{
			continue;
		}
		
		if (field->type == FT_String)
		{
			// strings are already null terminated
			//printf("%s: %s\n", field->name, ptr);
			// what should I do about strings?
		}
		else
		{
			if (length > 0)
			{
				double value = 0;
				// non dynamic types 
				switch (field->type)
				{
				case FT_Int8:
					value = *(int8_t*)ptr;
					break;
				case FT_Int16:
					value = *(int16_t*)ptr;
					break;
				case FT_Int32:
					value = *(int32_t*)ptr;
					break;
				case FT_Int64:
					value = *(int64_t*)ptr;
					break;
				case FT_UInt8:
					value = *(uint8_t*)ptr;
					break;
				case FT_UInt16:
					value = *(uint16_t*)ptr;
					break;
				case FT_UInt32:
					value = *(uint32_t*)ptr;
					break;
				case FT_UInt64:
					value = *(uint64_t*)ptr;
					break;
				case FT_Float32:
					value = *(float*)ptr;
					break;
				case FT_Float64:
					value = *(double*)ptr;
					break;
				default:
					printf("ERROR: unhandled field type when parsing....\n");
				}
				
				AddSample(channel, value, msg_time, max_graph_width);
                break;
			}
		}
	}
}

bool GraphBase::OnMouseWheeled( int delta )
{
	return true;
}

void GraphBase::AddSample(Channel* sub, double value, pubsub::Time time, double max_graph_width)
{
	pubsub::Duration dt = time - start_time_;
	
	sub->data.push_back({dt.toSec(), value});
	
	// remove any old samples that no longer fit on screen
	while (sub->data.size() && sub->data.front().first < dt.toSec() - max_graph_width)
	{
		sub->data.pop_front();
	}
	
	// now update the graph start x and y
	min_x_ = std::max(dt.toSec() - max_graph_width, min_x_);// todo use max here
	max_x_ = std::max(dt.toSec(), max_x_);

    Redraw();
}

void GraphBase::OnMouseClickLeft( int /*x*/, int /*y*/, bool down )
{

}

void GraphBase::OnMouseMoved(int x, int y, int dx, int dy)
{

}

template<typename T>
T roundMultiple( T value, T multiple )
{
    if (multiple == 0) return value;
    return static_cast<T>(std::round(static_cast<double>(value)/static_cast<double>(multiple))*static_cast<double>(multiple));
}

double CalcStepSize(double range, double targetSteps)
{
    // calculate an initial guess at step size
    auto tempStep = range/targetSteps;

    // get the magnitude of the step size
    auto mag = (float)std::floor(std::log10(tempStep));
    auto magPow = (float)std::pow(10, mag);

    // calculate most significant digit of the new step size
    auto magMsd = (int)(tempStep/magPow + 0.5);

    // promote the MSD to either 1, 2, or 5
    if (magMsd > 5)
        magMsd = 10;
    else if (magMsd > 2)
        magMsd = 5;
    else if (magMsd > 1)
        magMsd = 2;

    return magMsd*magPow;
}

const static float colors[][3] = {{1,0,0},{0,1,0},{0,0,1}, {0,1,1}, {1,0,1}, {1,1,0}};

void CalculateDivisions(std::vector<double>& divisions, double min, double max, int max_divisions)
{
    // now determine the period to use
    // for this we want enough divisions to fit and to minimize significant figures
	//int max_divs = graph_width/pixel_interval;
    //printf("Max divs: %i\n", max_divisions);
    double period = CalcStepSize(max - min, std::max(max_divisions, 1));
    if (period < 0)
    {
        period = 1 + max - min;
    }
    //printf("Target step: %f\n", period);
	
	// Determine where to put labels on the x axis using the best fitting period
	double time = roundMultiple(min, period);
    //printf("Min: %f Nearest: %f\n", min, time);
    if (time < min)
    {
        time += period;
    }
	while (time <= max)
	{
        divisions.push_back(time);
        //printf("Label: %f\n", time);
		time += period;
	}
    // make sure we at least show 2 labels
    if (divisions.size() == 1)
    {
        divisions.clear();
        divisions.push_back(min);
        divisions.push_back(max);
    }
}

void GraphBase::Render( Skin::Base* skin )
{
	auto r = skin->GetRender();
	
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

    // now determine the period to use
    // for this we want enough divisions to fit and to minimize significant figures
	int max_divs = graph_width/pixel_interval;
	
	// Determine where to put labels on the x axis using the best fitting period
	std::vector<double> x_labels;
    CalculateDivisions(x_labels, min_x_, max_x_, max_divs);
	
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
    double x_ppu = graph_width/(max_x_ - min_x_); 
    for (auto label: x_labels)
    {
		double val = label;//min_x_ + (i++)*x_interval;
        double x = x_ppu*(label - min_x_) + start_x;
		if (std::abs(val) > 1.0 || val == 0.0)
		{
			sprintf(buffer, "%lf", val);
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
			sprintf(buffer, "%lf", val);
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
    for (auto value: x_labels)
    {
        double x = x_ppu*(value - min_x_)+start_x;
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

    // Draw stuff from any child classes
    DrawOnGraph(start_x, start_y, graph_width, graph_height);
	
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
	}
}
