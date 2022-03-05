
#ifndef PUBVIZ_PLUGIN_H
#define PUBVIZ_PLUGIN_H

#include <Gwen/Controls/PropertyTree.h>

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
	Gwen::Controls::Properties* props_;
	
public:
	virtual ~Plugin() {};
	
	virtual void Initialize(Gwen::Controls::Properties*) = 0;
	
	virtual void Render() = 0;
	
	bool Enabled()
	{
		return enabled_;
	}
	
	virtual std::string GetTitle() = 0;

	void OnEnableChecked( Gwen::Controls::Base* pControl )
	{
		Gwen::Controls::PropertyRow* pRow = ( Gwen::Controls::PropertyRow* ) pControl;
		enabled_ = pRow->GetProperty()->GetPropertyValue().GetUnicode() == L"1";
	}
};

#endif
