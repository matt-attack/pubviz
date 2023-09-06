// todo license

#include <pubsub/TCPTransport.h>

#include "Gwen/Anim.h"
#include <Gwen/Platform.h>

#include <Gwen/Controls/DockBase.h>
#include <Gwen/Controls/TabControl.h>

#include "SackViewer.h"

#include <GL/glew.h>

#ifndef _WIN32
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#endif

#include <cmath>
#include <limits>

#include <rucksack/rucksack.h>

#undef min
#undef max

using namespace Gwen;
using namespace Gwen::Controls;

static ps_transport_t _tcp_transport;


GWEN_CONTROL_CONSTRUCTOR( SackViewer )
{

}

SackViewer::~SackViewer()
{
    if (run_thread_)
    {
        run_thread_ = false;
        playback_thread_.join();
    }

	for (auto& viewer: viewers_)
	{
        // todo delete string names in our callback
		viewer.second.first->onClose.RemoveHandler(this);
		viewer.second.first->Close();
	}
    viewers_.clear();

    CloseBag();

    if (node_initialized_)
    {
	    ps_node_destroy(&node_);
    }
}

#include "SackGraph.h"

void SackViewer::OnFieldRightClick(Gwen::Controls::Base* pControl)
{
    auto item = (Gwen::Controls::TreeNode*)pControl;

    //auto topic = item->UserData.Get<std::string>("topic");
    //auto field = item->UserData.Get<std::string>("field");
	auto parent = item->UserData.Get<Gwen::Controls::TreeNode*>("base");

    //printf("Want to plot: %s\n", topic.c_str());
	auto menu = new Gwen::Controls::Menu(GetCanvas());
	menu->AddItem("Plot")->SetAction(this, &ThisClass::OnMenuItemSelect);
	menu->AddItem("Plot 2D")->SetAction(this, &ThisClass::OnMenuItemSelect);
    menu->SetDeleteOnClose(true);
	menu->Show();

    auto canvas = GetCanvas();
    Gwen::Point mp = Gwen::Input::GetMousePosition();
    mp -= canvas->WindowPosition();
    mp.x /= canvas->Scale();
    mp.y /= canvas->Scale();

    menu->SetPos(mp);
}

void SackViewer::CloseBag()
{
    // if we have a playback thread, stop it
    if (node_initialized_ && run_thread_)
    {
        run_thread_ = false;
        playback_thread_.join();
    }

	// free any data we might have loaded
	for (auto& arr: bag_data_)
	{
		for (const auto& msg: arr.second.messages)
		{
			delete[] msg.msg;
		}

        if (arr.second.publisher_initialized)
        {
            // kill the publisher
            ps_pub_destroy(&arr.second.publisher);
        }

		ps_free_message_definition(&arr.second.def);
	}
    bag_data_.clear();
	
    index_messages_.clear();
}

class UpdateAnim: public Gwen::Anim::Animation
{
    SackViewer* control_;
public:
	
	UpdateAnim(SackViewer* c) { control_ = c;}
    virtual void Think() { control_->UpdateViewers(); };
    virtual bool Finished() { return false; }
};

