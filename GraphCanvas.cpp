// todo license

#include <Gwen/Platform.h>
#include <Gwen/Controls/TextBox.h>

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

static bool node_initialized = false;
static ps_node_t node;

static std::map<std::string, bool> _topics;


GWEN_CONTROL_CONSTRUCTOR( GraphCanvas )
{
	view_height_m_ = 150.0;
	m_Color = Gwen::Color( 255, 255, 255, 255 );
	
	// lets test some pubsub stuff here
	
	// testing, make some fake data
	/*data_.clear();
	
	for (int i = 0; i < 100; i++)
	{
		data_.push_back({ (double)i/10.0, sin(i*3.14159/10.0) });
	}*/
	
	subscribers_.push_back(new Subscriber());
	subscribers_.back()->canvas = this;
	
	topic_name_box_ = new Gwen::Controls::TextBox( this );
	topic_name_box_->SetText( "" );
	topic_name_box_->SetPos( 10, 10 );
	topic_name_box_->SetWidth(300);
	topic_name_box_->onTextChanged.Add( this, &ThisClass::OnTopicEdited );
	topic_name_box_->onReturnPressed.Add( this, &ThisClass::OnTopicChanged );
	topic_name_box_->onFocusLost.Add( this, &ThisClass::OnTopicEditFinished );
	
	field_name_ = new Gwen::Controls::TextBox( this );
	field_name_->SetText( "" );
	field_name_->SetPos( 320, 10 );
	field_name_->SetWidth(150);
	field_name_->onTextChanged.Add( this, &ThisClass::OnFieldChanged );
	//label->onReturnPressed.Add( this, &ThisClass::OnTopicChanged );
	
	auto add_button = new Gwen::Controls::Button( this );
	add_button->SetText("Add");
	add_button->SetPos(480, 10);
	add_button->SetWidth(70);
	add_button->onPress.Add(this, &ThisClass::OnAdd);
	
	topic_list_ = new Gwen::Controls::ListBox(this);
	topic_list_->AddItem("A");
	topic_list_->AddItem("B");
	topic_list_->AddItem("C");
	topic_list_->AddItem("D");
	topic_list_->AddItem("E");
	topic_list_->AddItem("F");
	topic_list_->AddItem("G");
	topic_list_->Hide();
	topic_list_->onRowSelected.Add( this, &ThisClass::OnSuggestionClicked );
	
	start_time_ = pubsub::Time::now();
	if (!node_initialized)
	{
		// first lets get a list of all topics, and then subscribe to them
		ps_node_init_ex(&node, "pubviz", "", false, false);

		ps_node_system_query(&node);

		node.adv_cb = [](const char* topic, const char* type, const char* node, const ps_advertise_req_t* data)
		{
			// todo check if we already have the topic
			if (_topics.find(topic) != _topics.end())
			{
				return;
			}

			printf("Discovered topic %s..\n", topic);
		
			_topics[topic] = true;
		};
		node_initialized = true;
	}
	
	ps_node_system_query(&node);
}

void GraphCanvas::OnAdd(Base* control)
{
	printf("Added new sub\n");
	// add a topic
	subscribers_.back()->field_name = field_name_->GetText().c_str();
	subscribers_.push_back(new Subscriber());
	subscribers_.back()->canvas = this;
	OnTopicChanged(topic_name_box_);
}

void GraphCanvas::OnSuggestionClicked(Base* control)
{
	topic_name_box_->SetText(((Gwen::Controls::ListBox*)control)->GetSelectedRowName());
	OnTopicChanged(topic_name_box_);
	topic_list_->Hide();
}

void GraphCanvas::OnTopicEditFinished(Base* control)
{
	topic_list_->Hide();
}

void GraphCanvas::OnFieldChanged(Base* control)
{
	subscribers_.back()->data.clear();
}

void GraphCanvas::OnTopicEdited(Base* control)
{
	auto p = control->GetPos();
	p.y += control->Height();
	topic_list_->SetPos(p);
	topic_list_->SetSize(100, 100);
	topic_list_->Show();
	
	topic_list_->Clear();
	if (_topics.size())
	{
		for (const auto& topic: _topics)
		{
			topic_list_->AddItem(topic.first, topic.first);
		}
	}
	else
	{
		topic_list_->AddItem("None found...");
	}
}

