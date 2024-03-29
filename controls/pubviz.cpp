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

#include "../plugins/Costmap.h"
#include "../plugins/Grid.h"
#include "../plugins/Marker.h"
#include "../plugins/PointCloud.h"
#include "../plugins/Pose.h"
#include "../plugins/Path.h"
#include "../plugins/Image.h"
#include "../plugins/GPS.h"
#include "../plugins/Dial.h"

#include <Gwen/Controls/Dialogs/FileOpen.h>
#include <Gwen/Controls/Dialogs/FileSave.h>

std::map<std::string, BaseRegisterObject*>& GetPluginList()
{
	static std::map<std::string, BaseRegisterObject*> list;
	return list;
}
using namespace Gwen;

void BaseRegisterObject::Add(const std::string& type, BaseRegisterObject* builder)
{
	GetPluginList()[type] = builder;
}

pubviz::Plugin* BaseRegisterObject::Construct(const std::string& type)
{
	auto builder = GetPluginList().find(type);
	if (builder == GetPluginList().end())
	{
		return 0;
	}
	auto obj = builder->second->Construct();
	obj->type_ = type;
	return obj;
}

PubViz::~PubViz()
{
	ClearPlugins();
	ps_node_destroy(&node_);
}

void PubViz::OnConfigSave(Gwen::Event::Info info)
{
	std::string config;
	// save our own configuration first
	config += "pubviz:";
	config += "show_config,";
	config += (show_config_->GetChecked() ? "true" : "false");
	config += ",show_selection,";
	config += (show_selection_->GetChecked() ? "true" : "false");
	config += ",show_status_bar,";
	config += (show_status_bar_->GetChecked() ? "true" : "false");
	config += ",window_width,";
	config += std::to_string(GetParent()->Width());
	config += ",window_height,";
	config += std::to_string(GetParent()->Height());
	for (auto p: properties_)
	{
		config += ",";
		config += p.first + ",";
		config += p.second->Serialize();
	}
	config += "\n";
	for (auto p: plugins_)
	{
		config += p->GetType();
		config += ":";
		config += p->GetConfiguration();
		config += "\n";
	}

	// save graphs
	for (auto g : graphs_)
	{
		config += "plot:";
		for (auto& ch : g.first->GetChannels())
		{
			if (ch->topic_name.length() == 0 || ch->field_name_y.length() == 0)
			{
				continue;
			}

			config += ch->topic_name + ":" + ch->field_name_y + ":";
		}
		config += "\n";
	}
	
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
	LoadConfig(info.String.c_str());
}

void PubViz::LoadConfig(const char* filename)
{
	// first clear our plugins
	ClearPlugins();
	
	// now load the new config
	FILE *f = fopen(filename, "rb");
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

		if (data[0] == "plot")
		{
			auto button = GetRight()->GetTabControl()->AddPage("Graph");
			button->SetPopoutable(true);
			button->SetClosable(true);
			auto page = button->GetPage();
			auto graph = new GraphCanvas(page);
			graph->canvas_ = canvas_;
			graph->Dock(Pos::Fill);
			graph->UserData.Set<Gwen::Controls::TabButton*>("button", button);
        	page->GetParent()->GetParent()->SetWidth(580);
			graphs_[graph] = true;
			for (int i = 1; i < data.size() - 1; i+= 2)
			{
				auto topic = data[i+0];
				auto field = data[i+1];

				// add each field
				graph->AddPlot(topic, field);
			}

			continue;
		}
		else if (data[0] == "pubviz")
		{
			auto pts = split(data[1], ",");
			if (pts.size() % 2 != 0)
			{
				printf("Invalid config\n");
				return;
			}
			double pitch = 0.0;
			double yaw = 0.0;
			for (int i = 0; i < pts.size(); i += 2)
			{
				auto key = pts[i];
				auto value = pts[i + 1];

				if (key == "show_config")
				{
					show_config_->SetChecked(value == "true");
				}
				else if (key == "show_selection")
				{
					show_selection_->SetChecked(value == "true");
				}
				else if (key == "show_status_bar")
				{
					show_status_bar_->SetChecked(value == "true");
				}
				else if (key == "window_width")
				{
					GetParent()->SetWidth(std::atoi(value.c_str()));
				}
				else if (key == "window_height")
				{
					GetParent()->SetHeight(std::atoi(value.c_str()));
				}
				else
				{
					// load to properties
					auto prop = properties_.find(key);
					if (prop != properties_.end())
					{
						prop->second->Deserialize(value);
					}
				}
			}
			continue;
		}
		printf("loaded plugin %s\n", data[0].c_str());
		auto plugin = AddPlugin(data[0]);
		if (plugin)
		{
			plugin->LoadConfiguration(data[1]);
		}
	}
}

