// Todo license

#ifndef PUBVIZ_PARAMETERS_H
#define PUBVIZ_PARAMETERS_H

#include <Gwen/Controls/Base.h>
#include <Gwen/Controls/Label.h>
#include <Gwen/Controls/ListBox.h>
#include <Gwen/Controls/HorizontalSlider.h>
#include <Gwen/Controls/TextBox.h>
#include <Gwen/Gwen.h>
#include <Gwen/Skin.h>

#include <deque>

#include <pubsub_cpp/Time.h>

#include <pubsub/Serialization.h>

#include <pubsub_cpp/Node.h>

class DoubleParameter : public Gwen::Controls::Base
{
	public:
		GWEN_CONTROL_INLINE( DoubleParameter, Gwen::Controls::Base )
		{
			SetSize( 200, 19 );
			
			name_ = "Unknown Parameter";
			
			label_ = new Gwen::Controls::Label( this );
			label_->SetAlignment( Gwen::Pos::CenterV | Gwen::Pos::Left );
			label_->SetText( name_ );
			label_->Dock( Gwen::Pos::Left );
			//label_->onPress.Add( m_RadioButton, &CheckBox::OnPress );
			label_->SetTabable( false );
			label_->SetKeyboardInputEnabled( false );
			
			min_ = new Gwen::Controls::Label( this );
			min_->SetAlignment( Gwen::Pos::CenterV | Gwen::Pos::Right );
			min_->SetText( "0.0" );
			min_->Dock( Gwen::Pos::Left );
			//label_->onPress.Add( m_RadioButton, &CheckBox::OnPress );
			min_->SetTabable( false );
			min_->SetKeyboardInputEnabled( false );
			min_->SetTextPadding(Gwen::Padding(5, 0, 5, 0));
			min_->SizeToContents();
			
			slider_ = new Gwen::Controls::HorizontalSlider( this );
			//slider_->SetPos( 10, 10 );
			slider_->SetSize( 150, 20 );
			slider_->SetRange( 0, 100 );
			slider_->SetFloatValue( 25 );
			slider_->Dock( Gwen::Pos::Fill );
			slider_->onValueChanged.Add( this, &DoubleParameter::SliderMoved );
			
			
			text_box_ = new Gwen::Controls::TextBoxNumeric(this);
			text_box_->Dock( Gwen::Pos::Right );
			text_box_->SetWidth(80.0);
			text_box_->onReturnPressed.Add( this, &DoubleParameter::TextChanged );
			
			max_ = new Gwen::Controls::Label( this );
			max_->SetAlignment( Gwen::Pos::CenterV | Gwen::Pos::Left );
			max_->SetText( "0.0" );
			max_->Dock( Gwen::Pos::Right );
			max_->SetTabable( false );
			max_->SetKeyboardInputEnabled( false );
			max_->SetTextPadding(Gwen::Padding(5, 0, 5, 0));
			max_->SizeToContents();
		}
		
		void SetNode(ps_node_t* node)
		{
			node_ = node;
		}
		
		void SetName(const std::string& name)
		{
			label_->SetText(name);
			name_ = name;
		}

		double remote_value_;
		void SetRemoteValue(double val)
		{
			remote_value_ = val;
			//printf("setting remote to %f\n", val);
		}

		void SetLocalValue(double val, bool send = true)
		{
			//printf("setting local to %f\n", val);
			if (send == false)
			{
				last_commanded_value_ = val;
			}
			text_box_->SetText(std::to_string(val));
			slider_->SetFloatValue(val);
		}
		
		void SetRange(double min, double max)
		{
			char buf[100];
			sprintf(buf, "%g", min);
			min_->SetText(buf);
			sprintf(buf, "%g", max);
			max_->SetText(buf);
			slider_->SetRange(min, max);
			min_->SizeToContents();
			max_->SizeToContents();
		}
		
		void Update()
		{
			// if the request timed out, reset to the last received value
			if (!FloatEqual(last_commanded_value_, remote_value_))
			{
				// if we fail to update after so long, give up
				if (pubsub::Time::now() > last_changed_time_ + pubsub::Duration(2.0))
				{
					SetLocalValue(remote_value_, false);
					printf("Change timed out on parameter '%s', resetting to previous value %lf\n", name_.c_str(), remote_value_);
				}
			}

			// if value still not equal to expected and the last request timed out, try again
			if (FloatEqual(remote_value_, last_commanded_value_))
			{
				return;
			}

			//printf("%f != %f\n", remote_value_, last_commanded_value_);
			
			// retry if value is not equal to expected after a little bit
			if (pubsub::Time::now() > last_commanded_time_ + pubsub::Duration(0.2))
			{
				printf("Retrying updating value %s to %f\n", name_.c_str(), last_commanded_value_);
				SendUpdatedValue(last_commanded_value_);
			}
		}

