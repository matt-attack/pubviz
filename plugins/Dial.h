
#ifndef PUBVIZ_PLUGIN_DIAL_H
#define PUBVIZ_PLUGIN_DIAL_H

#include <Gwen/Gwen.h>
#include <Gwen/Align.h>
#include <Gwen/Utility.h>
#include <Gwen/Controls/WindowControl.h>
#include <Gwen/Controls/TabControl.h>
#include <Gwen/Controls/ListBox.h>
#include <Gwen/Controls/DockBase.h>
#include <Gwen/Controls/StatusBar.h>
#include <Gwen/Controls/PropertyTree.h>
#include <Gwen/Controls/Property/Numeric.h>
#include <Gwen/Controls/ImagePanel.h>

#define GLEW_STATIC
#include <GL/glew.h>

#ifndef _WIN32
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#endif

#include "../Plugin.h"
#include "../properties.h"

#include <pubsub/Image.msg.h>

class DialPlugin: public pubviz::Plugin
{
	FloatProperty* min_value_;
	FloatProperty* max_value_;
	BooleanProperty* auto_minmax_;

	NumberProperty* x_position_, *y_position_;
	NumberProperty* size_;
	
	TopicProperty* topic_;
	StringProperty* field_;
	
	bool sub_open_ = false;
	ps_sub_t subscriber_;
	double current_value_ = NAN;
	
	std::string current_topic_;
	void Subscribe(std::string str)
	{
		if (sub_open_)
		{
			ps_sub_destroy(&subscriber_);
		}
		
		Clear();
		
		current_topic_ = str;
    	struct ps_subscriber_options options;
    	ps_subscriber_options_init(&options);
    	options.preferred_transport = 1;// tcp yo
		options.want_message_def = true;
    	ps_node_create_subscriber_adv(GetNode(), current_topic_.c_str(), NULL, &subscriber_, &options);
    	sub_open_ = true;
	}
	
public:

	DialPlugin()
	{

	}
	
	virtual ~DialPlugin()
	{
		if (sub_open_)
		{
			ps_sub_destroy(&subscriber_);
			sub_open_ = false;
		}
	}

	// Clear out any historical data so the view gets cleared
	virtual void Clear()
	{
		current_value_ = NAN;
	}
		
	virtual void Update()
	{
		// process any messages
		// our sub has a message definition, so the queue contains real messages
		if (sub_open_)
		{
			while (char* data = (char*)ps_sub_deque(&subscriber_))
			{
				if (Paused())
				{
					free(data);//todo use allocator free
					continue;
				}

				HandleMessage(data);

				// user is responsible for freeing the message and its arrays
				free(data);
			}
		}
	}
	
