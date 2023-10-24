// Todo license

#ifndef PUBVIZ_CANVAS_H
#define PUBVIZ_CANVAS_H

#include <Gwen/Controls/Base.h>
#include <Gwen/Controls/Label.h>
#include <Gwen/Controls/Menu.h>
#include <Gwen/Controls/TreeControl.h>
#include <Gwen/Controls/TabControl.h>
#include <Gwen/Gwen.h>
#include <Gwen/Skin.h>

#include <pubsub/Node.h>
#include <pubsub/Publisher.h>
#include <pubsub/Serialization.h>

#include <thread>

#include "SackGraph.h"


class PubViz;
class SackViewer : public Gwen::Controls::Base
{
	friend class PubViz;

	// used to select next message
	std::string selected_message_topic_;
	int selected_message_index_;

public:

	GWEN_CONTROL( SackViewer, Gwen::Controls::Base );
		
	~SackViewer();

	virtual void Render( Gwen::Skin::Base* skin );
		
	void OpenFile(const std::string& file);

    void Play();

    void Pause();

    void LoopPlayback(bool yn)
    {
        loop_playback_ = yn;
    }

    void ShouldPublish(bool yn)
    {
        should_publish_ = yn;
    }

    struct Message
	{
		uint64_t time;
        uint64_t length;
		const char* msg;
	};

    uint64_t GetPlayheadTime() { return playhead_time_; }
    uint64_t GetStartTime() { return start_time_; }
    uint64_t GetEndTime() { return end_time_; }

    void SetPlayheadTime(uint64_t time)
    {
        if (time < start_time_)
        {
            playhead_time_ = start_time_;
        }
        else if (time > end_time_)
        {
            playhead_time_ = end_time_;
        }
        else
        {
            playhead_time_ = time;
        }

        reseek_ = true;

		UpdateViewers();
    }

protected:
		
	struct Stream
	{
		std::vector<Message> messages;
		ps_message_definition_t def;

		bool latched;
        bool publisher_initialized;
        ps_pub_t publisher;
			
		Stream()
		{
			def.name = 0;
            publisher_initialized = false;
			latched = false;
		}
	};
	std::map<std::string, Stream> bag_data_;
	uint64_t start_time_ = 0, end_time_ = 0;// in uS
	uint64_t playhead_time_ = 0;

    // Used if we enable playback mode
    struct IndexedMessage
    {
        uint64_t time;
        uint64_t length;
        Stream* channel;
        const char* msg;
    };
    std::vector<IndexedMessage> index_messages_;
    ps_node_t node_;
    bool node_initialized_ = false;
    bool run_thread_ = false;
    bool playing_ = false;
    bool reseek_ = false;
    bool loop_playback_ = false;
    bool should_publish_ = false;
    std::thread playback_thread_;

    struct Viewer
    {
        Gwen::Controls::TabButton* first;
        Gwen::Controls::TreeControl* second;
        int current_message;
    };
		
	std::map<std::string, Viewer> viewers_;

	std::map<SackGraph*, bool> graphs_;

	inline Gwen::Controls::DockBase* GetParentDock() { return (Gwen::Controls::DockBase*)GetParent()->GetParent(); }
	
	void OnMouseMoved(int x, int y, int dx, int dy) override;
	bool OnMouseWheeled( int iDelta ) override;
	void OnMouseClickLeft( int /*x*/, int /*y*/, bool /*bDown*/ ) override;
	void OnMouseClickRight( int x, int y, bool bDown) override;
		
	void OnMenuItemSelect(Gwen::Controls::Base* pControl);
	void OnViewerClose(Gwen::Controls::Base* pControl);
	void OnGraphClose(Gwen::Controls::Base* pControl);

    void OnPreviousMessage(Gwen::Controls::Base* control);
    void OnNextMessage(Gwen::Controls::Base* control);
    void OnFirstMessage(Gwen::Controls::Base* control);
    void OnLastMessage(Gwen::Controls::Base* control);

    void OnFieldRightClick(Gwen::Controls::Base* pControl);

    void Layout(Gwen::Skin::Base* skin);
		
public:

	const std::vector<Message>& GetTopicData(const std::string& topic)
	{
		return bag_data_[topic].messages;
	}

	const ps_message_definition_t* GetTopicDefinition(const std::string& topic)
	{
		return &bag_data_[topic].def;
	}

	void UpdateViewers();

	std::string SerializeConfig();

	void PlotSelected(bool twod);

	void AddPlot(bool twod, std::vector<std::pair<std::string, std::string>> plots);

	void UpdateSelection(double min_x, double max_x);

	Gwen::Point GetMinimumSize() override { return Gwen::Point(50, bag_data_.size()*40 + 45); }
protected:
    void CloseBag();

	bool mouse_down_ = false;
		
	double x_mouse_position_ = 0.0;
	double y_mouse_position_ = 0.0;
};

#endif
