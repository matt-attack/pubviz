
#ifndef PUBVIZ_H
#define PUBVIZ_H

#include <pubsub/Node.h>

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
#include "../Plugin.h"


#include "Parameters.h"

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

class BaseRegisterObject
{
public:

	virtual Plugin* Construct() = 0;

	static void Add(const std::string& type, BaseRegisterObject* builder);

	static Plugin* Construct(const std::string& type);
};

std::map<std::string, BaseRegisterObject*>& GetPluginList();

template<class T>
struct RegisterObject: public BaseRegisterObject
{
	RegisterObject(std::string name)
	{
		printf("Registered plugin %s as %s\n", typeid(T).name(), name.c_str());
		BaseRegisterObject::Add(name, this);
	}

	virtual Plugin* Construct()
	{
		return new T;
	}
};

#define REGISTER_PLUGIN(name, class) static RegisterObject<class> register_##class(name);

class PubViz: public Gwen::Controls::DockBase
{
	public:

		GWEN_CONTROL(PubViz, Gwen::Controls::DockBase);

		~PubViz();

		void Render( Gwen::Skin::Base* skin );

		void MenuItemSelect(Gwen::Controls::Base* pControl);

		virtual void Layout(Gwen::Skin::Base* skin) override;
		
		Plugin* AddPlugin(const std::string& name);

	private:
		
		void OnCategorySelect( Gwen::Event::Info info );
		void OnAddPlugin(Gwen::Controls::Base* control);
		void OnRemovePlugin( Gwen::Controls::Base* control);
		void OnAddPluginFinish(Gwen::Controls::Base* control);
		void OnBackgroundChange(Gwen::Controls::Base* control);
		void OnFrameChange(Gwen::Controls::Base* control);
		void OnShowOriginChange(Gwen::Controls::Base* control);
		void OnCenter(Gwen::Controls::Base* control);
        void OnShowConfigChanged(Gwen::Controls::Base* control);
        void OnPause(Gwen::Controls::Base* control);
		
		void OnParametersClose(Gwen::Controls::Base* control)
		{
			parameters_page_ = 0;
		}
		
		void OnConfigSave(Gwen::Event::Info info);
		void OnConfigLoad(Gwen::Event::Info info);
		
		void ClearPlugins();
		
		Gwen::Controls::PropertyTree* plugin_tree_;
		Gwen::Controls::Button* pause_button_;
		Gwen::Controls::MenuItem* pause_item_;
		OpenGLCanvas* canvas_;
		
		Parameters* parameters_page_ = 0;
		
		ps_node_t node_;
		
		std::vector<Plugin*> plugins_;

		Gwen::Controls::StatusBar*	m_StatusBar;
		unsigned int				m_iFrames;
		float						m_fLastSecond;

		Gwen::Controls::MenuStrip*  menu_;
};

#define DEFINE_UNIT_TEST( name, displayname ) GUnit* RegisterUnitTest_##name( Gwen::Controls::Base* tab ){ return new name( tab ); }
#endif
