
#ifndef SACKVIZ_H
#define SACKVIZ_H

#include <Gwen/Gwen.h>
#include <Gwen/Align.h>
#include <Gwen/Utility.h>
#include <Gwen/Controls/WindowControl.h>
#include <Gwen/Controls/TabControl.h>
#include <Gwen/Controls/ListBox.h>
#include <Gwen/Controls/DockBase.h>
#include <Gwen/Controls/StatusBar.h>
#include <Gwen/Controls/PropertyTree.h>

#include "SackViewer.h"

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

class SackViz : public Gwen::Controls::DockBase
{
	public:

		GWEN_CONTROL(SackViz, Gwen::Controls::DockBase);

		~SackViz();

		void Render( Gwen::Skin::Base* skin );

		void Layout(Gwen::Skin::Base* skin);
		
		void Open(const std::string& str);

	private:

		void MenuItemSelect(Gwen::Controls::Base* pControl);
		void OnBagOpen(Gwen::Event::Info info);
        void OnPlay(Gwen::Controls::Base*);
        void OnLoop(Gwen::Controls::Base* ctrl);
        void OnPublish(Gwen::Controls::Base* ctrl);
		
		SackViewer* viewer_;

		Gwen::Controls::StatusBar*	m_StatusBar;
		unsigned int				m_iFrames;
		float						m_fLastSecond;

        Gwen::Controls::Button* play_button_;

		Gwen::Controls::MenuStrip*  menu_;
};
#endif
