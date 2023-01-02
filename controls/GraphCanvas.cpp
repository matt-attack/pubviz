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

GWEN_CONTROL_CONSTRUCTOR( GraphCanvas )
{
	m_Color = Gwen::Color( 255, 255, 255, 255 );
    auto sub = new Subscriber();
	sub->canvas = this;
    sub->channel = CreateChannel("", "");
    sub->channel->can_remove = false;
    sub->channel->on_remove = [sub, this]() { RemoveSub(sub); delete sub; };
	subscribers_.push_back(sub);
	
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
	field_list_->AddItem("A");
	field_list_->AddItem("B");
	field_list_->AddItem("C");
	field_list_->AddItem("D");
	field_list_->AddItem("E");
	field_list_->AddItem("F");
	field_list_->AddItem("G");
	field_list_->Hide();
	field_list_->onRowSelected.Add( this, &ThisClass::OnFieldSuggestionClicked );
	
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
}

void GraphCanvas::OnAdd(Base* control)
{
    if (field_name_box_->GetText().length() == 0)
    {
        return;
    }

	// add a topic
    auto old_sub = subscribers_.back();
	old_sub->field_name = field_name_box_->GetText().c_str();
    old_sub->channel->can_remove = true;
    auto new_sub = new Subscriber();
	new_sub->canvas = this;
    new_sub->field_name = field_name_box_->GetText().c_str();
    new_sub->channel = CreateChannel("", new_sub->field_name);
    new_sub->channel->can_remove = false;
    new_sub->channel->on_remove = [new_sub, this]() { RemoveSub(new_sub); delete new_sub; };
	subscribers_.push_back(new_sub);
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
    redo_scale_ = true;
	auto p = control->GetPos();
	p.y += control->Height();
	field_list_->SetPos(p);
	field_list_->SetSize(100, 100);
	field_list_->Show();

    auto sub = subscribers_.back();
    sub->field_name = field_name_box_->GetText().c_str();
    sub->channel->field_name = sub->field_name;
    
	sub->channel->data.clear();
}

void GraphCanvas::OnTopicEdited(Base* control)
{
    redo_scale_ = true;

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
    redo_scale_ = true;
 	auto tb = (TextBox*)control;
	printf("%s\n", tb->GetText().c_str());
  
  	auto sub = subscribers_.back();
	if (sub->subscribed)
	{
        // if the topic didnt change, dont do anything
        if (sub->topic_name == tb->GetText().c_str())
        {
            return;
        }
		ps_sub_destroy(&sub->sub);
	}
	// store the topic name somewhere safe since the subscriber doesnt make a copy
	sub->topic_name = tb->GetText().c_str();
    sub->channel->topic_name = sub->topic_name;
	sub->subscribed = true;
	sub->channel->data.clear();
  
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
            return;
		}

        // Populate the list of fields
        if (sub == sub->canvas->subscribers_.back() && sub->canvas->field_list_->GetTable()->RowCount(0) == 0)
        {
            //sub->canvas->field_list_->ClearItems();
            struct ps_deserialize_iterator iter = ps_deserialize_start((const char*)message, &sub->sub.received_message_def);
	        const struct ps_msg_field_t* field; uint32_t length; const char* ptr;
	        while (ptr = ps_deserialize_iterate(&iter, &field, &length))
	        {
		        sub->canvas->field_list_->AddItem(field->name, field->name);
	        }
        }

		if (!sub->canvas->canvas_->Paused())
		{
			sub->canvas->AddMessageSample(sub->channel, pubsub::Time::now(), message, &sub->sub.received_message_def, true, true);
		}
		free(message);
	};

	ps_node_create_subscriber_adv(&node, sub->topic_name.c_str(), 0, &sub->sub, &options);
}

void GraphCanvas::Layout( Gwen::Skin::Base* skin )
{
	ps_node_spin(&node);

	BaseClass::Layout(skin);
	Invalidate();// we are being hacky, just always invalidate so we keep laying out
}

void GraphCanvas::DrawOnGraph(double start_x, double start_y, double graph_width, double graph_height)
{

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

}