	private:

		static bool FloatEqual(double a, double b)
		{
			return std::abs(a-b) < 0.000001;
		}
	
		void TextChanged(Gwen::Controls::Base* control)
		{
			double value = std::atof(text_box_->GetValue().c_str());
			slider_->SetFloatValue(value);
			last_changed_time_ = pubsub::Time::now();
			if (!FloatEqual(value, last_commanded_value_))// prevents double sends on update
			{
				SendUpdatedValue(value);
			}
		}
		
		void SliderMoved(Gwen::Controls::Base* control)
		{
			double value = slider_->GetFloatValue();
			text_box_->SetText(std::to_string(value));
			last_changed_time_ = pubsub::Time::now();
			if (!FloatEqual(value, last_commanded_value_))// prevents double sends on update
			{
				SendUpdatedValue(value);
			}
		}
		
		void SendUpdatedValue(double value)
		{
			//printf("Sending set param to set %s to %f\n", name_.c_str(), value);
			ps_node_set_parameter(node_, name_.c_str(), value);
			last_commanded_value_ = value;
			last_commanded_time_ = pubsub::Time::now();
		}
		
		Gwen::Controls::HorizontalSlider* slider_;
		Gwen::Controls::Label* label_;
		
		Gwen::Controls::Label* min_;
		Gwen::Controls::Label* max_;
		
		Gwen::Controls::TextBoxNumeric* text_box_;
		
		std::string name_;
		
		ps_node_t* node_;
		
		double last_commanded_value_ = 0.0;
		pubsub::Time last_changed_time_ = pubsub::Time(0);
		pubsub::Time last_commanded_time_ = pubsub::Time(0);
};

class PubViz;
class Parameters : public Gwen::Controls::Base
{
	friend class PubViz;
	
		ps_sub_t param_sub_;
		
		ps_node_t* node_;
		
		std::map<std::string, DoubleParameter*> params_;
		
	public:

		GWEN_CONTROL( Parameters, Gwen::Controls::Base );
		
		~Parameters()
		{
			ps_sub_destroy(&param_sub_);
		}
		
		static void AckCB(const char* name, double value, void* data)
		{
			Parameters* myself = (Parameters*)data;
			//printf("Got param change ack for %s %f\n", name, value);
			
			auto iter = myself->params_.find(name);
			if (iter == myself->params_.end())
			{
				return;
			}
			
			iter->second->SetRemoteValue(value);
		}
		
		void SetNode(ps_node_t* node);
		
		void Layout( Gwen::Skin::Base* skin ) override;


		const Gwen::Color & GetColor() { return m_Color; }
		void SetColor( const Gwen::Color & col ) { m_Color = col; }
		
		void GetMousePosition(double& x, double& y)
		{
			x = x_mouse_position_;
			y = y_mouse_position_;
		}
		
		//void AddSample(double value, pubsub::Time time, Subscriber* sub);

	protected:
	
		void OnTopicEditFinished(Base* control);
		void OnTopicChanged(Base* control);
		void OnTopicEdited(Base* control);
		void OnFieldChanged(Base* control);
		void OnSuggestionClicked(Base* control);
		void OnAdd(Base* control);

		Gwen::Color		m_Color;
		double view_height_m_;
		bool mouse_down_ = false;
		
		Gwen::Controls::TextBox* field_name_;
		Gwen::Controls::TextBox* topic_name_box_;
		
		double view_x_ = 0.0;
		double view_y_ = 0.0;
		
		Gwen::Controls::ListBox* topic_list_;
		
		double x_mouse_position_ = 0.0;
		double y_mouse_position_ = 0.0;
		
		// these change as we get more samples
		double min_x_ = 0.0;
		double max_x_ = 10.0;// in seconds
		double x_width_ = 10.0;// in seconds, this is constant and set by user
		
		double min_y_ = -2.0;
		double max_y_ = 10.0;
		
		pubsub::Time start_time_;// the time we opened this graph, used for making time values smaller
};

#endif
