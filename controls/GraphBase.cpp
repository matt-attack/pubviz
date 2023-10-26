// todo license

#include <Gwen/Platform.h>
#include <Gwen/Controls/CheckBox.h>
#include <Gwen/Controls/ComboBox.h>
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


GWEN_CONTROL_CONSTRUCTOR( GraphBase )
{
	m_Color = Gwen::Color( 255, 255, 255, 255 );
	
	remove_button_ = new Gwen::Controls::Button( this );
	remove_button_->SetText("-");
	remove_button_->SetPos(520, 10);
    remove_button_->SetFont(L"", 20, false);
	remove_button_->SetWidth(30);
	remove_button_->onPress.Add(this, &ThisClass::OnRemove);

	configure_button_ = new Gwen::Controls::Button( this );
	configure_button_->SetText("*");
	configure_button_->SetPos(520, 40);
    configure_button_->SetFont(L"", 20, false);
	configure_button_->SetWidth(30);
	configure_button_->onPress.Add(this, &ThisClass::OnConfigure);
}

struct ConfigureDialog
{
	Gwen::Controls::TextBoxNumeric* left_, *right_, *bottom_, *top_;
	Gwen::Controls::CheckBox* autoscale_y_, *autoscale_x_;
	Gwen::Controls::ComboBox* style_;

	GraphBase* graph_;

	ConfigureDialog(GraphBase* graph)
	{
		graph_ = graph;
	}
};

struct ConfigureChannelDialog
{
	Gwen::Controls::TextBox* math_;
	GraphBase* graph_;
	GraphBase::Channel* channel;

	ConfigureChannelDialog(GraphBase* graph)
	{
		graph_ = graph;
	}
};

void GraphBase::OnMouseClickLeft( int x, int y, bool down )
{
	// toggle hidden when you click on a key label
	if (down)
	{
		Gwen::Point lp = CanvasPosToLocal(Gwen::Point(x, y));
		//		key.x = b.w - key_width + 5 - 50;
		//key.y = 104 + q*20 + 3;
		int c = lp.y - 104 - 3;
		c /= 20;
		if (c >= 0 && c < GetChannels().size())
		{
			auto b = GetRenderBounds();
			int key_start = b.w - key_width_ - 50;
			if (lp.x > key_start && lp.x < key_start + key_width_)
			{
				GetChannels()[c]->hidden = !GetChannels()[c]->hidden;
				Redraw();
			}
		}
	}
}

void GraphBase::OnMouseClickRight( int x, int y, bool bDown )
{
	DoMouseClickRight(x, y, bDown);
}

bool GraphBase::DoMouseClickRight( int x, int y, bool bDown )
{
	if (bDown)
	{
		// find the channel
		auto pos = CanvasPosToLocal(Gwen::Point(x,y));
		if (pos.x > key_rect_.x && pos.y > key_rect_.y &&
			pos.x < key_rect_.x + key_rect_.w)
		{
			int index = (pos.y-key_rect_.y+4)/20;
			if (index < channels_.size())
			{
				OnConfigureChannel(channels_[index]);
				return true;
			}
		}
	}
	return false;
}


void GraphBase::OnConfigureChannel(Channel* channel)
{
	// try and figure out which channel
	Controls::WindowControl* pWindow = new Controls::WindowControl( GetCanvas() );
	pWindow->SetTitle( "Configure Channel " + channel->GetTitle());
	pWindow->SetSize( 300, 120 );
	pWindow->MakeModal( true );
	//pWindow->Position( Pos::Center );// doesnt work if we have no inner space left
    auto pos = GetCanvas()->GetRenderBounds();
    pWindow->SetPos(Gwen::Point(pos.w/2 - 100, pos.h/2 - 100));
	pWindow->SetDeleteOnClose( true );
    pWindow->DisableResizing();

	auto dialog = new ConfigureChannelDialog(this);
	dialog->channel = channel;
	auto button = new Gwen::Controls::Button( pWindow );
	button->SetText("Apply");
	button->SetPos( 70, 30 + (is_2d_ ? 0 : 25));
	button->SetWidth(70);
	button->onPress.Add(this, &ThisClass::OnConfigureChannelClosed, dialog);

	int current_y = 10;
	if (!is_2d_)
    {
		Gwen::Controls::Label* label = new Gwen::Controls::Label( pWindow );
		label->SetText( "Math" );
		label->SizeToContents();
		label->SetPos( 10, current_y);
        Gwen::Controls::TextBox* box = new Gwen::Controls::TextBox( pWindow );
		box->SetText( channel->math_string_ );// todo get from channel
		box->SetPos( 70, current_y);
        box->SetWidth(200);
		dialog->math_ = box;
		current_y += 25;
    }
}

