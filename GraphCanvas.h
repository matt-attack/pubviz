// Todo license

#ifndef PUBVIZ_GRAPH_H
#define PUBVIZ_GRAPH_H

#include <Gwen/Controls/Base.h>
#include <Gwen/Controls/Label.h>
#include <Gwen/Gwen.h>
#include <Gwen/Skin.h>

#include <deque>

#include <pubsub_cpp/Time.h>

class PubViz;
class GraphCanvas : public Gwen::Controls::Base
{
	friend class PubViz;
	public:

		GWEN_CONTROL( GraphCanvas, Gwen::Controls::Base );

		virtual void Render( Gwen::Skin::Base* skin );

		const Gwen::Color & GetColor() { return m_Color; }
		void SetColor( const Gwen::Color & col ) { m_Color = col; }
		
		void GetMousePosition(double& x, double& y)
		{
			x = x_mouse_position_;
			y = y_mouse_position_;
		}
		
		void AddSample(double value, pubsub::Time time);

	protected:
	
		void OnMouseMoved(int x, int y, int dx, int dy) override;
		bool OnMouseWheeled( int iDelta ) override;
		void OnMouseClickLeft( int /*x*/, int /*y*/, bool /*bDown*/ ) override;

		Gwen::Color		m_Color;
		double view_height_m_;
		bool mouse_down_ = false;
		
		double view_x_ = 0.0;
		double view_y_ = 0.0;
		
		double x_mouse_position_ = 0.0;
		double y_mouse_position_ = 0.0;
		
		// these change as we get more samples
		double min_x_ = 0.0;
		double max_x_ = 10.0;// in seconds
		double x_width_ = 10.0;// in seconds, this is constant and set by user
		
		double min_y_ = -2.0;
		double max_y_ = 2.0;
		
		pubsub::Time start_time_;// the time we opened this graph, used for making time values smaller
		
		// contains the plot data of the graph, in sequential order
		std::deque<std::pair<double, double>> data_;
};

#endif