void PubViz::ClearPlugins()
{
	std::vector<pubviz::Plugin*> plugins;
	for (auto p: plugins_)
	{
		p->props_->GetParent()->DelayedDelete();
		delete p;
	}
	plugins_.clear();
	canvas_->plugins_.clear();

	// clear graphs
	for (auto graph : graphs_)
	{
		auto button = graph.first->UserData.Get<Gwen::Controls::TabButton*>("button");
		button->Close();
	}
	graphs_.clear();
}

void PubViz::MenuItemSelect(Controls::Base* pControl)
{
	Gwen::Controls::MenuItem* pMenuItem = (Gwen::Controls::MenuItem*) pControl;
	if (pMenuItem->GetText() == L"Pause" || pMenuItem->GetText() == L"Go Live")
	{
		OnPause(0);
	}
	else if (pMenuItem->GetText() == L"Quit")
	{
		exit(0);
	}
	else if (pMenuItem->GetText() == L"Screenshot")
	{
		canvas_->Screenshot();
	}
	else if (pMenuItem->GetText() == L"Clear Plugins")
	{
		ClearPlugins();
	}
	else if (pMenuItem->GetText() == L"Clear History")
	{
		for (auto p: plugins_)
		{
			p->Clear();
		}
	}
	else if (pMenuItem->GetText() == L"Plot")
	{
		auto button = GetRight()->GetTabControl()->AddPage("Graph");
		button->SetPopoutable(true);
		button->SetClosable(true);
		auto page = button->GetPage();
		auto graph = new GraphCanvas(page);
		graph->canvas_ = canvas_;
		graph->Dock(Pos::Fill);
		graph->UserData.Set<Gwen::Controls::TabButton*>("button", button);
		button->onClose.Add(this, &ThisClass::OnGraphClosed);
        page->GetParent()->GetParent()->SetWidth(580);

		graphs_[graph] = true;
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
		//add a page
		auto button = GetRight()->GetTabControl()->AddPage("Parameters");
		button->SetPopoutable(true);
		button->SetClosable(true);
		button->onClose.Add(this, &PubViz::OnParametersClose);
		auto page = button->GetPage();
		auto params = new Parameters(page);
		params->SetNode(&node_);
		params->Dock(Pos::Fill);
		page->GetParent()->GetParent()->SetWidth(500);
		parameters_page_ = params;
	}
//add view options to global options
	else if (pMenuItem->GetText() == L"Top Down")
	{
		canvas_->SetViewType(ViewType::TopDown);
	}
	else if (pMenuItem->GetText() == L"Orbit")
	{
		canvas_->SetViewType(ViewType::Orbit);
	}
	else if (pMenuItem->GetText() == L"First Person")
	{
		canvas_->SetViewType(ViewType::FPS);
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
	else if (pMenuItem->GetText() == L"Reset")
	{
		canvas_->ResetView();
	}
	else if (pMenuItem->GetText() == L"Reset Origin")
	{
		canvas_->ResetOrigin();
	}
}

void PubViz::OnGraphClosed(Gwen::Controls::Base* base)
{
	graphs_.erase((GraphCanvas*)base->GetChild(0));
}

void PubViz::Layout(Skin::Base* skin)
{
	BaseClass::Layout(skin);
	
	ps_node_spin(&node_);
	Invalidate();
}

std::map<std::string, std::vector<std::string>> _topics;
std::map<std::string, bool> _found_topics;

GWEN_CONTROL_CONSTRUCTOR(PubViz)
{
	Dock(Pos::Fill);
	SetSize(1024, 768);
	SetPadding(Gwen::Margin(0, 0, 0, 0));

	//add menu bar
	menu_ = new Gwen::Controls::MenuStrip(this->GetParent());
	menu_->Dock(Pos::Top);
	{
		Gwen::Controls::MenuItem* pRoot = menu_->AddItem(L"File");
		pRoot->GetMenu()->AddItem(L"Clear Plugins", "", "")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Load Config", "", "Ctrl+O")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Save Config As", "", "Ctrl+S")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddDivider();
		pRoot->GetMenu()->AddItem(L"Screenshot", "", "")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddDivider();
		pRoot->GetMenu()->AddItem(L"Quit", "", "Ctrl+Q")->SetAction(this, &ThisClass::MenuItemSelect);
	}
	{
		Gwen::Controls::MenuItem* pRoot = menu_->AddItem(L"View");
		Gwen::Controls::MenuItem* pCheckable = pRoot->GetMenu()->AddItem( "Show Config" );
		pCheckable->SetCheckable( true );
		pCheckable->SetChecked( true );
		pCheckable->onCheckChange.Add(this, &ThisClass::OnShowConfigChanged);
		show_config_ = pCheckable;
		pCheckable = pRoot->GetMenu()->AddItem( "Show Selection" );
		pCheckable->SetCheckable( true );
		pCheckable->SetChecked( false );
		pCheckable->onCheckChange.Add(this, &ThisClass::OnShowSelectionChanged);
		show_selection_ = pCheckable;
		pCheckable = pRoot->GetMenu()->AddItem( "Show Status Bar" );
		pCheckable->SetCheckable( true );
		pCheckable->SetChecked( true );
		pCheckable->onCheckChange.Add(this, &ThisClass::OnShowStatusBarChanged);
		show_status_bar_ = pCheckable;
		pRoot->GetMenu()->AddDivider();
		pRoot->GetMenu()->AddItem(L"Plot", "", "Ctrl+P")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Change Parameters", "", "Shift+P")->SetAction(this, &ThisClass::MenuItemSelect);
		pause_item_ = pRoot->GetMenu()->AddItem(L"Pause", "", "")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Clear History", "", "")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddDivider();
		pRoot->GetMenu()->AddItem(L"Orbit", "", "Ctrl+O")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Top Down", "", "Ctrl+T")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"First Person", "", "Ctrl+T")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddDivider();
		pRoot->GetMenu()->AddItem(L"Top", "", "")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Left", "", "")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Front", "", "")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddDivider();
		pRoot->GetMenu()->AddItem(L"Reset", "", "")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Reset Origin", "", "")->SetAction(this, &ThisClass::MenuItemSelect);
	}

	auto spage = GetBottom()->GetTabControl()->AddPage("Selection")->GetPage();
	selection_ = new Gwen::Controls::TreeControl(spage);
	selection_->Dock(Pos::Fill);
	selection_->SetBounds( 200, 10, 400, 200 );
	spage->GetParent()->GetParent()->SetWidth(300);
	selection_->GetParent()->GetParent()->GetParent()->Hide();
	
	auto page = GetLeft()->GetTabControl()->AddPage("Plugins")->GetPage();
	
	// now add the properties bar for plugins
	plugin_tree_ = new Gwen::Controls::PropertyTree( page );
	plugin_tree_->Dock(Pos::Fill);
	plugin_tree_->SetBounds( 200, 10, 400, 200 );
	page->GetParent()->GetParent()->SetWidth(300);
	
	Gwen::Controls::Properties* props = plugin_tree_->Add( "Global Options" );
	props->SetSplitWidth(150);

	// todo move these into canvas
	auto bg_color = new ColorProperty(props, "Background Color", Gwen::Color(50, 50, 50));
	bg_color->onChange = [this](Gwen::Color c)
	{
		canvas_->m_Color = c;
	};
	properties_["Background Color"] = bg_color;

	auto wgs84 = new BooleanProperty(props, "WGS84 Frame", false);
	wgs84->onChange = [this](bool b)
	{
		canvas_->SetFrame(b);
	};
	properties_["WGS84 Frame"] = wgs84;

	auto origin = new BooleanProperty(props, "Show Origin", "true");
	origin->onChange = [this](bool b)
	{
		canvas_->ShowOrigin(b);
	};
	properties_["Show Origin"] = origin;

	plugin_tree_->ExpandAll();
	
	//Controls::Button* remove_button = new Controls::Button( page );
	//remove_button->Dock(Pos::Bottom);
	//remove_button->SetText( L"Remove Plugin" );
	
	Controls::Button* add_button = new Controls::Button( page );
	add_button->Dock(Pos::Bottom);
	add_button->SetText( L"Add Plugin" );
	
	//ps_node_init_ex(&node_, "pubviz_real", "", false, false);
	ps_node_init_ex(&node_, "pubviz_real", "", true, false);
	
	struct ps_transport_t tcp_transport;
    ps_tcp_transport_init(&tcp_transport, &node_);
    ps_node_add_transport(&node_, &tcp_transport);
	
	ps_node_system_query(&node_);

	node_.adv_cb = [](const char* topic, const char* type, const char* node, const ps_advertise_req_t* data)
	{
		// check if we already have the topic
		if (_found_topics.find(topic) != _found_topics.end())
		{
			return;
		}

		//printf("Discovered topic %s..\n", topic);
		
		_topics[type].push_back(topic);
		_found_topics[topic] = true;
	};
	
	canvas_ = new OpenGLCanvas(this);
	canvas_->Dock(Pos::Fill);
	canvas_->plugins_ = plugins_;//todo lets not maintain two lists
	canvas_->SetFrame(false);

	// add canvas specific options
	auto props2 = canvas_->CreateProperties(props);
	for (auto p: props2)
	{
		properties_[p.first] = p.second;
	}
	
	//AddPlugin("image");

	AddPlugin("dial");
	AddPlugin("grid");
	AddPlugin("gps");
		AddPlugin("costmap");
		AddPlugin("marker");
		AddPlugin("pointcloud");
		AddPlugin("path");
		AddPlugin("pose");
	
	add_button->onPress.Add( this, &ThisClass::OnAddPlugin );

	m_StatusBar = new Controls::StatusBar(this->GetParent());
	m_StatusBar->Dock(Pos::Bottom);
	m_StatusBar->SendToBack();
	
	auto center_button = new Controls::Button(m_StatusBar);
	center_button->Dock(Pos::Right);
	center_button->SetText("Center");
	center_button->onPress.Add( this, &ThisClass::OnCenter );

	pause_button_ = new Controls::Button(m_StatusBar);
	pause_button_->Dock(Pos::Right);
	pause_button_->SetText("Pause");
	pause_button_->onPress.Add( this, &ThisClass::OnPause );
	canvas_->SetPaused(false);

	m_fLastSecond = Gwen::Platform::GetTimeInSeconds();
	m_iFrames = 0;
}