void GraphBase::OnConfigureChannelClosed(Gwen::Event::Info info)
{
	auto dialog = (ConfigureChannelDialog*)info.Data;
	dialog->channel->SetMath(dialog->math_->GetText().c_str());
}

void GraphBase::OnConfigure(Base* control)
{
	Controls::WindowControl* pWindow = new Controls::WindowControl( GetCanvas() );
	pWindow->SetTitle( L"Configure Graph" );
	pWindow->SetSize( 200, 275 + (is_2d_ ? 25 : 0) );
	pWindow->MakeModal( true );
	//pWindow->Position( Pos::Center );// doesnt work if we have no inner space left
    auto pos = GetCanvas()->GetRenderBounds();
    pWindow->SetPos(Gwen::Point(pos.w/2 - 100, pos.h/2 - 100));
	pWindow->SetDeleteOnClose( true );
    pWindow->DisableResizing();

	auto dialog = new ConfigureDialog(this);

	auto button = new Gwen::Controls::Button( pWindow );
	button->SetText("Apply");
	button->SetPos( 70, 20 + 25*8 + (is_2d_ ? 25 : 0));
	button->SetWidth(70);
	button->onPress.Add(this, &ThisClass::OnConfigureClosed, dialog);

    // add settings
    {
        Gwen::Controls::Label* label = new Gwen::Controls::Label( pWindow );
		label->SetText( "X-Axis" );
		label->SizeToContents();
		label->SetPos( 10, 10 );
    }
	int current_x = 10 + 25;
	if (is_2d_)
	{
		Gwen::Controls::Label* label = new Gwen::Controls::Label(pWindow);
		label->SetText("Auto Scale");
		label->SizeToContents();
		label->SetPos(20, current_x);
		Gwen::Controls::CheckBox* check = new Gwen::Controls::CheckBox(pWindow);
		check->SetPos(110, current_x);
		check->SetChecked(autoscale_x_);
		dialog->autoscale_x_ = check;
		current_x += 25;
	}
    {
		Gwen::Controls::Label* label = new Gwen::Controls::Label( pWindow );
		label->SetText( "Left" );
		label->SizeToContents();
		label->SetPos( 20, current_x);
        Gwen::Controls::TextBoxNumeric* box = new Gwen::Controls::TextBoxNumeric( pWindow );
		box->SetText( std::to_string(min_x_) );
		box->SetPos( 110, current_x);
        box->SetWidth(70);
		dialog->left_ = box;
		current_x += 25;
    }
    {
		Gwen::Controls::Label* label = new Gwen::Controls::Label( pWindow );
		label->SetText( "Right" );
		label->SizeToContents();
		label->SetPos( 20, current_x);
        Gwen::Controls::TextBoxNumeric* box = new Gwen::Controls::TextBoxNumeric( pWindow );
		box->SetText( std::to_string(max_x_) );
		box->SetPos( 110, current_x);
        box->SetWidth(70);
		dialog->right_ = box;
		current_x += 25;
    }


	current_x += 10;
    {
        Gwen::Controls::Label* label = new Gwen::Controls::Label( pWindow );
		label->SetText( "Y-Axis" );
		label->SizeToContents();
		label->SetPos( 10, current_x);
		current_x += 25;
    }
    {
		Gwen::Controls::Label* label = new Gwen::Controls::Label( pWindow );
		label->SetText( "Auto Scale" );
		label->SizeToContents();
		label->SetPos( 20, current_x);
		Gwen::Controls::CheckBox* check = new Gwen::Controls::CheckBox( pWindow );
		check->SetPos( 110, current_x);
        check->SetChecked(autoscale_y_);
		dialog->autoscale_y_ = check;

		Gwen::Controls::Button* button = new Gwen::Controls::Button( pWindow );
		button->SetText( "Reset" );
		button->SizeToContents();
		button->SetPos( 135, current_x );
		button->onPress.Add(this, &ThisClass::OnResetYScale, dialog);

		current_x += 25;
    }
    {
		Gwen::Controls::Label* label = new Gwen::Controls::Label( pWindow );
		label->SetText( "Bottom" );
		label->SizeToContents();
		label->SetPos( 20, current_x);
        Gwen::Controls::TextBoxNumeric* box = new Gwen::Controls::TextBoxNumeric( pWindow );
		box->SetText( std::to_string(min_y_) );
		box->SetPos( 110, current_x);
        box->SetWidth(70);
		dialog->bottom_ = box;
		current_x += 25;
    }
    {
		Gwen::Controls::Label* label = new Gwen::Controls::Label( pWindow );
		label->SetText( "Top" );
		label->SizeToContents();
		label->SetPos( 20, current_x);
        Gwen::Controls::TextBoxNumeric* box = new Gwen::Controls::TextBoxNumeric( pWindow );
		box->SetText( std::to_string(max_y_) );
		box->SetPos( 110, current_x );
        box->SetWidth(70);
		dialog->top_ = box;
		current_x += 25;
    }

	{
		Gwen::Controls::Label* label = new Gwen::Controls::Label( pWindow );
		label->SetText( "Line Style" );
		label->SizeToContents();
		label->SetPos( 10, current_x );
		auto box = new Gwen::Controls::ComboBox( pWindow );
		box->SetPos( 110, current_x );
        box->SetWidth(70);
		box->AddItem(L"Line", "Line");
		box->AddItem(L"Dots", "Dots");
		box->AddItem(L"Both", "Both");
		box->SelectItemByName(style_);
		current_x += 25;
		dialog->style_ = box;
	}
}

