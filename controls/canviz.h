
#ifndef CANVIZ_H
#define CANVIZ_H

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

#include <pubsub_cpp/Time.h>

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

struct CANMessage
{
	uint32_t id;
	bool is_extended;
	bool is_rtr;
	int length;// 0-8
	uint8_t data[8];
};

class CANViz: public Gwen::Controls::DockBase
{
	public:

		GWEN_CONTROL(CANViz, Gwen::Controls::DockBase);

		~CANViz();

		void Render( Gwen::Skin::Base* skin );

		void Layout(Gwen::Skin::Base* skin);
		
		void Open(const std::string& str);

		void HandleMessage(const CANMessage& msg, pubsub::Time time);

	private:

		void MenuItemSelect(Gwen::Controls::Base* pControl);
		void OnBagOpen(Gwen::Event::Info info);
        void OnPlay(Gwen::Controls::Base*);
        void OnLoop(Gwen::Controls::Base* ctrl);
        void OnPublish(Gwen::Controls::Base* ctrl);
		void OnClear(Gwen::Controls::Base* ctrl);
		void OnCapture(Gwen::Controls::Base* ctrl);
		void OnSave(Gwen::Controls::Base* ctrl);
		
		//SackViewer* viewer_;

		Gwen::Controls::StatusBar*	m_StatusBar;
		unsigned int				m_iFrames;
		float						m_fLastSecond;

        //Gwen::Controls::Button* play_button_;

		Gwen::Controls::MenuStrip*  menu_;

		Gwen::Controls::ListBox* received_by_id_;
		Gwen::Controls::ListBox* received_by_time_;


		bool run_thread_;
		std::thread can_thread_;
		std::mutex can_lock_;
		std::vector<std::pair<pubsub::Time, CANMessage>> can_data_;

		bool do_capture_ = true;
		std::map<int, std::pair<pubsub::Time, Gwen::Controls::Layout::TableRow*>> received_ids_;
};
#endif
