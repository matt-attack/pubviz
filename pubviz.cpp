/*
	Copyright (c) 2022 Matthew Bries
	See license
	*/

#include <pubsub/TCPTransport.h>

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
#include <sys/types.h>
#include <stdio.h>

#include "Gwen/Controls/Properties.h"
#include "Gwen/Controls/PropertyTree.h"
#include "Gwen/Controls/Property/ColorSelector.h"
#include "Gwen/Controls/Property/Checkbox.h"
#include "Gwen/Controls/Property/ComboBox.h"
#include "Gwen/Controls/Property/Button.h"

#include "OpenGLCanvas.h"
#include "GraphCanvas.h"

#include "Parameters.h"

#include "plugins/Costmap.h"
#include "plugins/Grid.h"
#include "plugins/Marker.h"
#include "plugins/PointCloud.h"
#include "plugins/Pose.h"

#include <Gwen/Controls/Dialogs/FileOpen.h>
#include <Gwen/Controls/Dialogs/FileSave.h>


using namespace Gwen;

PubViz::~PubViz()
{
	ps_node_destroy(&node_);
}

void PubViz::OnConfigSave(Gwen::Event::Info info)
{
	std::string config;
	for (auto p: plugins_)
	{
		config += p->GetTitle();
		config += ":";
		config += p->GetConfiguration();
		config += "\n";
	}
	//printf("%s\n", config.c_str());
	
	FILE* f = fopen(info.String.c_str(), "wb");
	fwrite(config.c_str(), 1, config.length(), f);
	fclose(f);
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

void PubViz::OnConfigLoad(Gwen::Event::Info info)
{
	// first clear our plugins
	ClearPlugins();
	
	// now load the new config
	FILE *f = fopen(info.String.c_str(), "rb");
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
		
		auto plugin = AddPlugin(data[0]);
		if (plugin)
		{
			plugin->LoadConfiguration(data[1]);
		}
	}
}

void PubViz::ClearPlugins()
{
	std::vector<Plugin*> plugins;
	for (auto p: plugins_)
	{
		p->props_->GetParent()->DelayedDelete();
	}
	plugins_.clear();
	canvas_->plugins_.clear();
}

void PubViz::MenuItemSelect(Controls::Base* pControl)
{
	Gwen::Controls::MenuItem* pMenuItem = (Gwen::Controls::MenuItem*) pControl;
	if (pMenuItem->GetText() == L"Quit")
	{
		exit(0);
	}
	else if (pMenuItem->GetText() == L"Clear Plugins")
	{
		ClearPlugins();
	}
	else if (pMenuItem->GetText() == L"Plot")
	{
		auto button = GetRight()->GetTabControl()->AddPage("Graph");
		button->SetPopoutable(true);
		button->SetClosable(true);
		auto page = button->GetPage();
		auto graph = new GraphCanvas(page);
		graph->Dock(Pos::Fill);
	}
	else if (pMenuItem->GetText() == L"Save Config As")
	{
		Gwen::Dialogs::FileSave(true, String("Save Config"), String("pubviz.config"), String(".config|*.config|All|*.*"), this, &ThisClass::OnConfigSave);
	}
	else if (pMenuItem->GetText() == L"Load Config")
	{
		Gwen::Dialogs::FileOpen(true, String("Load Config"), String(""), String(".config|*.config|All|*.*"), this, &ThisClass::OnConfigLoad);
	}
	else if (pMenuItem->GetText() == L"Change Parameters")
	{
		if (parameters_page_)
		{
			return;
		}
		
		auto button = GetRight()->GetTabControl()->AddPage("Parameters");
		button->SetPopoutable(true);
		button->SetClosable(true);
		button->onClose.Add(this, &PubViz::OnParametersClose);
		auto page = button->GetPage();
		auto params = new Parameters(page);
		params->SetNode(&node_);
		params->Dock(Pos::Fill);
		parameters_page_ = params;
	}
	else if (pMenuItem->GetText() == L"Top Down")
	{
		canvas_->SetViewType(ViewType::TopDown);
	}
	else if (pMenuItem->GetText() == L"Orbit")
	{
		canvas_->SetViewType(ViewType::Orbit);
	}
	else if (pMenuItem->GetText() == L"Top")
	{
		canvas_->SetViewAngle(90, 0);
	}
	else if (pMenuItem->GetText() == L"Front")
	{
		canvas_->SetViewAngle(0, -90);
	}
	else if (pMenuItem->GetText() == L"Left")
	{
		canvas_->SetViewAngle(0, 180);
	}
}

void PubViz::Layout(Skin::Base* skin)
{
	BaseClass::Layout(skin);
	
	ps_node_spin(&node_);
	Invalidate();
}