void GraphCanvas::OnTopicChanged(Base* control)
{
 	auto tb = (TextBox*)control;
	printf("%s\n", tb->GetText().c_str());
  
  	auto sub = subscribers_.back();
	if (sub->subscribed)
	{
		ps_sub_destroy(&sub->sub);
	}
	
	// store the topic name somewhere safe since the subscriber doesnt make a copy
	sub->topic_name = tb->GetText().c_str();
	sub->subscribed = true;
	
	sub->data.clear();
  
	// now create the subscriber
	struct ps_subscriber_options options;
	ps_subscriber_options_init(&options);
	options.skip = 0;
	options.queue_size = 0;
	options.want_message_def = true;
	options.allocator = 0;
	options.ignore_local = false;
	options.preferred_transport = false ? 1 : 0;
	options.cb_data = (void*)sub;
	options.cb = [](void* message, unsigned int size, void* data, const ps_msg_info_t* info)
	{
		// get and deserialize the messages
		Subscriber* sub = (Subscriber*)data;
		if (sub->sub.received_message_def.fields == 0)
		{
			//printf("WARN: got message but no message definition yet...\n");
			// queue it up, then print them out once I get it
			//todo_msgs.push_back({ message, *info });
		}
		else
		{
			//ps_deserialize_print(message, &sub.received_message_def);
			sub->canvas->HandleMessage(message, &sub->sub.received_message_def, sub);// fixme
			//printf("-------------\n");
			free(message);
		}
	};

	ps_node_create_subscriber_adv(&node, sub->topic_name.c_str(), 0, &sub->sub, &options);
}

void GraphCanvas::HandleMessage(const void* data, const ps_message_definition_t* definition, Subscriber* sub)
{
	std::string field_name = sub->field_name.length() ? sub->field_name : field_name_->GetText().c_str();
	
	pubsub::Time msg_time = pubsub::Time::now();
	
	struct ps_deserialize_iterator iter = ps_deserialize_start((const char*)data, definition);
	const struct ps_field_t* field; uint32_t length; const char* ptr;
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
				
				AddSample(value, msg_time, sub);
			}
			break;
		}
	}
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

void GraphCanvas::AddSample(double value, pubsub::Time time, Subscriber* sub)
{
	pubsub::Duration dt = time - start_time_;
	
	sub->data.push_back({dt.toSec(), value});
	
	// remove any old samples that no longer fit on screen
	while (sub->data.size() && sub->data.front().first < dt.toSec() - x_width_)
	{
		sub->data.pop_front();
	}
	
	// now update the graph start x and y
	min_x_ = dt.toSec() - x_width_;// todo use max here
	max_x_ = dt.toSec();
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

const static float colors[][3] = {{1,0,0},{0,1,0},{0,0,1}, {0,1,1}, {1,0,1}, {1,1,0}};

void GraphCanvas::Render( Skin::Base* skin )
{
	auto r = skin->GetRender();
	
	// do whatever we want here
	r->SetDrawColor( m_Color );// start by clearing to background color
	r->DrawFilledRect( GetRenderBounds() );
	
	r->SetDrawColor(Gwen::Color(0,0,0,255));

	ps_node_spin(&node);// todo move me
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
	Redraw();
        
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
	for (auto& sub: subscribers_)
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
		
	// do whatever we want here
	Rect rr;
	rr.x = b.w - 200;
	rr.w = 150;
	rr.y = 100;
	rr.h = subscribers_.size()*20;
	r->SetDrawColor( Gwen::Color(0,0,0,255) );// start by clearing to background color
	r->DrawFilledRect( rr );
	rr.x += 2;
	rr.y += 2;
	rr.w -= 4;
	rr.h -= 4;
	r->SetDrawColor( Gwen::Color(255,255,255,255) );// start by clearing to background color
	r->DrawFilledRect( rr );
	
	int q = 0;
	for (auto& sub: subscribers_)
	{
		r->SetDrawColor(Gwen::Color(colors[q%6][0]*255,colors[q%6][1]*255,colors[q%6][2]*355,255));
		std::string str = sub->topic_name + "." + sub->field_name;
		r->RenderText(skin->GetDefaultFont(), Gwen::PointF( b.w - 195, 104 + q*20), str);
		q++;
	}
}