void PubViz::OnShowConfigChanged(Gwen::Controls::Base* control)
{
    auto item = (Gwen::Controls::MenuItem*)control;

    // eh, this isnt great but mostly works
    auto tree = plugin_tree_->GetParent()->GetParent()->GetParent();

    if (item->GetChecked())
    {
        tree->Show();
    }
    else
    {
        tree->Hide();
    }
}

void PubViz::OnShowSelectionChanged(Gwen::Controls::Base* control)
{
    auto item = (Gwen::Controls::MenuItem*)control;

    // eh, this isnt great but mostly works
    auto tree = selection_->GetParent()->GetParent()->GetParent();

    if (item->GetChecked())
    {
        tree->Show();
    }
    else
    {
        tree->Hide();
    }
}

void PubViz::OnShowStatusBarChanged(Gwen::Controls::Base* control)
{
    auto item = (Gwen::Controls::MenuItem*)control;

    if (item->GetChecked())
    {
        m_StatusBar->Show();
    }
    else
    {
        m_StatusBar->Hide();
    }
}

void PubViz::OnPause(Gwen::Controls::Base* control)
{
    canvas_->SetPaused(!canvas_->Paused());
    if (canvas_->Paused())
    {
        pause_button_->SetText("Go Live");
        pause_item_->SetText("Go Live");
    }
    else
    {
        pause_button_->SetText("Pause");
        pause_item_->SetText("Pause");
    }
}

