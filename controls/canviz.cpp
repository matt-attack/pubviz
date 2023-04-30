/*
	Copyright (c) 2022 Matthew Bries
	See license
	*/

#include "canviz.h"
#include "Gwen/Controls/DockedTabControl.h"
#include "Gwen/Controls/WindowControl.h"
#include "Gwen/Controls/CollapsibleList.h"
#include "Gwen/Controls/Layout/Position.h"
#include "Gwen/Platform.h"
#include "Gwen/Controls/TreeControl.h"
#include "Gwen/Controls/Properties.h"
#include "Gwen/Controls/Layout/Table.h"
#include "Gwen/Controls/PropertyTree.h"
#include "Gwen/Controls/WindowCanvas.h"

#include "Gwen/Controls/MenuStrip.h"
#include "Gwen/Controls/MenuItem.h"
#include "Gwen/Controls/TabControl.h"

#include <fstream>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <iomanip>

#include "Gwen/Controls/ComboBox.h"
#include <sys/types.h>
#include <stdio.h>

#include "Gwen/Controls/Properties.h"
#include "Gwen/Controls/PropertyTree.h"
#include "Gwen/Controls/Property/ColorSelector.h"
#include "Gwen/Controls/Property/Checkbox.h"
#include "Gwen/Controls/Property/ComboBox.h"
#include "Gwen/Controls/Property/Button.h"


#include "Gwen/Controls/Dialogs/FileOpen.h"

struct MessageField
{
	std::string name;
	int start_bit;
	int length;
	bool lsb_first;

	double scale;
	double offset;
};

struct MessageFormat
{
	std::string name;
	uint32_t id;

	std::vector<MessageField*> fields;
};


#include <sstream>
#include <iostream>

using namespace Gwen;

static std::thread can_thread;

std::string& remove_chars(std::string& s, const std::string& chars) {
    s.erase(std::remove_if(s.begin(), s.end(), [&chars](const char& c) {
        return chars.find(c) != std::string::npos;
    }), s.end());
    return s;
}

