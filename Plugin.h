
#ifndef PUBVIZ_PLUGIN_H
#define PUBVIZ_PLUGIN_H

#include <Gwen/Controls/PropertyTree.h>

#include <pubsub/Node.h>

namespace Gwen
{
	namespace Controls
	{
		class TreeControl;
		class Properties;
		class MenuStrip;
		class TextBoxCode;
	}
}

class PubViz;
class Plugin: public Gwen::Event::Handler
{
	friend class PubViz;
	bool enabled_ = true;
	ps_node_t* node_;
	Gwen::Controls::Properties* props_;
	
	void OnEnableChecked( Gwen::Controls::Base* pControl )
	{
		Gwen::Controls::PropertyRow* pRow = ( Gwen::Controls::PropertyRow* ) pControl;
		enabled_ = pRow->GetProperty()->GetPropertyValue().GetUnicode() == L"1";
	}
	
public:
	virtual ~Plugin() {};
	
	virtual void Initialize(Gwen::Controls::Properties*) = 0;
	
	// Render the plugin to the canvas
	virtual void Render() = 0;
	
	// Update any topics we have and mark for redraws if necessary
	virtual void Update() = 0;
	
	// Returns if the plugin is enabled and should be rendered
	bool Enabled()
	{
		return enabled_;
	}
	
	// Get the node for this view
	ps_node_t* GetNode()
	{
		return node_;
	}
	
	// Indicate that we want a redraw
	void Redraw()
	{
		props_->Redraw();
	}
	
	virtual std::string GetTitle() = 0;
};

#endif