void SackViewer::Play()
{
    if (!node_initialized_)
    {
        // startup the node
        node_initialized_ = true;
	    ps_node_init(&node_, "sack_viewer", "", true);

        // Adds TCP transport
        ps_tcp_transport_init(&_tcp_transport, &node_);
        ps_node_add_transport(&node_, &_tcp_transport);
    }

    if (!run_thread_)
    {
        // index all the messages
        index_messages_.clear();

        index_messages_.reserve(10000);// to prevent most resizes

        // index! (and create pubs)
        for (auto& topic: bag_data_)
        {
            // create publisher
			ps_node_create_publisher(&node_, topic.first.c_str(), &topic.second.def, &topic.second.publisher, topic.second.latched);
            topic.second.publisher_initialized = true;

            // add messages
            for (auto& msg: topic.second.messages)
            {
                IndexedMessage m;
                m.time = msg.time;
                m.length = msg.length;
                m.channel = &topic.second;
                m.msg = msg.msg;
                index_messages_.push_back(m);
            }
        }

        // sort
	    std::sort(index_messages_.begin(), index_messages_.end(),
		    [](const IndexedMessage& a, const IndexedMessage& b) -> bool
	    {
		    return a.time < b.time;
	    });

        // start the playback thread
        reseek_ = true;
        run_thread_ = true;
        playback_thread_ = std::thread([this]()
        {
            uint64_t playback_position = 0;
            while (run_thread_)
            {
                if (reseek_)
                {
                    reseek_ = false;
                    // find the start position
                    playback_position = 0;
                    while (index_messages_[playback_position].time < playhead_time_)
                    {
                        playback_position++;
                    }
                }

                if (!playing_)
                {
                    ps_node_spin(&node_);
                    ps_sleep(10);
                    continue;
                }

                // Advance the playhead
                auto new_time = playhead_time_ + 10000;
                if (new_time > end_time_)
                {
                    if (loop_playback_)
                    {
                        playhead_time_ = start_time_;
                        playback_position = 0;
                        continue;
                    }
                    playhead_time_ = end_time_;
                    ps_node_spin(&node_);
                    ps_sleep(10);
                    continue;
                }
                playhead_time_ = new_time;

                // actually play the bag!
                while (true)
                {
                    auto& hdr = index_messages_[playback_position];
                    if (new_time >= hdr.time)
                    {
                        if (should_publish_)
                        {
                            // lets cheat for the moment, and just publish everything before this time
                            ps_msg_t msg;
		                    ps_msg_alloc(hdr.length, 0, &msg);
        		            memcpy(ps_get_msg_start(msg.data), hdr.msg, hdr.length);
		                    ps_pub_publish(&hdr.channel->publisher, &msg);
                        }
                        playback_position++;
                    }
                    else
                    {
                        break;
                    }
                }

                ps_node_spin(&node_);
                ps_sleep(10);
            }
        });
    }

    playing_ = true;
    Gwen::Anim::Add(this, new UpdateAnim(this));

    Invalidate();
}

void SackViewer::Pause()
{
    Gwen::Anim::Cancel(this);

    playing_ = false;
}

void SackViewer::Layout(Gwen::Skin::Base* skin)
{   
    // handle 
	BaseClass::Layout(skin);
}

