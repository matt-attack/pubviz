/*
	Copyright (c) 2022 Matthew Bries
	See license
	*/

#include "pubviz.h"
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
#include <signal.h>
#include <sys/types.h>
#include <stdio.h>

#include "Gwen/Controls/Properties.h"
#include "Gwen/Controls/PropertyTree.h"
#include "Gwen/Controls/Property/ColorSelector.h"
#include "Gwen/Controls/Property/Checkbox.h"
#include "Gwen/Controls/Property/ComboBox.h"

#include "OpenGLCanvas.h"

#include "plugins/Grid.h"


using namespace Gwen;

PubViz::~PubViz()
{

}

void PubViz::MenuItemSelect(Controls::Base* pControl)
{
	Gwen::Controls::MenuItem* pMenuItem = (Gwen::Controls::MenuItem*) pControl;
	if (pMenuItem->GetText() == L"Quit")
	{
		exit(0);
	}
}

void PubViz::Layout(Skin::Base* skin)
{
	BaseClass::Layout(skin);
}

GWEN_CONTROL_CONSTRUCTOR(PubViz)
{
	Dock(Pos::Fill);
	SetSize(1024, 768);
	SetPadding(Gwen::Padding(0, 0, 0, 0));

	//add menu bar
	menu_ = new Gwen::Controls::MenuStrip(this->GetParent());
	menu_->Dock(Pos::Top);
	{
		Gwen::Controls::MenuItem* pRoot = menu_->AddItem(L"File");
		pRoot->GetMenu()->AddItem(L"Quit", "", "Ctrl+Q")->SetAction(this, &ThisClass::MenuItemSelect);
	}
	{
		Gwen::Controls::MenuItem* pRoot = menu_->AddItem(L"View");
	}
	
	// now add the properties bar for plugins
	plugin_tree_ = new Gwen::Controls::PropertyTree( this );
	plugin_tree_->SetBounds( 200, 10, 200, 200 );
	{
		Gwen::Controls::Properties* props = plugin_tree_->Add( L"Item One" );
		props->Add( L"Middle Name" );
		props->Add( L"Last Name" );
		props->Add( L"Four" );
	}
				
	// Create our first plugin and bind default properties
	GridPlugin* grid = new GridPlugin();
	Gwen::Controls::Properties* props = plugin_tree_->Add( L"Grid Plugin" );
	auto pRow = props->Add( L"Enable", new Gwen::Controls::Property::Checkbox( props ), L"1" );
	pRow->onChange.Add( (Plugin*)grid, &Plugin::OnFirstNameChanged );
	grid->Initialize(props);
	plugins_.push_back(grid);
				
	plugin_tree_->ExpandAll();
				
	GetLeft()->GetTabControl()->AddPage("Plugins", plugin_tree_);
			
	canvas_ = new OpenGLCanvas(this);
	canvas_->Dock(Pos::Fill);
	canvas_->plugins_ = plugins_;//todo lets not maintain two lists

	m_StatusBar = new Controls::StatusBar(this->GetParent());
	m_StatusBar->Dock(Pos::Bottom);
	m_StatusBar->SendToBack();

	m_fLastSecond = Gwen::Platform::GetTimeInSeconds();
	m_iFrames = 0;
}

static int val = 0;
void PubViz::Render(Gwen::Skin::Base* skin)
{
	m_iFrames++;
	//show current line number of active tab, also need to figure out how to display active tab
	if (m_fLastSecond < Gwen::Platform::GetTimeInSeconds())
	{
		val = m_iFrames;
		m_fLastSecond = Gwen::Platform::GetTimeInSeconds() + 0.5f;
		m_iFrames = 0;
	}
	
	double x, y;
	canvas_->GetMousePosition(x, y);

	m_StatusBar->SetText(Gwen::Utility::Format(L"%i fps    X: %f Y: %f", val * 2, x, y));

	BaseClass::Render(skin);
}

