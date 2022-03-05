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
#include "Gwen/Controls/Property/Button.h"

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
				
	auto page = GetLeft()->GetTabControl()->AddPage("Plugins")->GetPage();
	
	// now add the properties bar for plugins
	plugin_tree_ = new Gwen::Controls::PropertyTree( page );
	plugin_tree_->Dock(Pos::Fill);
	plugin_tree_->SetBounds( 200, 10, 200, 200 );
	
	plugin_tree_->ExpandAll();
	
	//Controls::Button* remove_button = new Controls::Button( page );
	//remove_button->Dock(Pos::Bottom);
	//remove_button->SetText( L"Remove Plugin" );
	
	Controls::Button* add_button = new Controls::Button( page );
	add_button->Dock(Pos::Bottom);
	add_button->SetText( L"Add Plugin" );
			
	canvas_ = new OpenGLCanvas(this);
	canvas_->Dock(Pos::Fill);
	canvas_->plugins_ = plugins_;//todo lets not maintain two lists
	
	AddPlugin("grid");
	
	add_button->onPress.Add( this, &ThisClass::OnAddPlugin );

	m_StatusBar = new Controls::StatusBar(this->GetParent());
	m_StatusBar->Dock(Pos::Bottom);
	m_StatusBar->SendToBack();

	m_fLastSecond = Gwen::Platform::GetTimeInSeconds();
	m_iFrames = 0;
}

void PubViz::AddPlugin(const std::string& name)
{
	Plugin* plugin;
	if (name == "grid")
	{
		// todo
		plugin = new GridPlugin();
	}
	else
	{
		printf("Unknown plugin name '%s'!\n", name.c_str());
		return;
	}
	
	Gwen::Controls::Properties* props = plugin_tree_->Add( name );
	((Gwen::Controls::TreeNode*)props->GetParent())->Open();
	auto pRow = props->Add( L"Enable", new Gwen::Controls::Property::Checkbox( props ), L"1" );
	pRow->onChange.Add( plugin, &Plugin::OnEnableChecked );
	plugin->Initialize(props);
	plugin->props_ = props;
	
	// add the close button
	auto pRow2 = props->Add( L"", new Gwen::Controls::Property::Button( props ), L"Close" );
	pRow2->onChange.Add( this, &PubViz::OnRemovePlugin );
	pRow2->UserData.Set<Plugin*>("plugin", plugin);
	
	plugins_.push_back(plugin);
	canvas_->plugins_ = plugins_;
}

void PubViz::OnAddPlugin(Gwen::Controls::Base* control)
{
	Controls::WindowControl* window = new Controls::WindowControl( GetCanvas() );
	window->SetTitle( L"Add Plugin" );
	window->SetSize( 200, 100 );
	window->MakeModal( true );
	window->Position( Pos::Center );
	window->SetDeleteOnClose( true );
	
	Gwen::Controls::ComboBox* combo = new Gwen::Controls::ComboBox( window );
		combo->Dock(Pos::Top);
	combo->SetPos( 50, 50 );
	combo->SetWidth( 200 );
	combo->AddItem( L"Grid", "grid" );
	combo->AddItem( L"Number Two", "two" );
	combo->AddItem( L"Door Three", "three" );
	combo->AddItem( L"Four Legs", "four" );
	combo->AddItem( L"Five Birds", "five" );
				
	Controls::Button* add_button = new Controls::Button( window );
	add_button->Dock(Pos::Bottom);
	add_button->SetText( L"Add Plugin" );
	add_button->UserData.Set<Gwen::Controls::ComboBox*>("combo", combo);
	add_button->onPress.Add( this, &ThisClass::OnAddPluginFinish );
}

void PubViz::OnAddPluginFinish(Gwen::Controls::Base* control)
{
	Gwen::Controls::ComboBox* combo = control->UserData.Get<Gwen::Controls::ComboBox*>("combo");
	AddPlugin(combo->GetSelectedItem()->GetName());
	control->GetParent()->DelayedDelete();
}

void PubViz::OnRemovePlugin(Gwen::Controls::Base* control)
{
	Plugin* plugin = control->UserData.Get<Plugin*>("plugin");
	
	// remove the properties for this plugin
	plugin->props_->GetParent()->DelayedDelete();
	
	// now lets remove it
	std::vector<Plugin*> plugins;
	for (auto p: plugins_)
	{
		if (p != plugin)
		{
			plugins.push_back(p);
		}
	}
	plugins_ = plugins;
	canvas_->plugins_ = plugins;
	
	delete plugin;
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

