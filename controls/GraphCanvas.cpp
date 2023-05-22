// todo license

#include <Gwen/Platform.h>
#include <Gwen/Controls/CheckBox.h>
#include <Gwen/Controls/Menu.h>
#include <Gwen/Controls/TextBox.h>
#include <Gwen/Controls/WindowControl.h>

#include "GraphCanvas.h"
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

static bool node_initialized = false;
static ps_node_t node;

static std::map<std::string, bool> _topics;

static int _num_canvases = 0;
static std::map<std::string, GraphSubscriber*> _subscribers;


GraphChannel::~GraphChannel()
{
	if (subscribed)
	{
		_subscribers[topic_name]->RemoveListener(this);
	}
}

GraphSubscriber::GraphSubscriber(const std::string& topic)
{
	topic_name = topic;

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
		GraphSubscriber* sub = (GraphSubscriber*)data;
		if (sub->subscriber.received_message_def.fields == 0)
		{
			//printf("WARN: got message but no message definition yet...\n");
			// queue it up, then print them out once I get it
			//todo_msgs.push_back({ message, *info });
            return;
		}

        // Populate the list of fields
		for (auto& channel: sub->listeners)
		{
        	if (channel == channel->canvas->graph_channels_.back() && 
				channel->canvas->field_list_->GetTable()->RowCount(0) == 0)
        	{
				printf("Updated fields for %s with %s\n", channel->topic_name.c_str(), sub->topic_name.c_str());
        	    //sub->canvas->field_list_->ClearItems();
        	    struct ps_deserialize_iterator iter = ps_deserialize_start((const char*)message, &sub->subscriber.received_message_def);
	    	    const struct ps_msg_field_t* field; uint32_t length; const char* ptr;
	    	    while (ptr = ps_deserialize_iterate(&iter, &field, &length))
	    	    {
			        channel->canvas->field_list_->AddItem(field->name, field->name);
	    	    }
        	}
		}

		// add the sample to each graph that this matches for
		auto time = pubsub::Time::now();
		for (auto& channel: sub->listeners)
		{
			if (!channel->canvas->canvas_->Paused())
			{
				channel->canvas->AddMessageSample(channel->channel, time, message, &sub->subscriber.received_message_def, true, true);
			}
		}
		free(message);
	};

	ps_node_create_subscriber_adv(&node, topic_name.c_str(), 0, &subscriber, &options);
}

