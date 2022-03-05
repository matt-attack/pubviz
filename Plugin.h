
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

class Plugin: public Gwen::Event::Handler
{
	bool enabled_ = true;
public:
	virtual ~Plugin() {};
	
	virtual void Initialize(Gwen::Controls::Properties*) = 0;
	
	virtual void Render() = 0;
	
	bool Enabled()
	{
		return enabled_;
	}

	void OnFirstNameChanged( Gwen::Controls::Base* pControl )
	{
		Gwen::Controls::PropertyRow* pRow = ( Gwen::Controls::PropertyRow* ) pControl;
		enabled_ = pRow->GetProperty()->GetPropertyValue().GetUnicode() == L"1";
	}
};

#endif
