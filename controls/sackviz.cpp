/*
	Copyright (c) 2022 Matthew Bries
	See license
	*/

#include "sackviz.h"
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
#include "Gwen/Controls/Dialogs/FileSave.h"

#include "SackViewer.h"
#include "SackGraph.h"


using namespace Gwen;

SackViz::~SackViz()
{

}

void SackViz::MenuItemSelect(Controls::Base* pControl)
{
	Gwen::Controls::MenuItem* pMenuItem = (Gwen::Controls::MenuItem*) pControl;
	if (pMenuItem->GetText() == L"Quit")
	{
		exit(0);
	}
	else if (pMenuItem->GetText() == L"Plot")
	{
		viewer_->PlotSelected(false);
	}
	else if (pMenuItem->GetText() == L"Plot 2D")
	{
		viewer_->PlotSelected(true);
	}
	else if (pMenuItem->GetText() == L"Open")
	{
		Gwen::Dialogs::FileOpen(true, Gwen::String("Open Rucksack"), Gwen::String(""), Gwen::String("sack|*.sack"), this, &ThisClass::OnBagOpen);
	}
	else if (pMenuItem->GetText() == L"Open Config")
	{
		Gwen::Dialogs::FileOpen(true, Gwen::String("Open Config"), Gwen::String(""), Gwen::String(".config|*.config|All|*.*"), this, &ThisClass::OnConfigOpen);
	}
	else if (pMenuItem->GetText() == L"Save Config")
	{
		Gwen::Dialogs::FileSave(true, String("Save Config"), String("sackviz.config"), String(".config|*.config|All|*.*"), this, &ThisClass::OnConfigSave);
	}
}