GWEN_CONTROL_CONSTRUCTOR( GraphCanvas )
{
	m_Color = Gwen::Color( 255, 255, 255, 255 );
    auto sub = new GraphChannel();
	sub->canvas = this;
	sub->subscribed = false;
    sub->channel = CreateChannel("", "");
    sub->channel->can_remove = false;
    sub->channel->on_remove = [sub, this]() { RemoveChannel(sub); delete sub; };
	graph_channels_.push_back(sub);
	
	topic_name_box_ = new Gwen::Controls::TextBox( this );
	topic_name_box_->SetText( "" );
	topic_name_box_->SetPos( 10, 10 );
	topic_name_box_->SetWidth(300);
	topic_name_box_->onTextChanged.Add( this, &ThisClass::OnTopicEdited );
	topic_name_box_->onReturnPressed.Add( this, &ThisClass::OnTopicChanged );
	topic_name_box_->onFocusLost.Add( this, &ThisClass::OnTopicEditFinished );
    topic_name_box_->onFocusGained.Add( this, &ThisClass::OnTopicEditStart );
	
	field_name_box_ = new Gwen::Controls::TextBox( this );
	field_name_box_->SetText( "" );
	field_name_box_->SetPos( 320, 10 );
	field_name_box_->SetWidth(150);
	field_name_box_->onTextChanged.Add( this, &ThisClass::OnFieldChanged );
	field_name_box_->onFocusLost.Add( this, &ThisClass::OnFieldEditFinished );
    field_name_box_->onFocusGained.Add( this, &ThisClass::OnFieldEditStart );
	//label->onReturnPressed.Add( this, &ThisClass::OnTopicChanged );
	
	auto add_button = new Gwen::Controls::Button( this );
	add_button->SetText("+");
    add_button->SetFont(L"", 20, false);
	add_button->SetPos(480, 10);
	add_button->SetWidth(30);
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
	topic_list_->onRowSelected.Add( this, &ThisClass::OnTopicSuggestionClicked );

	field_list_ = new Gwen::Controls::ListBox(this);
	field_list_->Hide();
	field_list_->onRowSelected.Add( this, &ThisClass::OnFieldSuggestionClicked );

//add ability to save plots, for now just act like graph was pressed multiple times rather than worry about placement of the graphs (or could always pop them out)
	start_time_ = pubsub::Time::now();
	if (!node_initialized)
	{
		ps_node_init_ex(&node, "pubviz", "", false, false);
		//ps_node_init(&node, "pubviz", "", false);

		node.adv_cb = [](const char* topic, const char* type, const char* node, const ps_advertise_req_t* data)
		{
			// check if we already have the topic
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

	_num_canvases++;
}

GraphCanvas::~GraphCanvas()
{
	_num_canvases--;

	if (_num_canvases == 0)
	{
		// remove all subs just in case
		for (auto sub: _subscribers)
		{
			delete sub.second;
		}
		_subscribers.clear();
	}
}

void GraphCanvas::OnAdd(Base* control)
{
    if (field_name_box_->GetText().length() == 0)
    {
        return;
    }

	// add the current topic we have selected
    auto old_sub = graph_channels_.back();
	old_sub->field_name = field_name_box_->GetText().c_str();
    old_sub->channel->can_remove = true;

	// duplicate the current topic
    auto new_sub = new GraphChannel();
	new_sub->canvas = this;
	new_sub->subscribed = false;
	new_sub->topic_name = topic_name_box_->GetText().c_str();
    new_sub->field_name = "";//field_name_box_->GetText().c_str();
	field_name_box_->SetText("", false);
    new_sub->channel = CreateChannel(new_sub->topic_name, new_sub->field_name);
    new_sub->channel->can_remove = false;
    new_sub->channel->on_remove = [new_sub, this]() { RemoveChannel(new_sub); delete new_sub; };
	graph_channels_.push_back(new_sub);

	OnTopicChanged(topic_name_box_);
}

void GraphCanvas::OnTopicSuggestionClicked(Base* control)
{
	topic_name_box_->SetText(((Gwen::Controls::ListBox*)control)->GetSelectedRowName());
	OnTopicChanged(topic_name_box_);
	topic_list_->Hide();
}

void GraphCanvas::OnFieldSuggestionClicked(Base* control)
{
	field_name_box_->SetText(((Gwen::Controls::ListBox*)control)->GetSelectedRowName());
	OnFieldChanged(field_name_box_);
	field_list_->Hide();
}

void GraphCanvas::OnTopicEditStart(Base* control)
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

void GraphCanvas::OnTopicEditFinished(Base* control)
{
	topic_list_->Hide();
    OnTopicChanged(control);
}

void GraphCanvas::OnFieldEditStart(Base* control)
{
    auto p = control->GetPos();
	p.y += control->Height();
	field_list_->SetPos(p);
	field_list_->SetSize(100, 100);
	field_list_->Show();
}

void GraphCanvas::OnFieldEditFinished(Base* control)
{
	field_list_->Hide();
}

void GraphCanvas::OnFieldChanged(Base* control)
{
    RecalculateScale();

	auto p = control->GetPos();
	p.y += control->Height();
	field_list_->SetPos(p);
	field_list_->SetSize(100, 100);
	field_list_->Show();

    auto sub = graph_channels_.back();
    sub->field_name = field_name_box_->GetText().c_str();
    sub->channel->field_name = sub->field_name;
    
	sub->channel->data.clear();
}

void GraphCanvas::OnTopicEdited(Base* control)
{
    RecalculateScale();

	auto p = control->GetPos();
	p.y += control->Height();
	topic_list_->SetPos(p);
	topic_list_->SetSize(100, 100);
	topic_list_->Show();
	
	topic_list_->Clear();
    field_list_->Clear();
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
    RecalculateScale();

 	auto tb = (TextBox*)control;
	printf("%s\n", tb->GetText().c_str());
  
  	auto channel = graph_channels_.back();
	if (channel->subscribed)
	{
        // if the topic didnt change, dont do anything
        if (channel->topic_name == tb->GetText().c_str())
        {
            return;
        }
		auto sub = _subscribers[channel->topic_name];
		sub->RemoveListener(channel);
		if (sub->listeners.size() == 0)
		{
			_subscribers.erase(channel->topic_name);
		}
	}

	// store the topic name somewhere safe since the subscriber doesnt make a copy
	channel->topic_name = tb->GetText().c_str();
    channel->channel->topic_name = channel->topic_name;
	channel->subscribed = true;
	channel->channel->data.clear();
	auto sub = _subscribers[channel->topic_name];
	if (sub == 0)
	{
		sub = new GraphSubscriber(channel->topic_name);
		_subscribers[channel->topic_name] = sub;
	}
	sub->AddListener(channel);
}

void GraphCanvas::AddPlot(std::string topic, std::string field)
{
	auto backc = channels_.back();
	channels_.pop_back();
	auto back = graph_channels_.back();
	graph_channels_.pop_back();
	// duplicate the current topic
	auto new_sub = new GraphChannel();
	new_sub->canvas = this;
	new_sub->subscribed = true;
	new_sub->topic_name = topic;
	new_sub->field_name = field;
	new_sub->channel = CreateChannel(new_sub->topic_name, new_sub->field_name);
	new_sub->channel->can_remove = true;
	new_sub->channel->on_remove = [new_sub, this]() { RemoveChannel(new_sub); delete new_sub; };
	graph_channels_.push_back(new_sub);
	graph_channels_.push_back(back);
	channels_.push_back(backc);

	auto sub = _subscribers[new_sub->topic_name];
	if (sub == 0)
	{
		sub = new GraphSubscriber(new_sub->topic_name);
		_subscribers[new_sub->topic_name] = sub;
	}
	sub->AddListener(new_sub);
}

void GraphCanvas::Layout( Gwen::Skin::Base* skin )
{
	ps_node_spin(&node);

	BaseClass::Layout(skin);
	Invalidate();// we are being hacky, just always invalidate so we keep laying out
}

void GraphCanvas::DrawOnGraph(double start_x, double start_y, double graph_width, double graph_height)
{
	// convert timestamp to bag time
	if (hover_time_ != std::numeric_limits<double>::infinity())
	{
		// draw the hovered area
		glLineWidth(4.0f);
		glBegin(GL_LINE_STRIP);
		glColor3f(1.0, 0.0, 0.0);
		double sx = start_x + graph_width * (hover_time_/*position here*/ - min_x_) / (max_x_ - min_x_);
		glVertex2f(sx, start_y);
		glVertex2f(sx, start_y + graph_height);
		glEnd();
	}
}

void GraphCanvas::PaintOnGraph(double start_x, double start_y, double graph_width, double graph_height)
{
	if (hover_time_ != std::numeric_limits<double>::infinity())
	{
		auto r = GetSkin()->GetRender();
		double slider_time = hover_time_;
		int j = 0;
		char buffer[50];
		for (auto channel : channels_)
		{
			for (int i = channel->data.size() - 1; i >= 0; i--)
			{
				auto& pt = channel->data[i];
				if (pt.first < slider_time)
				{
					double x = start_x + graph_width * (slider_time - min_x_) / (max_x_ - min_x_);
					double y = start_y + j * 20;
					glVertex2f(x, y);

					sprintf(buffer, "%lf", pt.second);
					r->SetDrawColor(Gwen::Color(graph_colors[j][0] * 255, graph_colors[j][1] * 255, graph_colors[j][2] * 255, 255));
					r->RenderText(GetSkin()->GetDefaultFont(), Gwen::PointF(x + 15, y - 15), (std::string)buffer);

					break;
				}
			}
			j++;
		}
	}
}

bool GraphCanvas::OnMouseWheeled( int delta )
{
	return true;
}

void GraphCanvas::OnMouseClickLeft( int /*x*/, int /*y*/, bool down )
{

}

void GraphCanvas::OnMouseMoved(int x, int y, int dx, int dy)
{
	auto start_x = GraphStartPosition();
	auto graph_width = GraphWidth();

	x = CanvasPosToLocal(Gwen::Point(x, y)).x;

	double rel_time = ((x - start_x) / graph_width) * (max_x_ - min_x_) + min_x_;

	if (rel_time < min_x_ || rel_time > max_x_)
	{
		if (hover_time_ != std::numeric_limits<double>::infinity())
		{
			Redraw();
		}
		hover_time_ = std::numeric_limits<double>::infinity();
	}
	else
	{
		hover_time_ = rel_time;
		Redraw();
	}
}
