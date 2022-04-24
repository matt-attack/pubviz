
#ifndef PUBVIZ_H
#define PUBVIZ_H

#include <Gwen/Gwen.h>
#include <Gwen/Align.h>
#include <Gwen/Utility.h>
#include <Gwen/Controls/WindowControl.h>
#include <Gwen/Controls/TabControl.h>
#include <Gwen/Controls/ListBox.h>
#include <Gwen/Controls/DockBase.h>
#include <Gwen/Controls/StatusBar.h>
#include <Gwen/Controls/PropertyTree.h>

#include "OpenGLCanvas.h"
#include "Plugin.h"

#include <pubsub/Node.h>

#include <thread>
#include <mutex>

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

class PubViz : public Gwen::Controls::DockBase
{
	public:

		GWEN_CONTROL(PubViz, Gwen::Controls::DockBase);

		~PubViz();

		void Render( Gwen::Skin::Base* skin );

		void MenuItemSelect(Gwen::Controls::Base* pControl);

		void Layout(Gwen::Skin::Base* skin);
		
		void AddPlugin(const std::string& name);

	private:
		
		void OnCategorySelect( Gwen::Event::Info info );
		void OnAddPlugin(Gwen::Controls::Base* control);
		void OnRemovePlugin( Gwen::Controls::Base* control);
		void OnAddPluginFinish(Gwen::Controls::Base* control);
		void OnBackgroundChange(Gwen::Controls::Base* control);
		void OnCenter(Gwen::Controls::Base* control);
		
		Gwen::Controls::PropertyTree* plugin_tree_;
		OpenGLCanvas* canvas_;
		
		ps_node_t node_;
		
		std::vector<Plugin*> plugins_;

		Gwen::Controls::StatusBar*	m_StatusBar;
		unsigned int				m_iFrames;
		float						m_fLastSecond;

		Gwen::Controls::MenuStrip*  menu_;
};

#define DEFINE_UNIT_TEST( name, displayname ) GUnit* RegisterUnitTest_##name( Gwen::Controls::Base* tab ){ return new name( tab ); }
#endif
