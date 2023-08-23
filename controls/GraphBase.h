// Todo license

#ifndef PUBVIZ_GRAPH_BASE_H
#define PUBVIZ_GRAPH_BASE_H

#include <Gwen/Controls/Base.h>
#include <Gwen/Controls/Label.h>
#include <Gwen/Controls/ListBox.h>
#include <Gwen/Gwen.h>
#include <Gwen/Skin.h>

#include <deque>
#include <vector>
#include <cmath>

#include <pubsub_cpp/Time.h>

#include <pubsub/Serialization.h>

#include <pubsub_cpp/Node.h>

extern const float graph_colors[6][3];

struct DataSample
{
	double first;
	double second;
	pubsub::Time time;
	double raw_second;
};

enum class OpCode
{
	X = 0,
	Num = 1,
	Add = 2,
	Sub = 3,
	Mul = 4,
	Div = 5
};

struct Operation
{
	OpCode code;
	double number;
};

class PubViz;
class GraphBase : public Gwen::Controls::Base
{
    Gwen::Controls::Button remove_button_;
    Gwen::Controls::Button configure_button_;

	friend class PubViz;
		
public:
    class Channel
    {
    public:
        friend class GraphBase;
        std::string topic_name;
		std::string field_name_x;// optional
		std::string field_name_y;
        std::deque<DataSample> data;

        bool can_remove = true;
        std::function<void()> on_remove;

		std::string GetTitle()
		{
			if (field_name_x.length())
			{
				return topic_name + "[" + field_name_x + "," + field_name_y + "]";
			}
			else
			{
				if (math_string_.length())
					return topic_name + "." + field_name_y + " (" + math_string_ + ")";
				return topic_name + "." + field_name_y;
			}
		}
		bool hidden = false;


		std::vector<Operation> math_;
		std::string math_string_;

		void SetMath(const std::string& str)
		{
			// tokenize
			std::vector<std::string> tokens;
			std::string current_token;
			int i = 0;
			while (i < str.length())
			{
				// todo break apart symbols
				char c = str[i];
				if (c == ' ' || c == '\t' || c == '\n')
				{
					if (current_token.length())
					{
						tokens.push_back(current_token);
					}
					current_token.clear();
				}
				else
				{
					current_token += c;
				}
	
				i++;
			}
			if (current_token.length())
			{
				tokens.push_back(current_token);
			}

			// now turn into math
			std::vector<Operation> math;
			for (const auto& tok: tokens)
			{
				if (tok == "+")
				{
					Operation op;
					op.code = OpCode::Add;
					math.push_back(op);
				}
				else if (tok == "*")
				{
					Operation op;
					op.code = OpCode::Mul;
					math.push_back(op);
				}
				else if (tok == "-")
				{
					Operation op;
					op.code = OpCode::Sub;
					math.push_back(op);
				}
				else if (tok == "x")
				{
					Operation op;
					op.code = OpCode::X;
					math.push_back(op);
				}
				else
				{
					Operation op;
					op.code = OpCode::Num;
					op.number = std::atof(tok.c_str());
					math.push_back(op);
				}
			}
			math_ = math;
			math_string_ = str;

			// re-evaluate
			for (auto& sample: data)
			{
				sample.second = Evaluate(sample.raw_second);
			}
		}