std::vector<std::string> split(std::string s, std::string delimiter)
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
    {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

void SackViz::OnConfigOpen(Gwen::Event::Info info)
{
	auto filename = info.String;
	LoadConfig(filename);
}

void SackViz::LoadConfig(const std::string& filename)
{
	// now load the new config
	FILE *f = fopen(filename.c_str(), "rb");
	if (f == 0)
	{
		return;// failed to open file
	}
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

	std::string string;
	string.resize(fsize, 'a');
	fread((char*)string.data(), fsize, 1, f);
	fclose(f);
	
	// create all the plugins
	// first, split by line
	auto lines = split(string, "\n");
	for (auto l: lines)
	{
		//printf("Line: %s\n", l.c_str());
		auto data = split(l, ":");
		
		if (data.size() <= 1)
		{
			continue;
		}

		if (data[0] == "plot" || data[0] == "plot2d")
		{
			bool is_2d = data[0] == "plot2d";

			auto fields = split(data[1], ",");

			std::vector<std::pair<std::string, std::string>> plots;
			for (int i = 0; i < fields.size() - 1; i+=2)
			{
				plots.push_back({fields[i], fields[i+1]});
			}
			viewer_->AddPlot(is_2d, plots);

			continue;
		}
	}
}

void SackViz::OnConfigSave(Gwen::Event::Info info)
{
	auto name = info.String;
	auto config = viewer_->SerializeConfig();
	FILE* f = fopen(name.c_str(), "wb");
	fwrite(config.c_str(), 1, config.length(), f);
	fclose(f);
}

void SackViz::OnBagOpen(Gwen::Event::Info info)
{
	auto name = info.String;
	//this->OpenTab(name);
	Open(name);
}

void SackViz::Open(const std::string& file)
{
    play_button_->onToggle.RemoveHandler(this);
    play_button_->SetToggleState(false);
    play_button_->SetText("Play");
	play_button_->onToggle.Add( this, &ThisClass::OnPlay );
	viewer_->OpenFile(file);

	((Gwen::Controls::WindowCanvas*)GetParent())->SetTitle("Sackviz (" + file + ")");
}

void SackViz::Layout(Skin::Base* skin)
{
    // handle 
	BaseClass::Layout(skin);
}

GWEN_CONTROL_CONSTRUCTOR(SackViz)
{
	Dock(Pos::Fill);
	SetSize(1024, 768);
	SetPadding(Gwen::Padding(0, 0, 0, 0));

	//add menu bar
	menu_ = new Gwen::Controls::MenuStrip(this);
	menu_->Dock(Pos::Top);
	{
		Gwen::Controls::MenuItem* pRoot = menu_->AddItem(L"File");
		pRoot->GetMenu()->AddItem(L"Open", "", "Ctrl+O")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddDivider();		
		pRoot->GetMenu()->AddItem(L"Open Config", "", "")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Save Config", "", "Ctrl+S")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddDivider();
		pRoot->GetMenu()->AddItem(L"Quit", "", "Ctrl+Q")->SetAction(this, &ThisClass::MenuItemSelect);
	}
	{
		Gwen::Controls::MenuItem* pRoot = menu_->AddItem(L"View");
		pRoot->GetMenu()->AddItem(L"Plot", "", "Ctrl+P")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Plot 2D", "", "Shift+P")->SetAction(this, &ThisClass::MenuItemSelect);
	}
	
	auto scroll = new Gwen::Controls::ScrollControl(this);
	scroll->SetScroll(false, true);
	scroll->Dock(Pos::Fill);
	scroll->SizeToMinimum(true);

	viewer_ = new SackViewer(scroll);
	viewer_->Dock(Pos::Fill);

	m_StatusBar = new Controls::StatusBar(this->GetParent());
	m_StatusBar->Dock(Pos::Bottom);
	m_StatusBar->SendToBack();

	auto loop_button = new Controls::Button(m_StatusBar);
	loop_button->Dock(Pos::Right);
	loop_button->SetText("Loop");
    loop_button->SetIsToggle(true);
    loop_button->onToggle.Add( this, &ThisClass::OnLoop );
    
	play_button_ = new Controls::Button(m_StatusBar);
	play_button_->Dock(Pos::Right);
	play_button_->SetText("Play");
    play_button_->SetIsToggle(true);
	play_button_->onToggle.Add( this, &ThisClass::OnPlay );

	auto publish_button = new Controls::Button(m_StatusBar);
	publish_button->Dock(Pos::Right);
	publish_button->SetText("Publish");
    publish_button->SetIsToggle(true);
    publish_button->onToggle.Add( this, &ThisClass::OnPublish );

	m_fLastSecond = Gwen::Platform::GetTimeInSeconds();
	m_iFrames = 0;
}

void SackViz::OnPublish(Gwen::Controls::Base* ctrl)
{
    auto button = (Gwen::Controls::Button*)ctrl;
    viewer_->ShouldPublish(button->GetToggleState());
}

void SackViz::OnLoop(Gwen::Controls::Base* ctrl)
{
    auto button = (Gwen::Controls::Button*)ctrl;
    viewer_->LoopPlayback(button->GetToggleState());
}

void SackViz::OnPlay(Gwen::Controls::Base*)
{
    if (play_button_->GetToggleState())
    {
        viewer_->Play();
        play_button_->SetText("Pause");
    }
    else
    {
        viewer_->Pause();
        play_button_->SetText("Resume");
    }
}

static int val = 0;
void SackViz::Render(Gwen::Skin::Base* skin)
{
	m_iFrames++;
	if (m_fLastSecond < Gwen::Platform::GetTimeInSeconds())
	{
		val = m_iFrames;
		m_fLastSecond = Gwen::Platform::GetTimeInSeconds() + 0.5f;
		m_iFrames = 0;
	}

	double dT = (viewer_->GetPlayheadTime() - viewer_->GetStartTime())/1000000.0;

	if (viewer_->GetStartTime() == 0 && viewer_->GetEndTime() == 0)
	{
		m_StatusBar->SetText(Gwen::Utility::Format(L"%3i fps", val * 2));
	}
	else
	{
		auto string = pubsub::Time(viewer_->GetPlayheadTime()).toString();
		char buf[1000];
		sprintf(buf, "%3i fps    %lf    %lfs   %s", val * 2, viewer_->GetPlayheadTime() / 1000000.0, dT, string.c_str());
		m_StatusBar->SetText(buf);
	}

	BaseClass::Render(skin);
}