	void HandleMessage(const char* msg)
	{
		if (!subscriber_.received_message_def.fields)
		{
			return;
		}

		current_value_ = NAN;

		struct ps_deserialize_iterator iter = ps_deserialize_start((const char*)msg, &subscriber_.received_message_def);
		const struct ps_msg_field_t* field; uint32_t length; const char* ptr;
		while (ptr = ps_deserialize_iterate(&iter, &field, &length))
		{
			if (strcmp(field->name, field_->GetValue().c_str()) != 0)
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
					
					current_value_ = value;
					Redraw();
					break;
				}
			}
		}
	}
	
	virtual void Render()
	{

	}

	virtual void Paint()
	{
		// nothing to do here since we dont render to the world
		// actually, lets render to the world?
		//current_value_ = rand()%11;

		glLineWidth(3.0);

		auto r = GetCanvas()->GetSkin()->GetRender();
		Gwen::Point pos = r->GetRenderOffset();
		//glLoadIdentity();
	//glRotatef(yaw, 0, 0, 1);
		auto scale = GetCanvas()->GetCanvas()->Scale();
		//glTranslatef(pos.x*scale, pos.y*scale, 0);// shift it so 0, 0 is _our_ top left corner
		//glScalef(scale, scale, 1.0);

		const int start_x = x_position_->GetValue() + pos.x;
		const int start_y = y_position_->GetValue() + pos.y;
		const float radius = size_->GetValue();
		const float height = radius/2.0;

		// draw it bitch
		const int num_pts = 20;
		const double radians_per_pt = M_PI/num_pts;
		glBegin(GL_LINE_STRIP);
		float opts[num_pts*2+2];
		for (int i = 0; i <= num_pts; i++)
		{
			float x = start_x + height + height*cos(radians_per_pt*i);
			float y = start_y + (height - height*sin(radians_per_pt*i));
			//printf("x: %f y: %f\n", x, y);
			// want to draw a half circle
			glColor4f(1.0, 1.0, 1.0, 1.0);
			glVertex2f(x*scale, y*scale);
			opts[i*2] = x*scale;
			opts[i*2+1] = y*scale;
		}

		// now draw the inner part
		const float inner_radius = radius*0.5;
		float ipts[num_pts*2+2];
		for (int i = num_pts; i >= 0; i--)
		{
			float x = start_x + height + inner_radius*cos(radians_per_pt*i)*0.5;
			float y = start_y + (height - inner_radius*sin(radians_per_pt*i)*0.5);
			//printf("x: %f y: %f\n", x, y);
			// want to draw a half circle
			glColor4f(1.0, 1.0, 1.0, 1.0);
			glVertex2f(x*scale, y*scale);
			ipts[i*2] = x*scale;
			ipts[i*2+1] = y*scale;
		}
		glVertex2f((start_x + radius)*scale, (start_y + height)*scale);
		glEnd();

		if (std::isnan(current_value_))
		{
			return;
		}

		float frac = (current_value_-min_value_->GetValue())/(max_value_->GetValue() - min_value_->GetValue());
		frac = std::max(0.0f, std::min(1.0f, frac));
		float angle = M_PI - std::max(0.0f, std::min(1.0f, frac))*M_PI;
		
		// draw a colored inside?
		glBegin(GL_TRIANGLES);
		for (int j = 0; j < 20; j++)
		{
			int i = j;//num_pts - j;
			glColor4f(0, 0, 0, 1);
			glVertex2f(opts[i*2], opts[i*2+1]);
			glVertex2f(opts[(i+1)*2], opts[(i+1)*2+1]);
			glVertex2f(ipts[i*2], ipts[i*2+1]);

			glVertex2f(ipts[(i+1)*2], ipts[(i+1)*2+1]);
			glVertex2f(opts[(i+1)*2], opts[(i+1)*2+1]);
			glVertex2f(ipts[i*2], ipts[i*2+1]);
		}
		for (int j = 0; j < num_pts*frac; j++)
		{
			int i = std::min(19, num_pts - j);
			glColor4f(2*(frac), 2*(1.0-frac), 0, 1);
			glVertex2f(opts[i*2], opts[i*2+1]);
			glVertex2f(opts[(i+1)*2], opts[(i+1)*2+1]);
			glVertex2f(ipts[i*2], ipts[i*2+1]);

			glVertex2f(ipts[(i+1)*2], ipts[(i+1)*2+1]);
			glVertex2f(opts[(i+1)*2], opts[(i+1)*2+1]);
			glVertex2f(ipts[i*2], ipts[i*2+1]);
		}
		// then draw the last segment
		glEnd();

		glLineWidth(5.0);
		glBegin(GL_LINES);
		glColor4f(0, 0, 0, 1);
		glVertex2f((start_x + radius/2)*scale, (start_y + height)*scale);
		glVertex2f((start_x + height + height*cos(angle))*scale, (start_y + (height - height*sin(angle)))*scale);
		glEnd();

		glLineWidth(2.0);
		glBegin(GL_LINES);
		glColor4f(1, 1, 1, 1);
		glVertex2f((start_x + radius/2)*scale, (start_y + height)*scale);
		glVertex2f((start_x + height + height*cos(angle))*scale, (start_y + (height - height*sin(angle)))*scale);
		glEnd();

		// draw the value
		r->SetDrawColor( Gwen::Color(255,255,255,255) );
		char buf[50];
		sprintf(buf, "%g", current_value_);
		r->RenderText(GetCanvas()->GetSkin()->GetDefaultFont(), Gwen::PointF( x_position_->GetValue(), y_position_->GetValue() + height ), (std::string)buf);
	}

	virtual void Initialize(Gwen::Controls::Properties* tree)
	{
		// add any properties
		topic_ = AddTopicProperty(tree, "Topic", "/image", "", "");
		topic_->onChange = std::bind(&DialPlugin::Subscribe, this, std::placeholders::_1);

		field_ = AddStringProperty(tree, "Field", "width");

		min_value_ = AddFloatProperty(tree, "Minimum", 0, -10000000, 100000);
		max_value_ = AddFloatProperty(tree, "Maximum", 10, -10000000, 100000);

		x_position_ = AddNumberProperty(tree, "X Offset", 0, 0, 10000);
		y_position_ = AddNumberProperty(tree, "Y Offset", 0, 0, 10000);
		size_ = AddNumberProperty(tree, "Size", 100, 1, 10000);
		
		Subscribe(topic_->GetValue());
	}
	
	std::string GetTitle() override
	{
		return "Dial";
	}
};

REGISTER_PLUGIN("dial", DialPlugin)

#endif