void GraphBase::OnResetYScale(Gwen::Event::Info info)
{
	redo_scale_ = true;
	UpdateScales();
	Redraw();

	auto dialog = (ConfigureDialog*)info.Data;
	dialog->bottom_->SetText(std::to_string(min_y_));
	dialog->top_->SetText(std::to_string(max_y_));
}

void GraphBase::OnConfigureClosed(Gwen::Event::Info info)
{
	auto dialog = (ConfigureDialog*)info.Data;
	autoscale_y_ = dialog->autoscale_y_->IsChecked();
	min_y_ = dialog->bottom_->GetFloatFromText();
	max_y_ = dialog->top_->GetFloatFromText();

	min_x_ = dialog->left_->GetFloatFromText();
	max_x_ = dialog->right_->GetFloatFromText();

	style_ = dialog->style_->GetSelectedItem()->GetText().c_str();

	UpdateScales();

	dialog->bottom_->SetText(std::to_string(min_y_));
	dialog->top_->SetText(std::to_string(max_y_));
}

GraphBase::Channel* GraphBase::CreateChannel(const std::string& topic, const std::string& field_x, const std::string& field_y)
{
	is_2d_ = true;
	auto ch = new GraphBase::Channel();
	ch->topic_name = topic;
	ch->field_name_x = field_x;
	ch->field_name_y = field_y;
	channels_.push_back(ch);
	return ch;
}