void SackViewer::OnMenuItemSelect(Gwen::Controls::Base* pControl)
{
	Gwen::Controls::MenuItem* pMenuItem = (Gwen::Controls::MenuItem*) pControl;
	if (pMenuItem->GetText() == L"View")
	{
		int index = pMenuItem->GetParent()->UserData.Get<int>("message_index");
		//todo do something besides this to get which viewer its for
		// create a new viewer
		//int index = (CanvasPosToLocal(pMenuItem->GetParent()->GetPos()).y-40)/40;
		
		std::string title; int i = 0;
		for (const auto& row: bag_data_)
		{
			if (i == index)
			{
				title = row.first;
				break;
			}
			i++;
		}

		if (viewers_.find(title) != viewers_.end())
		{
			return;
		}
		
		auto button = GetParentDock()->GetRight()->GetTabControl()->AddPage(title);
		button->SetPopoutable(true);
		button->SetClosable(true);
		auto page = button->GetPage();
		button->UserData.Set<std::string>("title", title);
		button->onClose.Add(this, &ThisClass::OnViewerClose);

		auto topbar = new Gwen::Controls::Base(page);
		topbar->Dock(Pos::Bottom);
		topbar->UserData.Set<std::string>("topic", title);

		auto button2 = new Gwen::Controls::Button(topbar);
		button2->SetText("|<");
		button2->SetToolTip("First Message");
		button2->SetSize(25, 25);
		button2->Dock(Pos::Left);
		button2->onDown.Add(this, &ThisClass::OnFirstMessage);
		auto button22 = new Gwen::Controls::Button(topbar);
		button22->SetText("<");
		button22->SetToolTip("Previous Message");
		button22->SetSize(25, 25);
		button22->Dock(Pos::Left);
		button22->onDown.Add(this, &ThisClass::OnPreviousMessage);


		auto button222 = new Gwen::Controls::Button(topbar);
		button222->SetText(">|");
		button222->SetToolTip("Last Message");
		button222->SetSize(25, 25);
		button222->Dock(Pos::Right);
		button222->onDown.Add(this, &ThisClass::OnLastMessage);
		auto button3 = new Gwen::Controls::Button(topbar);
		button3->SetText(">");
		button3->SetToolTip("Next Message");
		button3->SetSize(25, 25);
		button3->Dock(Pos::Right);
		button3->onDown.Add(this, &ThisClass::OnNextMessage);

		topbar->SizeToChildren();
				
		auto tree = new Gwen::Controls::TreeControl( page );
		tree->Dock(Pos::Fill);
		tree->AllowMultiSelect( true );
		tree->SetBounds( 240, 30, 200, 200 );
		tree->ExpandAll();
		viewers_[title] = {button, tree};
		button->OnPress();// set this tab as active
		
		UpdateViewers();
	}
	else if (pMenuItem->GetText() == L"Plot")
	{
		PlotSelected(false);
	}
	else if (pMenuItem->GetText() == L"Plot 2D")
	{
		PlotSelected(true);
	}
}

void SackViewer::OnPreviousMessage(Gwen::Controls::Base* control)
{
	std::string topic = control->GetParent()->UserData.Get<std::string>("topic");
	auto& data = bag_data_[topic];
	auto& viewer = viewers_[topic];

	if (data.messages.size() == 0)
	{
		return;
	}
	
	SetPlayheadTime(data.messages[std::max(viewer.current_message - 1, 0)].time);
}

void SackViewer::OnNextMessage(Gwen::Controls::Base* control)
{
	std::string topic = control->GetParent()->UserData.Get<std::string>("topic");
	auto& data = bag_data_[topic];
	auto& viewer = viewers_[topic];

	if (data.messages.size() == 0)
	{
		return;
	}

	SetPlayheadTime(data.messages[std::min<int>(viewer.current_message + 1, data.messages.size() - 1)].time);
}

void SackViewer::OnLastMessage(Gwen::Controls::Base* control)
{
	std::string topic = control->GetParent()->UserData.Get<std::string>("topic");
	auto& data = bag_data_[topic];

	if (data.messages.size() == 0)
	{
		return;
	}

	auto time = data.messages[data.messages.size() - 1].time;
	SetPlayheadTime(time);
}

void SackViewer::OnFirstMessage(Gwen::Controls::Base* control)
{
	std::string topic = control->GetParent()->UserData.Get<std::string>("topic");
	auto& data = bag_data_[topic];

	if (data.messages.size() == 0)
	{
		return;
	}

	auto time = data.messages[0].time;
	SetPlayheadTime(time);
}

