// todo license

#include <Gwen/Platform.h>
#include <Gwen/Controls/TextBox.h>

#include "Parameters.h"

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

GWEN_CONTROL_CONSTRUCTOR( Parameters )
{
	//auto p = new DoubleParameter(this);
	//p->Dock( Gwen::Pos::Top );
	//p->SetPos(0,0);
	//auto p2 = new DoubleParameter(this);
	//p2->Dock( Gwen::Pos::Top );
	//p->SetPos(0,0);
}

void Parameters::SetNode(ps_node_t* node)
{
	// lets also set up everything we need here
	node_ = node;
	
	node->param_confirm_cb = Parameters::AckCB;
	node->param_confirm_cb_data = (void*)this;
	
	struct ps_subscriber_options options;
	ps_subscriber_options_init(&options);
	options.skip = 0;
	options.allocator = 0;
	options.ignore_local = false;
	options.preferred_transport = true ? 1 : 0;
	options.cb_data = this;
	options.cb = [](void* message, unsigned int size, void* data, const ps_msg_info_t* info)
	{
	  auto ths = (Parameters*)data;
		ths->queue_.push_back((pubsub::msg::Parameters*)message);
	};

	ps_node_create_subscriber_adv(node_, "/parameters", pubsub::msg::Parameters::GetDefinition(), &param_sub_, &options);
}

void Parameters::Layout( Gwen::Skin::Base* skin )
{
	// Update all of the parameters
	for (auto param : params_)
	{
		param.second->Update();
	}
	
	// process any messages
	// our sub has a message definition, so the queue contains real messages
	while (queue_.size())
	{
	  auto data = queue_.front();
	  queue_.pop_front();
		// assert that its properly formatted
		if (data->name.size() != data->min.size() ||
			data->name.size() != data->max.size() ||
			data->name.size() != data->value.size())
		{
			printf("ERROR: Invalid parameter message.\n");
			
			delete data;
			continue;
		}

		//printf("got parameter message\n");
		
		// add parameters to our list
		for (int i = 0; i < data->name.size(); i++)
		{
			// todo handle more than double
			DoubleParameter* param = 0;
			if (params_.find(data->name[i]) == params_.end())
			{
				// instantiate the new parameter
				param = new DoubleParameter(this);
				param->Dock(Gwen::Pos::Top);
				param->SetNode(node_);
				param->SetName(data->name[i]);
				param->SetRange(data->min[i], data->max[i]);
				param->SetRemoteValue(std::atof(data->value[i]));
				param->SetLocalValue(std::atof(data->value[i]), false);
				params_[data->name[i]] = param;
			}
			else
			{
				// configure things that could have changed (not name or local value)
				param = params_[data->name[i]];
				param->SetRange(data->min[i], data->max[i]);
				param->SetRemoteValue(std::atof(data->value[i]));
			}
			std::string description = data->description[i];
			if (!description.length())
			{
			  description = "No description.";
			}
			param->SetDescription(description);
		}
		delete data;
	}
	Invalidate();// we are being hacky, just always invalidate so we keep laying out
}