		double Evaluate(double input)
		{
			if (math_.size() == 0)
			{
				return input;
			}
	
			std::deque<double> stack;
			for (const auto& op: math_)
			{
				if (op.code == OpCode::Num)
				{
					stack.push_back(op.number);
				}
				else if (op.code == OpCode::X)
				{
					stack.push_back(input);
				}
				else if (op.code == OpCode::Mul)
				{
					if (stack.size() < 2)
					{
						return NAN;
					}
					double one = stack.back();
					stack.pop_back();
					double two = stack.back();
					stack.pop_back();
					stack.push_back(one*two);
				}
				else if (op.code == OpCode::Add)
				{
					if (stack.size() < 2)
					{
						return NAN;
					}
					double one = stack.back();
					stack.pop_back();
					double two = stack.back();
					stack.pop_back();
					stack.push_back(one+two);
				}
				else if (op.code == OpCode::Div)
				{
					if (stack.size() < 2)
					{
						return NAN;
					}
					double one = stack.back();
					stack.pop_back();
					double two = stack.back();
					stack.pop_back();
					stack.push_back(one/two);
				}
				else if (op.code == OpCode::Sub)
				{
					if (stack.size() < 2)
					{
						return NAN;
					}
					double one = stack.back();
					stack.pop_back();
					double two = stack.back();
					stack.pop_back();
					stack.push_back(one-two);
				}
			}
			return stack.back();
		}
    };

	GWEN_CONTROL( GraphBase, Gwen::Controls::Base );
		
	~GraphBase()
	{
		for (auto& sub: channels_)
		{
			delete sub;
		}
	}

	std::vector<Channel*>& GetChannels()
	{
		return channels_;
	}

	virtual void Render( Gwen::Skin::Base* skin );

	const Gwen::Color & GetColor() { return m_Color; }
	void SetColor( const Gwen::Color & col ) { m_Color = col; }
		
    void AddSample(Channel* channel, double value, pubsub::Time time, bool scroll_to_fit, bool remove_old);

    void AddMessageSample(Channel* channel, pubsub::Time time, const void* message, const ps_message_definition_t* def, bool scroll_to_fit, bool remove_old);

	// 2D Plot
	Channel* CreateChannel(const std::string& topic, const std::string& field_x, const std::string& field_y);

	// 1D Time Plot
    Channel* CreateChannel(const std::string& topic, const std::string& field);

    virtual void DrawOnGraph(double start_x, double start_y, double graph_width, double graph_height) {}
	virtual void PaintOnGraph(double start_x, double start_y, double graph_width, double graph_height) {};

    double GraphStartPosition() { return 80; };

    double GraphWidth()
    {
       	const int left_padding = 80;
	    const int other_padding = 50;
	
	    // target grid cell size
	    const int pixel_interval = 100;
	
	    // Calculate size to make the graph
	    Gwen::Rect b = GetRenderBounds();
	    return b.w - left_padding - other_padding;
    }

	void RecalculateScale()
	{
		redo_scale_ = true;
	}

	void SetXRange(double min, double max)
	{
		if (is_2d_) { return; }
		autoscale_x_ = false;
		min_x_ = min;
		max_x_ = max;
	}

	inline bool Is2D()
	{
		return is_2d_;
	}

protected:

    void Layout(Gwen::Skin::Base* skin);
	
	void OnRemove(Base* control);
    void OnRemoveSelect(Gwen::Controls::Base* pControl);

    void OnConfigure(Base* control);
	
	void OnConfigureChannel(Channel* channel);

	void OnConfigureClosed(Gwen::Event::Info info);
	void OnConfigureChannelClosed(Gwen::Event::Info info);
		
	void OnMouseMoved(int x, int y, int dx, int dy) override;
	bool OnMouseWheeled( int iDelta ) override;
	void OnMouseClickLeft( int /*x*/, int /*y*/, bool /*bDown*/ ) override;
	void OnMouseClickRight( int /*x*/, int /*y*/, bool /*bDown*/ ) override;

	Gwen::Color		m_Color;
		
	std::vector<Channel*> channels_;

	bool is_2d_ = false;
		
	// these change as we get more samples
	double min_x_ = 0.0;
	double max_x_ = 10.0;// in seconds
		
	double min_y_ = -100.0;
	double max_y_ = 100.0;

	bool autoscale_x_ = true;// only used in a 2d plot
    bool autoscale_y_ = true;

	int key_width_ = 70;
	std::string style_ = "Line";// line, dots or both

private:
    bool redo_scale_ = true;
protected:
		
	pubsub::Time start_time_;// the time we opened this graph, used for making time values smaller
};

#endif