void PubViz::OnCenter(Gwen::Controls::Base* control)
{
	canvas_->ResetView();
}

pubviz::Plugin* PubViz::AddPlugin(const std::string& name)
{
	auto plugin = BaseRegisterObject::Construct(name);
	if (plugin == 0)
	{
		printf("Unknown plugin name '%s'!\n", name.c_str());
		return 0;
	}

	Gwen::Controls::Properties* props = plugin_tree_->Add( name );
	
	auto node = (Gwen::Controls::TreeNode*)props->GetParent();
	node->Open();
	plugin->node_ = &node_;
	plugin->plugin_button_ = node->GetButton();
	plugin->canvas_ = canvas_;

	//use this for enable then make sure to save it
	auto box = new Gwen::Controls::CheckBox(node->GetButton());
	box->Dock(Pos::Right);
	box->SetChecked(true);
	box->SetMargin(Padding(0, 0, 5, 0));

	plugin->tree_node_ = node;
	plugin->enabled_ = box;
	plugin->Initialize(props);
	props->SetSplitWidth(150);
	plugin->props_ = props;

	node->Position(Pos::Right);
	
	// add the close button
	auto pRow2 = props->Add( L"", new Gwen::Controls::Property::Button( props ), L"Close" );
	pRow2->onChange.Add( this, &PubViz::OnRemovePlugin );
	pRow2->UserData.Set<pubviz::Plugin*>("plugin", plugin);
	
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
	for (auto plugin: GetPluginList())
	{
		auto thing = plugin.second->Construct();
		combo->AddItem(Gwen::Utility::StringToUnicode(thing->GetTitle()), plugin.first);
		//delete thing;
	}
	
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
	auto plugin = control->UserData.Get<pubviz::Plugin*>("plugin");
	
	// remove the properties for this plugin
	plugin->props_->GetParent()->DelayedDelete();
	
	// now lets remove it
	std::vector<pubviz::Plugin*> plugins;
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

	if (canvas_->GetViewType() == ViewType::TopDown)
	{
		double x, y;
		canvas_->GetMousePosition(x, y);
		m_StatusBar->SetText(Gwen::Utility::Format(L"%i fps    X: %f Y: %f", val * 2, x, y));
	}
	else
	{
		if (canvas_->wgs84_mode_)
		{
			m_StatusBar->SetText(Gwen::Utility::Format(L"%i fps    Lat: %f Lon: %f Alt: %f", val * 2, canvas_->view_lat_, canvas_->view_lon_, canvas_->view_alt_));
		}
		else
		{
			m_StatusBar->SetText(Gwen::Utility::Format(L"%i fps    X: %f Y: %f Z: %f", val * 2, canvas_->view_x_, canvas_->view_y_, canvas_->view_z_));
		}
	}

	BaseClass::Render(skin);
}

