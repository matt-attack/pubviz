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

#include <pubsub_cpp/Node.h>

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
	
	Gwen::Controls::TextBox* label = new Gwen::Controls::TextBox( this );
	label->SetText( "" );
	label->SetPos( 10, 10 );
	label->SetWidth(300);
	//label->onTextChanged.Add( this, &ThisClass::OnEdit );
	label->onReturnPressed.Add( this, &ThisClass::OnTopicChanged );
	
	field_name_ = new Gwen::Controls::TextBox( this );
	field_name_->SetText( "" );
	field_name_->SetPos( 320, 10 );
	field_name_->SetWidth(150);
	//field->onTextChanged.Add( this, &ThisClass::OnFieldChanged );
	//label->onReturnPressed.Add( this, &ThisClass::OnTopicChanged );
	
	start_time_ = pubsub::Time::now();
	if (!node_initialized)
	{
		// first lets get a list of all topics, and then subscribe to them
		ps_node_init(&node, "pubviz", "", false);

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
			/*TopicData tdata;
			tdata.topic = topic;
			tdata.type = type;
			tdata.hash = data->type_hash;
			_topics[topic] = tdata;*/
		};
		node_initialized = true;
	}
	
	
	ps_node_system_query(&node);
}

static bool sub_created = false;
static ps_sub_t sub;
static std::string topic_name;
void GraphCanvas::OnTopicChanged(Base* control)
{
 	auto tb = (TextBox*)control;
	printf("%s\n", tb->GetText().c_str());
  
	if (sub_created)
	{
		ps_sub_destroy(&sub);
	}
	
	// store the topic name somewhere safe since the subscriber doesnt make a copy
	topic_name = tb->GetText().c_str();
	sub_created = true;
  
	// now create the subscriber
	struct ps_subscriber_options options;
	ps_subscriber_options_init(&options);
	options.skip = 0;
	options.queue_size = 0;
	options.want_message_def = true;
	options.allocator = 0;
	options.ignore_local = false;
	options.preferred_transport = false ? 1 : 0;
	options.cb_data = (void*)this;
	options.cb = [](void* message, unsigned int size, void* data, const ps_msg_info_t* info)
	{
		// get and deserialize the messages
		if (sub.received_message_def.fields == 0)
		{
			//printf("WARN: got message but no message definition yet...\n");
			// queue it up, then print them out once I get it
			//todo_msgs.push_back({ message, *info });
		}
		else
		{
			GraphCanvas* canvas = (GraphCanvas*)data;
			//ps_deserialize_print(message, &sub.received_message_def);
			canvas->HandleMessage(message, &sub.received_message_def);
			//printf("-------------\n");
			free(message);
		}
	};

	ps_node_create_subscriber_adv(&node, topic_name.c_str(), 0, &sub, &options);
}

void GraphCanvas::HandleMessage(const void* data, const ps_message_definition_t* definition)
{
	std::string field_name = field_name_->GetText().c_str();
	
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
				
				AddSample(value, msg_time);
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

void GraphCanvas::AddSample(double value, pubsub::Time time)
{
	pubsub::Duration dt = time - start_time_;
	
	data_.push_back({dt.toSec(), value});
	
	// remove any old samples that no longer fit on screen
	while (data_.size() && data_.front().first < dt.toSec() - x_width_)
	{
		data_.pop_front();
	}
	
	// now update the graph start x and y
	min_x_ = dt.toSec() - x_width_;
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
	glBegin(GL_LINE_STRIP);
	glColor3f(1, 0, 0);
	for (auto& pt: data_)
	{
		glVertex2f(start_x + graph_width*(pt.first - min_x_)/(max_x_ - min_x_),
		           start_y + y_count*y_cell_size - graph_height*(pt.second - min_y_)/(max_y_ - min_y_));
	}
	glEnd();
	
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
}