void SackViewer::AddPlot(bool twod, std::vector<std::pair<std::string, std::string>> plots)
{
	if (twod && plots.size() != 2)
	{
		// cant do it
		return;
	}
	else if (twod)
	{
		// todo display a prompt asking what is x and y
		// for now just go in order
	}

	auto button = GetParentDock()->GetRight()->GetTabControl()->AddPage("Graph");
	button->SetPopoutable(true);
	button->SetClosable(true);
	auto page = button->GetPage();
	auto graph = new SackGraph(page);
    graph->SetViewer(this);
	graph->Dock(Pos::Fill);
    page->GetParent()->GetParent()->SetWidth(580);
	button->GetTabControl()->SelectTab(button);
	button->UserData.Set<SackGraph*>("graph", graph);
	button->onClose.Add(this, &ThisClass::OnGraphClose);
	graphs_[graph] = true;

	if (twod)
	{
		// display a prompt asking 
		auto topic = plots[0].first;//selected.front()->UserData.Get<std::string>("topic");
		auto field_x = plots[0].second;//selected.front()->UserData.Get<std::string>("field");
		//auto iter = ++selected.begin();
		auto field_y = plots[1].second;//(*iter)->UserData.Get<std::string>("field");

		auto ch = graph->CreateChannel(topic, field_x, field_y);

		auto& data = bag_data_[topic];
		for (auto& msg : data.messages)
		{
			graph->AddMessageSample(ch, msg.time, msg.msg, &data.def, false, false);
		}

		return;
	}

	for (auto& item: plots)
	{
		auto topic = item.first;//item->UserData.Get<std::string>("topic");
   		auto field = item.second;//item->UserData.Get<std::string>("field");

   		//printf("Want to plot: %s:%s\n", topic.c_str(), field.c_str());

   		auto ch = graph->CreateChannel(topic, field);

   		auto& data = bag_data_[topic];
   		for (auto& msg: data.messages)
   		{
       		graph->AddMessageSample(ch, msg.time, msg.msg, &data.def, false, false);
   		}
	}

	graphs_[graph] = true;
}

void SackViewer::PlotSelected(bool twod)
{
	Gwen::Controls::TreeNode* parent = 0;
	for (auto& viewer: viewers_)
	{
		// todo this isnt the best if multiple are visible
		// maybe deselect when another is clicked on?
		if (viewer.second.second->Visible())
		{
			parent = viewer.second.second;
			break;
		}
	}

	if (parent == 0)
	{
		return;
	}

	auto selected = parent->GetSelectedChildNodes();

	std::vector<std::pair<std::string, std::string>> plots;
	for (auto& item: selected)
	{
		auto topic = item->UserData.Get<std::string>("topic");
   		auto field = item->UserData.Get<std::string>("field");
		plots.push_back({topic, field});
	}
	AddPlot(twod, plots);
}

void SackViewer::OnViewerClose(Gwen::Controls::Base* pControl)
{
	viewers_.erase(pControl->UserData.Get<std::string>("title"));
}

void SackViewer::OnGraphClose(Gwen::Controls::Base* pControl)
{
	auto graph = pControl->UserData.Get<SackGraph*>("graph");
	graphs_.erase(graph);
}

bool SackViewer::OnMouseWheeled( int delta )
{
	return true;
}

void SackViewer::OpenFile(const std::string& file)
{
	start_time_ = std::numeric_limits<uint64_t>::max();
	end_time_ = 0;

    CloseBag();
	
	rucksack::SackReader sack;
	if (!sack.open(file))
	{
		printf("Error opening file.\n");
		return;
	}
	
	rucksack::MessageHeader const* hdr;
	rucksack::SackChannelDetails const* info;
	while (const void* msg = sack.read(hdr, info))
	{
		start_time_ = std::min(start_time_, hdr->time);
		end_time_ = std::max(end_time_, hdr->time);
		
		// if we want to use the message later, we need to copy it
		char* cpy = new char[hdr->length];
		memcpy(cpy, (const char*)msg, hdr->length);
		auto& val = bag_data_[info->topic];
		val.messages.push_back({hdr->time, hdr->length, cpy});
		
		if (val.def.name == 0)
		{
			ps_copy_message_definition(&val.def, &info->definition);
			val.latched = info->latched;
		}
	}

    // close any viewers that arent relevant and update the rest
    auto viewers_copy = viewers_;
    for (auto& viewer: viewers_copy)
	{
        if (bag_data_.find(viewer.first) != bag_data_.end())
        {
            continue;
        }
        
        // todo delete string names in our callback
		viewer.second.first->onClose.RemoveHandler(this);
		viewer.second.first->Close();

        viewers_.erase(viewer.first);
	}
	
	playhead_time_ = start_time_;

    UpdateViewers();
}

