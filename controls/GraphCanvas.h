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

#include "GraphBase.h"

class OpenGLCanvas;
class PubViz;
class GraphCanvas : public GraphBase
{
	friend class PubViz;
	
		struct Subscriber
		{
			bool subscribed = false;
			ps_sub_t sub;
			std::string topic_name;
			std::string field_name;
			GraphCanvas* canvas;
			
			GraphBase::Channel* channel;
			
			~Subscriber()
			{
				if (subscribed)
				{
					ps_sub_destroy(&sub);
				}
			}
		};
		
	public:

		GWEN_CONTROL( GraphCanvas, GraphBase );
		
		~GraphCanvas()
		{

		}

        virtual void DrawOnGraph(double start_x, double start_y, double graph_width, double graph_height);

	protected:

        void RemoveSub(Subscriber* sub)
        {
            for (int i = 0; i < subscribers_.size(); i++)
            {
                if (subscribers_[i] == sub)
                {
                    subscribers_.erase(subscribers_.begin() + i);
                    break;
                }
            }
        }
	
        void OnTopicEditStart(Base* control);
		void OnTopicEditFinished(Base* control);
		void OnTopicChanged(Base* control);
		void OnTopicEdited(Base* control);
        void OnFieldEditStart(Base* control);
		void OnFieldEditFinished(Base* control);
		void OnFieldChanged(Base* control);
		void OnTopicSuggestionClicked(Base* control);
        void OnFieldSuggestionClicked(Base* control);
		void OnAdd(Base* control);

        //void OnConfigure(Base* control);

		void Layout( Gwen::Skin::Base* skin );
		
		void OnMouseMoved(int x, int y, int dx, int dy) override;
		bool OnMouseWheeled( int iDelta ) override;
		void OnMouseClickLeft( int /*x*/, int /*y*/, bool /*bDown*/ ) override;
		
		Gwen::Controls::TextBox* field_name_box_;
		Gwen::Controls::TextBox* topic_name_box_;
		
		std::vector<Subscriber*> subscribers_;
		
		Gwen::Controls::ListBox* topic_list_;
		Gwen::Controls::ListBox* field_list_;

		OpenGLCanvas* canvas_;
		
		// these change as we get more samples
		//double min_x_ = 0.0;
		//double max_x_ = 10.0;// in seconds
		double x_width_ = 10.0;// in seconds, this is constant and set by user

        //bool autoscale_y_ = true;
        //bool redo_scale_ = true;
		
		//pubsub::Time start_time_;// the time we opened this graph, used for making time values smaller
};

#endif