static std::map<std::string, std::vector<std::string>> _topics;

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
		pRoot->GetMenu()->AddItem(L"Clear Plugins", "", "")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Load Config", "", "")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Save Config As", "", "Ctrl+S")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddDivider();
		pRoot->GetMenu()->AddItem(L"Quit", "", "Ctrl+Q")->SetAction(this, &ThisClass::MenuItemSelect);
	}
	{
		Gwen::Controls::MenuItem* pRoot = menu_->AddItem(L"View");
		pRoot->GetMenu()->AddItem(L"Plot", "", "Ctrl+P")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Change Parameters", "", "Shift+P")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddDivider();
		pRoot->GetMenu()->AddItem(L"Orbit", "", "Ctrl+O")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Top Down", "", "Ctrl+T")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddDivider();
		pRoot->GetMenu()->AddItem(L"Top", "", "")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Left", "", "")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Front", "", "")->SetAction(this, &ThisClass::MenuItemSelect);
	}	
				
	auto page = GetLeft()->GetTabControl()->AddPage("Plugins")->GetPage();
	
	// now add the properties bar for plugins
	plugin_tree_ = new Gwen::Controls::PropertyTree( page );
	plugin_tree_->Dock(Pos::Fill);
	plugin_tree_->SetBounds( 200, 10, 400, 200 );
	page->GetParent()->GetParent()->SetWidth(300);
	
	Gwen::Controls::Properties* props = plugin_tree_->Add( "Global Options" );
	props->SetSplitWidth(150);
	auto row = props->Add(L"Background Color", new Gwen::Controls::Property::ColorSelector(props), L"50 50 50");
	row->onChange.Add(this, &ThisClass::OnBackgroundChange);
	
	plugin_tree_->ExpandAll();
	
	//Controls::Button* remove_button = new Controls::Button( page );
	//remove_button->Dock(Pos::Bottom);
	//remove_button->SetText( L"Remove Plugin" );
	
	Controls::Button* add_button = new Controls::Button( page );
	add_button->Dock(Pos::Bottom);
	add_button->SetText( L"Add Plugin" );
	
	//ps_node_init_ex(&node_, "pubviz_real", "", false, false);
	ps_node_init(&node_, "pubviz_real", "", true);
	
	struct ps_transport_t tcp_transport;
    ps_tcp_transport_init(&tcp_transport, &node_);
    ps_node_add_transport(&node_, &tcp_transport);
	
	ps_node_system_query(&node_);

	node_.adv_cb = [](const char* topic, const char* type, const char* node, const ps_advertise_req_t* data)
	{
		// todo check if we already have the topic
		//if (_topics.find(topic) != _topics.end())
		//{
		//	return;
		//}

		//printf("Discovered topic %s..\n", topic);
		
		_topics[type].push_back(topic);
		//_topics[topic] = true;
	};
	
	// make a test for the graph
	/*{
		auto button = GetRight()->GetTabControl()->AddPage("Graph");
		button->SetClosable(true);
		auto page = button->GetPage();
		auto graph = new GraphCanvas(page);
		graph->Dock(Pos::Fill);
	}*/
	
	canvas_ = new OpenGLCanvas(this);
	canvas_->Dock(Pos::Fill);
	canvas_->plugins_ = plugins_;//todo lets not maintain two lists
	
	AddPlugin("grid");
		AddPlugin("costmap");
		AddPlugin("marker");
		AddPlugin("pointcloud");
		AddPlugin("pose");
	
	add_button->onPress.Add( this, &ThisClass::OnAddPlugin );

	m_StatusBar = new Controls::StatusBar(this->GetParent());
	m_StatusBar->Dock(Pos::Bottom);
	m_StatusBar->SendToBack();
	
	auto center_button = new Controls::Button(m_StatusBar);
	center_button->Dock(Pos::Right);
	center_button->SetText("Center");
	center_button->onPress.Add( this, &ThisClass::OnCenter );

	m_fLastSecond = Gwen::Platform::GetTimeInSeconds();
	m_iFrames = 0;
}

void PubViz::OnCenter(Gwen::Controls::Base* control)
{
	canvas_->ResetView();
}

void PubViz::OnBackgroundChange(Gwen::Controls::Base* control)
{
	auto prop = ((Gwen::Controls::PropertyRow*)control)->GetProperty();
	Gwen::Controls::Property::ColorSelector* selector = (Gwen::Controls::Property::ColorSelector*)prop;
	canvas_->m_Color = selector->m_Button->m_Color;
}

Plugin* PubViz::AddPlugin(const std::string& name)
{
	Plugin* plugin;
	if (name == "grid")
	{
		plugin = new GridPlugin();
	}
	else if (name == "costmap")
	{
		plugin = new CostmapPlugin();
	}
	else if (name == "marker")
	{
		plugin = new MarkerPlugin();
	}
	else if (name == "pointcloud")
	{
		plugin = new PointCloudPlugin();
	}
	else if (name == "pose")
	{
		plugin = new PosePlugin();
	}
	else
	{
		printf("Unknown plugin name '%s'!\n", name.c_str());
		return 0;
	}
	
	Gwen::Controls::Properties* props = plugin_tree_->Add( name );
	((Gwen::Controls::TreeNode*)props->GetParent())->Open();
	auto pRow = props->Add( L"Enable", new Gwen::Controls::Property::Checkbox( props ), L"1" );
	pRow->onChange.Add( plugin, &Plugin::OnEnableChecked );
	plugin->node_ = &node_;
	plugin->canvas_ = canvas_;
	plugin->Initialize(props);
	props->SetSplitWidth(150);
	plugin->props_ = props;
	
	// add the close button
	auto pRow2 = props->Add( L"", new Gwen::Controls::Property::Button( props ), L"Close" );
	pRow2->onChange.Add( this, &PubViz::OnRemovePlugin );
	pRow2->UserData.Set<Plugin*>("plugin", plugin);
	
	plugins_.push_back(plugin);
	canvas_->plugins_ = plugins_;
	
	return plugin;
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
	combo->AddItem( L"Costmap", "costmap" );
	combo->AddItem( L"Marker", "marker" );
	combo->AddItem( L"Point Cloud", "pointcloud" );
	combo->AddItem( L"Pose", "pose");
	
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

