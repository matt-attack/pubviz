// Todo license

#ifndef PUBVIZ_GRAPH_H
#define PUBVIZ_GRAPH_H

#include <Gwen/Controls/Base.h>
#include <Gwen/Controls/Label.h>
#include <Gwen/Controls/ListBox.h>
#include <Gwen/Gwen.h>
#include <Gwen/Skin.h>

#include <deque>

#include <pubsub_cpp/Time.h>

#include <pubsub/Serialization.h>

#include <pubsub_cpp/Node.h>

class PubViz;
class GraphCanvas : public Gwen::Controls::Base
{
	friend class PubViz;
	
		struct Subscriber
		{
			bool subscribed = false;
			ps_sub_t sub;
			std::string topic_name;
			std::string field_name;
			GraphCanvas* canvas;
			
			// contains the plot data of the graph, in sequential order
			std::deque<std::pair<double, double>> data;
			
			~Subscriber()
			{
				if (subscribed)
				{
					ps_sub_destroy(&sub);
				}
			}
		};
		
	public:

		GWEN_CONTROL( GraphCanvas, Gwen::Controls::Base );
		
		~GraphCanvas()
		{
			for (auto& sub: subscribers_)
			{
				delete sub;
			}
		}

		virtual void Render( Gwen::Skin::Base* skin );

		const Gwen::Color & GetColor() { return m_Color; }
		void SetColor( const Gwen::Color & col ) { m_Color = col; }
		
		void GetMousePosition(double& x, double& y)
		{
			x = x_mouse_position_;
			y = y_mouse_position_;
		}
		
		void AddSample(double value, pubsub::Time time, Subscriber* sub);

	protected:
	
		void OnTopicEditFinished(Base* control);
		void OnTopicChanged(Base* control);
		void OnTopicEdited(Base* control);
		void OnFieldChanged(Base* control);
		void OnSuggestionClicked(Base* control);
		void OnAdd(Base* control);
		
		void OnMouseMoved(int x, int y, int dx, int dy) override;
		bool OnMouseWheeled( int iDelta ) override;
		void OnMouseClickLeft( int /*x*/, int /*y*/, bool /*bDown*/ ) override;
		
		void HandleMessage(const void* message, const ps_message_definition_t* def, Subscriber* sub);

		Gwen::Color		m_Color;
		double view_height_m_;
		bool mouse_down_ = false;
		
		Gwen::Controls::TextBox* field_name_;
		Gwen::Controls::TextBox* topic_name_box_;
		
		std::vector<Subscriber*> subscribers_;
		
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