GraphBase::Channel* GraphBase::CreateChannel(const std::string& topic, const std::string& field)
{
    auto ch = new GraphBase::Channel();
    ch->topic_name = topic;
    ch->field_name_y = field;
    channels_.push_back(ch);
    return ch;
}

void GraphBase::Layout(Gwen::Skin::Base* skin)
{   
    // handle 
    auto width = Width();
	remove_button_->SetPos(width - 40, 10);
	configure_button_->SetPos(width - 40, 40);

	BaseClass::Layout(skin);
}

void GraphBase::OnRemove(Base* control)
{
    // todo disable button if no channels
    if (channels_.size() == 0)
    {
        return;
    }

    auto menu = new Gwen::Controls::Menu(this);
    for (int i = 0; i < channels_.size(); i++)
    {
        auto sub = channels_[i];
        if (sub->can_remove)
        {
            menu->AddItem(sub->GetTitle())->SetAction(this, &ThisClass::OnRemoveSelect);
        }
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
        auto channel = channels_[i];
		std::string current_name = channel->GetTitle();
        //printf("%s vs %s\n", name.c_str(), current_name.c_str());
        if (name == current_name && channel->can_remove)
        {
            if (channel->on_remove)
            {
                channel->on_remove();
            }
            delete channel;
            channels_.erase(channels_.begin() + i);
            break;
        }
    }
}

void GraphBase::AddMessageSample(Channel* channel, pubsub::Time msg_time, const void* message, const ps_message_definition_t* definition, bool scroll_to_fit, bool remove_old)
{
	const std::string& field_name = channel->field_name_y;

	double x = std::numeric_limits<double>::quiet_NaN();
	double y = std::numeric_limits<double>::quiet_NaN();

	const char* x_name = channel->field_name_x.length() ? channel->field_name_x.c_str() : 0;
	const char* y_name = channel->field_name_y.c_str();
	
	struct ps_deserialize_iterator iter = ps_deserialize_start((const char*)message, definition);
	const struct ps_msg_field_t* field; uint32_t length; const char* ptr;
	while (ptr = ps_deserialize_iterate(&iter, &field, &length))
	{
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
				
				if (x_name && strcmp(x_name, field->name) == 0)
				{
					x = value;
				}
				else if (strcmp(y_name, field->name) == 0)
				{
					y = value;
				}
			}
		}
	}
	
	if (!x_name && !std::isnan(y))
	{
		// 1d
		AddSample(channel, y, msg_time, scroll_to_fit, remove_old);
	}
	else if (x_name && !std::isnan(x) && !std::isnan(y))
	{
		// 2d
		channel->data.push_back({ x, y, msg_time, 0 });

		Redraw();
	}
}

bool GraphBase::OnMouseWheeled( int delta )
{
	return true;
}

