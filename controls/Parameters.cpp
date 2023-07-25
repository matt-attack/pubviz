// todo license

#include <Gwen/Platform.h>
#include <Gwen/Controls/TextBox.h>

#include "Parameters.h"

#include <pubsub/Parameters.msg.h>

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

Parameters* myself = 0;

void Parameters::SetNode(ps_node_t* node)
{
	// lets also set up everything we need here
	node_ = node;
	
	myself = this;
	
	node->param_confirm_cb = Parameters::AckCB;
	
	struct ps_subscriber_options options;
	ps_subscriber_options_init(&options);
	options.skip = 0;
	options.queue_size = 100;
	options.want_message_def = false;
	options.allocator = 0;
	options.ignore_local = false;
	options.preferred_transport = true ? 1 : 0;
	options.cb_data = 0;
			/*options.cb = [](void* message, unsigned int size, void* data, const ps_msg_info_t* info)
			{
				// todo deserialize
				free(message);
			};*/

	ps_node_create_subscriber_adv(node_, "/parameters", pubsub::msg::Parameters::GetDefinition(), &param_sub_, &options);
}

void Parameters::Layout( Gwen::Skin::Base* skin )
{
	// Update all of the parameters
	for (auto param: params_)
	{
		param.second->Update();
	}
	
	// process any messages
	// our sub has a message definition, so the queue contains real messages
	pubsub::msg::Parameters* data;
	while (data = (pubsub::msg::Parameters*)ps_sub_deque(&param_sub_))
	{
		// assert that its properly formatted
		if (data->name_length != data->min_length ||
			data->name_length != data->max_length ||
			data->name_length != data->value_length)
		{
			printf("ERROR: Invalid parameter message.\n");
			
			data->~Parameters();
			free(data);
			continue;
		}
		
		// add parameters to our list
		for (int i = 0; i < data->name_length; i++)
		{
			// todo handle more than double
			DoubleParameter* param = 0;
			if (params_.find(data->name[i]) == params_.end())
			{
				// instantiate the new parameter
				param = new DoubleParameter(this);
				param->Dock(Gwen::Pos::Top);
				param->SetNode(node_);
				params_[data->name[i]] = param;
			}
			else
			{
				param = params_[data->name[i]];
			}
			
			// configure it
			param->SetName(data->name[i]);
			param->SetRange(data->min[i], data->max[i]);
			param->SetValue(std::atof(data->value[i]));
		}
		data->~Parameters();
		free(data);
	}
	Invalidate();// we are being hacky, just always invalidate so we keep laying out
}


