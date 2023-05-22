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

struct GraphChannel;
struct GraphSubscriber
{
	std::string topic_name;
	ps_sub_t subscriber;
	std::vector<GraphChannel*> listeners;

	GraphSubscriber(const std::string& topic);

	~GraphSubscriber()
	{
		ps_sub_destroy(&subscriber);
	}

	void AddListener(GraphChannel* channel)
	{
		for (int i = 0; i < listeners.size(); i++)
       	{
       		if (listeners[i] == channel)
       		{
           	    break;
           	}
       	}

		listeners.push_back(channel);
	}

	void RemoveListener(GraphChannel* channel)
	{
		for (int i = 0; i < listeners.size(); i++)
       	{
       		if (listeners[i] == channel)
       		{
           	    listeners.erase(listeners.begin() + i);
           	    break;
           	}
       	}
	}
};

class GraphCanvas;
struct GraphChannel
{
	bool subscribed = false;

	std::string topic_name;
	std::string field_name;
	GraphCanvas* canvas;
			
	GraphBase::Channel* channel;
	
	~GraphChannel();
};


class OpenGLCanvas;
class PubViz;
class GraphCanvas : public GraphBase
{
	friend class PubViz;

		
	public:

		GWEN_CONTROL( GraphCanvas, GraphBase );
		
		~GraphCanvas();

        virtual void DrawOnGraph(double start_x, double start_y, double graph_width, double graph_height);
		virtual void PaintOnGraph(double start_x, double start_y, double graph_width, double graph_height);

		void AddPlot(std::string topic, std::string field);

	protected:

        void RemoveChannel(GraphChannel* sub)
        {
            for (int i = 0; i < graph_channels_.size(); i++)
            {
                if (graph_channels_[i] == sub)
                {
                    graph_channels_.erase(graph_channels_.begin() + i);
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
		public:
		Gwen::Controls::TextBox* field_name_box_;
		Gwen::Controls::TextBox* topic_name_box_;
		
		std::vector<GraphChannel*> graph_channels_;
		
		Gwen::Controls::ListBox* topic_list_;
		Gwen::Controls::ListBox* field_list_;

		OpenGLCanvas* canvas_;
		
		// these change as we get more samples
		double x_width_ = 10.0;// in seconds, this is constant and set by user

		double hover_time_ = std::numeric_limits<double>::infinity();
};

#endif