#include <Gwen/Application.h>
void SackViewer::OnMouseClickRight(int x, int y, bool bDown)
{
	if (bDown)
	{
		auto pos = CanvasPosToLocal(Gwen::Point(x, y));
		if (pos.y > 40 && pos.y < bag_data_.size()*40 + 40)
		{
			auto current_win = (Gwen::Controls::WindowCanvas*)GetCanvas();
			auto wpos = current_win->WindowPosition();
			//auto window = Gwen::gApplication->AddWindow("", 1, 100, wpos.x + x, wpos.y + y, true);
			//window->SetRemoveWhenChildless(true);
//make a function which can pop this out automatically
	        auto menu = new Gwen::Controls::Menu(GetCanvas());
			menu->SetPos(x, y);
    	    menu->AddItem("View")->SetAction(this, &ThisClass::OnMenuItemSelect);
    	    menu->SetDeleteOnClose(true);
			menu->UserData.Set<int>("message_index", (pos.y-40)/40);
            menu->Show();
			//window->DoThink();
			auto size = menu->GetSize();
			//window->SetWindowSize(size.x, size.y);
		}
	}
}

std::string SackViewer::SerializeConfig()
{
	std::string config = "";
	int j = 0;
	for (auto& graph: graphs_)
	{
		config += graph.first->Is2D() ? "plot2d:" : "plot:";
		int i = 0;
		auto channels = graph.first->GetChannels();
		for (auto& plot: channels)
		{
			if (graph.first->Is2D())
			{
				config += plot->topic_name + "," + plot->field_name_x;
				config += "," + plot->topic_name + "," + plot->field_name_y;
			}
			else
			{
				config += plot->topic_name + "," + plot->field_name_y;
			}
			if (i != channels.size() - 1)
			{
				config += ",";
			}
			i++;
		}
		if (j != graphs_.size() - 1)
		{
			config += "\n";
		}
		j++;
	}
	return config;
}

void SackViewer::OnMouseClickLeft( int x, int y, bool down )
{
	mouse_down_ = down;
	
	OnMouseMoved(x, y, 0, 0);
}

void SackViewer::OnMouseMoved(int x, int y, int dx, int dy)
{
	Gwen::Point lp = CanvasPosToLocal(Gwen::Point(x, y));
	if (!mouse_down_)
	{
		return;
	}
	
	if (!Gwen::Input::IsLeftMouseDown())
	{
		mouse_down_ = false;
		return;
	}
	
	// Reposition the play head
	const int title_width = 150;
	auto bounds = GetRenderBounds();
	int graph_width = bounds.w - title_width - 10;// for a bit of side padding
	const double duration_usec = end_time_ - start_time_;
	const double usec_per_px = duration_usec/(double)graph_width;
	playhead_time_ = static_cast<double>(lp.x - title_width)*usec_per_px + start_time_;
	playhead_time_ = std::max(start_time_, playhead_time_);
	playhead_time_ = std::min(end_time_, playhead_time_);

    reseek_ = true;

	Redraw();
	
	UpdateViewers();
}

int BinarySearch(uint64_t target_time, const std::vector<SackViewer::Message>& array)
{
	SackViewer::Message msg;
	msg.time = 0;
	msg.msg = 0;
	
	int left = 0; int right = array.size() - 1;
	while (left <= right)
	{
		int middle = (left + right)/2;
		if (array[middle].time == target_time)
		{
			return middle;
		}
		if (target_time > array[middle].time)
		{
			left = middle + 1;
		}
		else
		{
			right = middle - 1;
		}
	}
	
	if (left > array.size() - 1)
	{
		left = array.size() - 1;
	}
	
	// pick one less if we just overshot possible
	if (left != 0 && array[left].time > target_time)
		return left-1;
	else if (array[left].time < target_time)
		return left;

	// nothing was before this time
	return -1;
}

