// Todo license

#ifndef PUBVIZ_SACK_GRAPH_H
#define PUBVIZ_SACK_GRAPH_H

#include <Gwen/Controls/Base.h>
#include <Gwen/Controls/Label.h>
#include <Gwen/Controls/ListBox.h>
#include <Gwen/Gwen.h>
#include <Gwen/Skin.h>

#include <deque>

#include <pubsub_cpp/Time.h>

#include <pubsub/Serialization.h>

#include "GraphBase.h"

class SackViewer;
class SackGraph : public GraphBase
{
	
public:

	GWEN_CONTROL( SackGraph, GraphBase );
		
	~SackGraph()
	{

	}

	virtual void Render( Gwen::Skin::Base* skin );

	virtual void PaintOnGraph(double start_x, double start_y, double graph_width, double graph_height);
    virtual void DrawOnGraph(double start_x, double start_y, double graph_width, double graph_height);

    void SetViewer(SackViewer* viewer);

	virtual bool DragAndDrop_CanAcceptPackage( Gwen::DragAndDrop::Package* pPackage ) 
	{
		if (pPackage->name == "topic" && !Is2D())
		{
			return true;
		}
		return false;
	}

	virtual bool DragAndDrop_HandleDrop( Gwen::DragAndDrop::Package* pPackage, int x, int y );

    //Channel* GetChannel(const std::string& topic, const std::string& field);

protected:

    bool mouse_down_ = false;
    SackViewer* viewer_;

	bool selecting_ = false;
	double selection_start_;
	double selection_end_;

    //void Layout(Gwen::Skin::Base* skin);
	
	//void OnRemove(Base* control);
    //void OnRemoveSelect(Gwen::Controls::Base* pControl);

    //void OnConfigure(Base* control);

	virtual bool OnKeyLeft( bool bDown ) override;
	virtual bool OnKeyRight( bool bDown ) override;
		
	void OnMouseMoved(int x, int y, int dx, int dy) override;
	bool OnMouseWheeled( int iDelta ) override;
	void OnMouseClickLeft( int /*x*/, int /*y*/, bool /*bDown*/ ) override;
	void OnMouseClickRight( int /*x*/, int /*y*/, bool /*bDown*/ ) override;
};

#endif
