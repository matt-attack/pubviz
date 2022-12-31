// Todo license

#ifndef PUBVIZ_GRAPH_BASE_H
#define PUBVIZ_GRAPH_BASE_H

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
class GraphBase : public Gwen::Controls::Base
{
    Gwen::Controls::Button remove_button_;
    Gwen::Controls::Button configure_button_;

	friend class PubViz;

    class Channel
    {
        friend class GraphBase;
        std::string topic_name;
		std::string field_name;
        std::deque<std::pair<double, double>> data;
    };
		
public:

	GWEN_CONTROL( GraphBase, Gwen::Controls::Base );
		
	~GraphBase()
	{
		for (auto& sub: channels_)
		{
			delete sub;
		}
	}

	virtual void Render( Gwen::Skin::Base* skin );

	const Gwen::Color & GetColor() { return m_Color; }
	void SetColor( const Gwen::Color & col ) { m_Color = col; }
		
    void AddSample(Channel* channel, double value, pubsub::Time time, double max_graph_width);

    void AddMessageSample(Channel* channel, pubsub::Time time, const void* message, const ps_message_definition_t* def, double max_graph_width = std::numeric_limits<double>::max());

    Channel* GetChannel(const std::string& topic, const std::string& field);

    virtual void DrawOnGraph(double start_x, double start_y, double graph_width, double graph_height) {}

    double GraphStartPosition() { return 80; };

    double GraphWidth()
    {
       	const int left_padding = 80;
	    const int other_padding = 50;
	
	    // target grid cell size
	    const int pixel_interval = 100;
	
	    // Calculate size to make the graph
	    Gwen::Rect b = GetRenderBounds();
	    return b.w - left_padding - other_padding;
    }

protected:

    void Layout(Gwen::Skin::Base* skin);
	
	void OnRemove(Base* control);
    void OnRemoveSelect(Gwen::Controls::Base* pControl);

    void OnConfigure(Base* control);
		
	void OnMouseMoved(int x, int y, int dx, int dy) override;
	bool OnMouseWheeled( int iDelta ) override;
	void OnMouseClickLeft( int /*x*/, int /*y*/, bool /*bDown*/ ) override;

	Gwen::Color		m_Color;
		
	std::vector<Channel*> channels_;
		
	// these change as we get more samples
	double min_x_ = 0.0;
	double max_x_ = 10.0;// in seconds
		
	double min_y_ = -100.0;
	double max_y_ = 100.0;

    bool autoscale_y_ = true;
    bool redo_scale_ = true;
		
	pubsub::Time start_time_;// the time we opened this graph, used for making time values smaller
};

#endif