GWEN_CONTROL_CONSTRUCTOR(CANViz)
{
	Dock(Pos::Fill);
	SetSize(1024, 868);
	SetPadding(Gwen::Padding(0, 0, 0, 0));

	// load message format from file
	std::ifstream ifs("myfile.txt");
	std::string content( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );

	MessageFormat* msg = 0;

	std::istringstream sstream(content);
    std::string line;    
    while (std::getline(sstream, line))
	{
		if (line.length() < 3) continue;

		remove_chars(line, ":");

        std::cout << line << std::endl;

		std::vector<std::string> tokens;
    	std::stringstream ss(line);
    	std::string item;
    	while (std::getline(ss, item, ' '))
		{
        	tokens.push_back(item);
    	}

		if (line.length() > 3 && line[0] == 'B' && line[1] == 'O' && line[2] == '_')
		{
			msg = new MessageFormat();
			msg->id = std::atoi(tokens[1].c_str());
			msg->name = tokens[2];
			printf("message start %s\n", msg->name.c_str());
		}
		else if (line.length() > 4 && line[1] == 'S' && line[2] == 'G' && line[3] == '_')
		{
			printf("field start\n");
			if (!msg) continue;

			auto field = new MessageField();
			field->name = tokens[1];

			msg->fields.push_back(field);
		}
    }

	//add menu bar
	menu_ = new Gwen::Controls::MenuStrip(this);
	menu_->Dock(Pos::Top);
	{
		Gwen::Controls::MenuItem* pRoot = menu_->AddItem(L"File");
		pRoot->GetMenu()->AddItem(L"Open", "", "Ctrl+O")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Quit", "", "Ctrl+Q")->SetAction(this, &ThisClass::MenuItemSelect);
	}
	{
		Gwen::Controls::MenuItem* pRoot = menu_->AddItem(L"View");
		pRoot->GetMenu()->AddItem(L"Plot", "", "Ctrl+P")->SetAction(this, &ThisClass::MenuItemSelect);
	}

	Gwen::Controls::Base* base = new Gwen::Controls::Base(this);
	base->Dock(Pos::Fill);

	{
		Gwen::Controls::Label* label = new Gwen::Controls::Label( base );
		label->SetText( "ID" );
		label->SizeToContents();
		label->SetWidth(100);
		label->SetPos( 15, 10 );
	}

	{
		Gwen::Controls::Label* label = new Gwen::Controls::Label( base );
		label->SetText( "Period" );
		label->SizeToContents();
		label->SetWidth(100);
		label->SetPos( 125, 10 );
	}

	{
		Gwen::Controls::Label* label = new Gwen::Controls::Label( base );
		label->SetText( "Data" );
		label->SizeToContents();
		label->SetWidth(100);
		label->SetPos( 235, 10 );
	}

	Gwen::Controls::ListBox* ctrl = new Gwen::Controls::ListBox( base );
	ctrl->SetBounds( 10, 30, 500, 200 );
	ctrl->SetColumnCount( 3 );
	ctrl->SetAllowMultiSelect( true );
	ctrl->SetColumnWidth(0, 110);
	ctrl->SetColumnWidth(1, 110);
	ctrl->SetColumnWidth(2, 200);

	received_by_id_ = ctrl;


	{
		Gwen::Controls::Label* label = new Gwen::Controls::Label( base );
		label->SetText( "Time" );
		label->SizeToContents();
		label->SetWidth(100);
		label->SetPos( 15, 240 );
	}

	{
		Gwen::Controls::Label* label = new Gwen::Controls::Label( base );
		label->SetText( "ID" );
		label->SizeToContents();
		label->SetWidth(100);
		label->SetPos( 125, 240 );
	}

	{
		Gwen::Controls::Label* label = new Gwen::Controls::Label( base );
		label->SetText( "Data" );
		label->SizeToContents();
		label->SetWidth(100);
		label->SetPos( 235, 240 );
	}

	ctrl = new Gwen::Controls::ListBox( base );
	ctrl->SetBounds( 10, 260, 500, 200 );
	ctrl->SetColumnCount( 3 );
	ctrl->SetAllowMultiSelect( true );
	ctrl->SetColumnWidth(0, 110);
	ctrl->SetColumnWidth(1, 110);
	ctrl->SetColumnWidth(2, 200);

	received_by_time_ = ctrl;

	Controls::Button* pButtonA = new Controls::Button( base );
	pButtonA->SetPos(520, 290);
	pButtonA->SetText( L"Clear" );
	pButtonA->onPress.Add( this, &ThisClass::OnClear );

	pButtonA = new Controls::Button( base );
	pButtonA->SetPos(520, 260);
	pButtonA->SetText( L"Stop Capture" );
	pButtonA->onPress.Add( this, &ThisClass::OnCapture );

	pButtonA = new Controls::Button( base );
	pButtonA->SetPos(520, 320);
	pButtonA->SetText( L"Save Capture" );
	pButtonA->onPress.Add( this, &ThisClass::OnSave );

	base->SizeToChildren();

	/*auto m_SplitterBar = new Gwen::Controls::SplitterBar( ctrl );
	m_SplitterBar->SetPos( 80, 0 );
	m_SplitterBar->SetCursor( Gwen::CursorType::SizeWE );
	//m_SplitterBar->onDragged.Add( this, &Properties::OnSplitterMoved );
	//m_SplitterBar->SetShouldDrawBackground( false );
	m_SplitterBar->DoNotIncludeInSize();
m_SplitterBar->SetSize( 3, 200 );*/

	m_StatusBar = new Controls::StatusBar(this->GetParent());
	m_StatusBar->Dock(Pos::Bottom);
	m_StatusBar->SendToBack();

	SizeToChildren();

	m_fLastSecond = Gwen::Platform::GetTimeInSeconds();
	m_iFrames = 0;

	run_thread_ = true;
	can_thread_ = std::thread([this]()
	{
		CANMessage msg;
		msg.id = 0xFF0000;
		msg.length = 7;
		msg.data[0] = 0x12;
		msg.data[1] = 0x34;
		msg.data[2] = 0x56;
		msg.data[3] = rand()%0xFF;
		msg.data[4] = 0x0;
		msg.data[5] = 0x0;
		msg.data[6] = 0x0;
		msg.data[7] = 0x0;
		while (run_thread_)
		{
			msg.data[3] = rand()%0xFF;
			HandleMessage(msg, pubsub::Time::now());
			
			Gwen::Platform::Sleep(1000);
		}
	});
}