void GraphBase::AddSample(Channel* sub, double value, pubsub::Time time, bool scroll_to_fit, bool remove_old)
{
	pubsub::Duration dt = time - start_time_;
	
	sub->data.push_back({dt.toSec(), sub->Evaluate(value), time, value});

    //if resize to fit, just make graph wider to show it
    if (scroll_to_fit)
    {
        double width = max_x_ - min_x_;
        max_x_ = std::max(dt.toSec(), max_x_);
        min_x_ = max_x_ - width;
    }
	
	// remove any old samples that no longer fit on screen
    if (remove_old)
    {
	    while (sub->data.size() && sub->data.front().first < min_x_)
	    {
		    sub->data.pop_front();
	    }
    }

    Redraw();
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

const float graph_colors[6][3] = {{1,0,0},{0,1,0},{0,0,1}, {0,1,1}, {1,0,1}, {0.7,0.7,0}};

void CalculateDivisions(std::vector<double>& divisions, double min, double max, int max_divisions)
{
    // now determine the period to use
    // for this we want enough divisions to fit and to minimize significant figures
    //printf("Max divs: %i\n", max_divisions);
    double period = CalcStepSize(max - min, std::max(max_divisions, 1));
    if (period <= 0)
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

void GraphBase::UpdateScales()
{
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
	if (is_2d_ && autoscale_x_)
	{
		bool data_point = false;
		double current_min_x = std::numeric_limits<double>::max();
		double current_max_x = -std::numeric_limits<double>::max();
		for (auto& sub : channels_)
		{
			for (auto& pt : sub->data)
			{
				current_min_x = std::min(current_min_x, pt.first);
				current_max_x = std::max(current_max_x, pt.first);
				data_point = true;
			}
		}

		if (redo_scale_ && data_point)
		{
			redo_scale_ = false;
			min_x_ = std::numeric_limits<double>::max();
			max_x_ = -std::numeric_limits<double>::max();
		}

		if (redo_scale_)
		{
			min_x_ = -1.0;
			max_x_ = 1.0;
		}
		else
		{
			// add a bit of a buffer on each edge (5% of difference)
			const double difference = std::abs(current_min_x - current_max_x);
			double buffer = difference * 0.05;
			current_max_x += buffer;
			current_min_x -= buffer;

			max_x_ = current_max_x;// std::max(max_x_, current_max_x);
			min_x_ = current_min_x;// std::min(min_x_, current_min_x);
		}

		// handle max and min being identical resulting in bad graphs
		if (std::abs(min_x_ - max_x_) < 0.0001)
		{
			max_x_ += 0.0001;
			min_x_ -= 0.0001;
		}
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
	UpdateScales();

    // now determine the period to use
    // for this we want enough divisions to fit and to minimize significant figures
	int max_divs = graph_width/pixel_interval;
	
	// Determine where to put labels on the x axis using the best fitting period
	std::vector<double> x_labels;
    CalculateDivisions(x_labels, min_x_, max_x_, max_divs);

    // Do the same for the y axis
    std::vector<double> y_labels;
    CalculateDivisions(y_labels, min_y_, max_y_, graph_height/pixel_interval);
	
	const double x_cell_size = graph_width/x_count;
	const double y_cell_size = graph_height/y_count;
		
	const double start_x = left_padding;
	const double start_y = other_padding;
	
	const double x_interval = (max_x_ - min_x_)/x_count;
	const double y_interval = (max_y_ - min_y_)/y_count;
	
	// lets make a grid for the graph
	char buffer[50];
	
    double x_ppu = graph_width/(max_x_ - min_x_); 
    for (auto label: x_labels)
    {
		double val = label;//min_x_ + (i++)*x_interval;
        double x = x_ppu*(label - min_x_) + start_x;
		if (std::abs(val) > 1.0 || val == 0.0)
		{
			sprintf(buffer, "%lg", val);
		}
		else
		{
			sprintf(buffer, "%lg", val);
		}
		r->RenderText(skin->GetDefaultFont(), Gwen::PointF( x, b.h - 30 ), (std::string)buffer);
    }
    double y_ppu = graph_height/(max_y_ - min_y_); 
    for (auto label: y_labels)
    {
		double val = label;
        double y = y_ppu*(max_y_ - label) + start_y;
		if (std::abs(val) > 1.0 || val == 0.0)
		{
			sprintf(buffer, "%lg", val);
		}
		else
		{
			sprintf(buffer, "%lg", val);
		}
		r->RenderText(skin->GetDefaultFont(), Gwen::PointF( 10, y - 7 ), (std::string)buffer);
    }
	
	// force a flush essentially
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
	Gwen::Point pos = r->GetRenderOffset();
	glLoadIdentity();
	//glRotatef(yaw, 0, 0, 1);
	auto scale = GetCanvas()->Scale();
	glTranslatef(pos.x*scale, pos.y*scale, 0);// shift it so 0, 0 is _our_ top left corner
	glScalef(scale, scale, 1.0);
	
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
    for (auto value: y_labels)
    {
        double y = y_ppu*(max_y_ - value)+start_y;
        glVertex2f(start_x - 10, y);
		glVertex2f(start_x + graph_width, y);
    }
	glEnd();
	
	// Set a clip region around the graph to crop anything out of range
	Gwen::Rect old_clip_region = r->ClipRegion();
	Gwen::Point start_pos = LocalPosToCanvas(Gwen::Point(start_x, start_y));
	r->SetClipRegion(Gwen::Rect(start_pos.x, start_pos.y, graph_width, graph_height));
	r->StartClip();
	
	// Draw the graph line (if enabled)
	bool draw_dots = style_ == "Dots";
	bool draw_lines = style_ == "Line";
	if (style_ == "Both")
	{
		draw_dots = true;
		draw_lines = true;
	}

	if (draw_lines)
	{
		glLineWidth(4.0f);
		for (int j = 0; j < channels_.size(); j++)
		{
			auto sub = channels_[j];
			if (sub->hidden) { continue; }
			glBegin(GL_LINE_STRIP);
			glColor3f(graph_colors[j%6][0], graph_colors[j%6][1], graph_colors[j%6][2]);
			for (auto& pt: sub->data)
			{
				glVertex2f(start_x + graph_width*(pt.first - min_x_)/(max_x_ - min_x_),
		           start_y + y_count*y_cell_size - graph_height*(pt.second - min_y_)/(max_y_ - min_y_));
			}
			glEnd();
		}
	}

	// Draw graph dots (if enabled)
	if (draw_dots)
	{
		glPointSize(8.0f);
		glBegin(GL_POINTS);
		for (int j = 0; j < channels_.size(); j++)
		{
			auto sub = channels_[j];
			if (sub->hidden) { continue; }
			glColor3f(graph_colors[j%6][0], graph_colors[j%6][1], graph_colors[j%6][2]);
			for (auto& pt: sub->data)
			{
				glVertex2f(start_x + graph_width*(pt.first - min_x_)/(max_x_ - min_x_),
		           start_y + y_count*y_cell_size - graph_height*(pt.second - min_y_)/(max_y_ - min_y_));
			}
		}
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

	PaintOnGraph(start_x, start_y, graph_width, graph_height);
	
    // return if we dont actually have any channels to plot so we dont draw a silly empty box
    if (channels_.size() == 0)
    {
        return;
    }

	// guess how wide to make the key box, so we dont make it wider than necessary
	int key_width = 70;
	for (auto& sub: channels_)
	{
		int len = sub->GetTitle().length();
		key_width = std::max<int>(len*8 + 25, key_width);
	}
	key_width_ = key_width;

	// do whatever we want here
	Rect rr;
	rr.x = b.w - key_width - 50;
	rr.w = key_width;
	rr.y = 100;
	rr.h = channels_.size()*20 + 3;
	r->SetDrawColor( Gwen::Color(0,0,0,255) );// start by clearing to background color
	r->DrawFilledRect( rr );
	key_rect_ = rr;
	rr.x += 2;
	rr.y += 2;
	rr.w -= 4;
	rr.h -= 4;
	r->SetDrawColor( Gwen::Color(255,255,255,255) );// start by clearing to background color
	r->DrawFilledRect( rr );
	
	int q = 0;
	for (auto& sub: channels_)
	{
		r->SetDrawColor(Gwen::Color(graph_colors[q%6][0]*255,graph_colors[q%6][1]*255,graph_colors[q%6][2]*255,255));
		Rect key;
		key.w = 10;
		key.h = 10;
		key.x = b.w - key_width + 5 - 50;
		key.y = 104 + q*20 + 3;
		r->DrawFilledRect(key);
		if (sub->hidden)
		{
			r->SetDrawColor(Gwen::Color(100, 100, 100));
		}
		else
		{
			r->SetDrawColor(Gwen::Color(0, 0, 0));
		}
		r->RenderText(skin->GetDefaultFont(), Gwen::PointF( 15 + b.w - key_width + 5 - 50, 104 + q*20), sub->GetTitle());
		q++;
	}
}