void SackViewer::UpdateSelection(double min_x, double max_x)
{
	for (auto& graph: graphs_)
	{
		graph.first->SetXRange(min_x, max_x);
	}
}

void SackViewer::UpdateViewers()
{
    auto current_time = playhead_time_;
	// Now update topic visualizations based on the new playhead position
	int i = 0;
	for (auto& t: bag_data_)
	{
		auto viewer = viewers_.find(t.first);
		if (viewer == viewers_.end())
		{
			continue;
		}
		
		// find the topic and visualize it
		auto topic = t.second;
		auto msg_index = BinarySearch(current_time, topic.messages);

		if (msg_index == viewer->second.current_message)
		{
			continue;
		}
		
		auto tree = viewer->second.second;
		viewer->second.current_message = msg_index;

		std::vector<std::string> items;
		for (auto node2: tree->GetChildNodes())
		{
			auto node = (Gwen::Controls::TreeNode*)node2;
			items.push_back(node->GetButton()->GetText().c_str());
		}

		tree->Clear();
		if (msg_index < 0)
		{
			continue;
		}

		auto msg = topic.messages[msg_index];

		// Add the timestamp
		std::string str = "timestamp: " + std::to_string(msg.time/1000000.0);
		auto node = tree->AddNode(str);
		node->SetSelectable(false);// not plottable, so dont allow selecting

        const int max_array_length = 100;
		
		int i = 1;
		struct ps_deserialize_iterator iter = ps_deserialize_start((const char*)msg.msg, &topic.def);
		const struct ps_msg_field_t* field; uint32_t length; const char* ptr;
		while (ptr = ps_deserialize_iterate(&iter, &field, &length))
		{
			// add each field
			std::string str = field->name;
			str += ": ";
			if (field->type == FT_String)
			{
				// strings are already null terminated
				str += '"';
				str += ptr;
				str += '"';
			}
			else
			{
				if (field->length == 1)
				{
					//printf("%s: ", field->name);
				}
                else if (length == 0)
                {
                    str += "[]";
                }
				else
				{
					str += "[";
				}

				for (unsigned int i = 0; i < length; i++)
				{
                    if (i > max_array_length)
                    {
                        str += "... ]";
                        break;
                    }

					// non dynamic types 
					switch (field->type)
					{
					case FT_Int8:
						str += std::to_string((int)*(int8_t*)ptr);
						ptr += 1;
						break;
					case FT_Int16:
						str += std::to_string((int)*(int16_t*)ptr);
						ptr += 2;
						break;
					case FT_Int32:
						str += std::to_string((int)*(int32_t*)ptr);
						ptr += 4;
						break;
					case FT_Int64:
						str += std::to_string((long int)*(int64_t*)ptr);
						ptr += 8;
						break;
					case FT_UInt8:
						str += std::to_string((int)*(uint8_t*)ptr);
						ptr += 1;
						break;
					case FT_UInt16:
						str += std::to_string((int)*(uint16_t*)ptr);
						ptr += 2;
						break;
					case FT_UInt32:
						str += std::to_string((unsigned int)*(uint32_t*)ptr);
						ptr += 4;
						break;
					case FT_UInt64:
						str += std::to_string((unsigned long int)*(uint64_t*)ptr);
						ptr += 8;
						break;
					case FT_Float32:
						str += std::to_string(*(float*)ptr);
						ptr += 4;
						break;
					case FT_Float64:
						str += std::to_string(*(double*)ptr);
						ptr += 8;
						break;
					default:
						printf("ERROR: unhandled field type when parsing....\n");
					}

					if (field->length == 1)
					{
						//printf("\n");
					}
					else if (i == length - 1)
					{
						str += "]";
					}
					else
					{
						str += ", ";
					}
				}
			}
			//color based on change from the last one
			auto node = tree->AddNode(str);
			if (i >= items.size() || str != items[i])
			{
				node->GetButton()->SetTextColorOverride(Gwen::Color(255, 0, 0, 255));
			}
            node->UserData.Set<std::string>("topic", t.first);
            node->UserData.Set<std::string>("field", field->name);
			node->UserData.Set<Gwen::Controls::TreeNode*>("base", tree);
			node->GetButton()->DragAndDrop_SetPackage(true, "topic");
            node->onRightPress.Add(this, &ThisClass::OnFieldRightClick);
			i++;
		}
		tree->ExpandAll();
	}
	Redraw();
}