CANViz::~CANViz()
{
	run_thread_ = false;
	can_thread_.join();
}

void CANViz::OnCapture(Gwen::Controls::Base* ctrl)
{
	do_capture_ = !do_capture_;

	auto button = (Gwen::Controls::Button*)ctrl;
	button->SetText(do_capture_ ? "Stop Capture" : "Resume Capture");
}

void CANViz::OnSave(Gwen::Controls::Base* ctrl)
{

}

void CANViz::OnClear(Gwen::Controls::Base* ctrl)
{
	received_by_time_->Clear();
}

void CANViz::HandleMessage(const CANMessage& msg, pubsub::Time time)
{
	can_lock_.lock();
	can_data_.push_back({ time, msg });
	can_lock_.unlock();

	Invalidate();
	Gwen::Platform::InterruptWait();
}

void CANViz::MenuItemSelect(Controls::Base* pControl)
{
	Gwen::Controls::MenuItem* pMenuItem = (Gwen::Controls::MenuItem*) pControl;
	if (pMenuItem->GetText() == L"Quit")
	{
		exit(0);
	}
	else if (pMenuItem->GetText() == L"Plot")
	{
		auto button = GetRight()->GetTabControl()->AddPage("Graph");
		button->SetPopoutable(true);
		button->SetClosable(true);
		auto page = button->GetPage();
		//auto graph = new SackGraph(page);
    	//graph->SetViewer(viewer_);
		//graph->Dock(Pos::Fill);
    	page->GetParent()->GetParent()->SetWidth(580);
	}
	else if (pMenuItem->GetText() == L"Open")
	{
		Gwen::Dialogs::FileOpen(true, Gwen::String(""), Gwen::String(""), Gwen::String("sack|*.sack"), this, &ThisClass::OnBagOpen);
	}
}

void CANViz::OnBagOpen(Gwen::Event::Info info)
{
	auto name = info.String;
	//this->OpenTab(name);
	Open(name);
}

void CANViz::Open(const std::string& file)
{
	((Gwen::Controls::WindowCanvas*)GetParent())->SetTitle("Sackviz (" + file + ")");
}

static pubsub::Time start_time = pubsub::Time::now();

void CANViz::Layout(Skin::Base* skin)
{
	// process incoming can messages
    can_lock_.lock();
	auto data = can_data_;
	can_data_.clear();
	can_lock_.unlock();

	for (auto& msg: data)
	{
		// produce id string
		std::stringstream idstr;
		idstr << std::hex << msg.second.id;

		Gwen::Controls::Layout::TableRow* id_row;
		auto res = received_ids_.find(msg.second.id);
		if (res != received_ids_.end())
		{
			id_row = res->second.second;
			id_row->SetCellText( 1, std::to_string((msg.first - res->second.first).toSec()) );
		}
		else
		{
			// check for a matching id in our table
			id_row = received_by_id_->AddItem( idstr.str() );
			id_row->SetCellText( 1, L"" );
		}
		received_ids_[msg.second.id] = {msg.first, id_row};
		id_row->SetCellText( 2, L"$1.27" );

		// produce data string
		std::stringstream str;
		for (int i = 0; i < msg.second.length; i++)
		{
			str << std::setw(2) << std::setfill('0') << std::hex << (int)msg.second.data[i];
			str << " ";
		}
		id_row->SetCellText(2, str.str());

		std::stringstream timestr;
		timestr << std::fixed << std::setprecision(3) << (msg.first - start_time).toSec();

		if (do_capture_)
		{
			Gwen::Controls::Layout::TableRow* pRow2 = received_by_time_->AddItem( timestr.str() );
			pRow2->SetCellText(1, std::to_string(msg.second.id) );
			pRow2->SetCellText(2, str.str());
		}
	}

	BaseClass::Layout(skin);
}

static int val = 0;
void CANViz::Render(Gwen::Skin::Base* skin)
{
	m_iFrames++;
	if (m_fLastSecond < Gwen::Platform::GetTimeInSeconds())
	{
		val = m_iFrames;
		m_fLastSecond = Gwen::Platform::GetTimeInSeconds() + 0.5f;
		m_iFrames = 0;
	}

	m_StatusBar->SetText(Gwen::Utility::Format(L"%3i fps", val * 2));

	BaseClass::Render(skin);
}

