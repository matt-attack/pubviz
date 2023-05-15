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


class PubViz;
class SackViewer : public Gwen::Controls::Base
{
	friend class PubViz;
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

		UpdateViewers();
    }

protected:
		
	struct Stream
	{
		std::vector<Message> messages;
		ps_message_definition_t def;

        bool publisher_initialized;
        ps_pub_t publisher;
			
		Stream()
		{
			def.name = 0;
            publisher_initialized = false;
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
		
	std::map<std::string, std::pair<Gwen::Controls::TabButton*, Gwen::Controls::TreeControl*>> viewers_;
		
	//Gwen::Controls::Menu* context_menu_;
	
	void OnMouseMoved(int x, int y, int dx, int dy) override;
	bool OnMouseWheeled( int iDelta ) override;
	void OnMouseClickLeft( int /*x*/, int /*y*/, bool /*bDown*/ ) override;
	void OnMouseClickRight( int x, int y, bool bDown) override;
		
	void OnMenuItemSelect(Gwen::Controls::Base* pControl);
	void OnViewerClose(Gwen::Controls::Base* pControl);

    void OnFieldRightClick(Gwen::Controls::Base* pControl);

    void Layout(Gwen::Skin::Base* skin);
		
public:
	void UpdateViewers();

	void PlotSelected();
protected:
    void CloseBag();

	bool mouse_down_ = false;
		
	double x_mouse_position_ = 0.0;
	double y_mouse_position_ = 0.0;
};

#endif