std::string format_time(uint64_t dt, int period)
{
	if (period >= 1000000)
	{
		// minutes and seconds
		int min = dt/60000000;
		int sec = (dt/1000000)%60;
		if (sec < 10)
		{
			return std::to_string(min)+ "m0" + std::to_string(sec) + "s";
		}
		else
		{
			return std::to_string(min)+ "m" + std::to_string(sec) + "s";
		}
	}
	else
	{
		// just do milliseconds, probably need to do something else here
		return std::to_string(dt/1000) + "ms";
	}
}

const int num_periods = 15;
int periods[] = {1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000, 2000000, 5000000, 15000000, 30000000, 60000000};

void SackViewer::Render( Skin::Base* skin )
{
	auto r = skin->GetRender();
	
	// fill the background
	auto bounds = GetRenderBounds();
	r->SetDrawColor( Gwen::Color( 250, 250, 250, 255 ) );
	r->DrawFilledRect( bounds );

	if (end_time_ == 0 && start_time_ == 0)
	{
		return;
	}
	
	// Now draw all the topic names
	r->SetDrawColor( Gwen::Color(0,0,0,255) );
	int off = 40;// top padding
	for (auto& topic: bag_data_)
	{
		r->RenderText(skin->GetDefaultFont(), Gwen::PointF(10, off), topic.first);
		off += 40;
	}
	
	// now draw the bars on the right side for each
	const int title_width = 150;
	int graph_width = bounds.w - title_width - 10;// for a bit of side padding
	const double duration_usec = end_time_ - start_time_;
	const double px_per_usec = (double)graph_width / duration_usec;
	
	// draw timeline labels, finding the period which fits the most that fit thats less than max_divs
	const int min_div_width = 100;
	int max_divs = graph_width/min_div_width;
	int period_iter = num_periods - 1;
	int period = periods[period_iter];
	while (period_iter > 0)
	{	
		period_iter--;
		period = periods[period_iter];
		
		if (duration_usec/period > max_divs)
		{
			period_iter++;
			period = periods[period_iter];
			break;
		}
	}
	
	r->SetDrawColor( Gwen::Color(0,0,0,255) );
	off = 40;
	for (auto& topic: bag_data_)
	{
		r->SetDrawColor( Gwen::Color(0,0,200,255) );
		for (auto& msg : topic.second.messages)
		{
			double x = (msg.time - start_time_)*px_per_usec;
			r->DrawFilledRect(Rect(title_width + x, off, 2, 38));
		}

		r->SetDrawColor(Gwen::Color(0, 0, 0, 255));
		r->DrawLinedRect(Rect(title_width, off, graph_width, 38));
		
		off += 40;
	}
	
	// draw the playhead
	r->SetDrawColor( Gwen::Color(255, 0, 0, 255));
	double x = (playhead_time_ - start_time_)*px_per_usec;
	r->DrawFilledRect(Rect(title_width + x, 20, 4, bag_data_.size()*40 + 20));

	// Actually draw the time labels and bars
	r->SetDrawColor(Gwen::Color(0, 0, 0, 255));
	uint64_t time = start_time_;
	while (time < end_time_)
	{
		double x = (time - start_time_) * px_per_usec;
		r->RenderText(skin->GetDefaultFont(), Gwen::PointF(title_width + x + 5, 20), format_time(time - start_time_, period));
		time += period;
		r->DrawFilledRect(Rect(title_width + x, 20, 2, bag_data_.size() * 40 + 20));
	}
}
