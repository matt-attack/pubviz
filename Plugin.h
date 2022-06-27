
#ifndef PUBVIZ_PLUGIN_H
#define PUBVIZ_PLUGIN_H

#include <pubsub/Node.h>

#include <Gwen/Controls/PropertyTree.h>

#include "properties.h"

namespace Gwen
{
	namespace Controls
	{
		class TreeControl;
		class Properties;
		class MenuStrip;
		class TextBoxCode;
	}
}

extern std::vector<std::string> split(std::string s, std::string delimiter);

class PubViz;
class Plugin: public Gwen::Event::Handler
{
	friend class PubViz;
	bool enabled_ = true;
	ps_node_t* node_;
	Gwen::Controls::Properties* props_;
	OpenGLCanvas* canvas_;
	
	void OnEnableChecked( Gwen::Controls::Base* pControl )
	{
		Gwen::Controls::PropertyRow* pRow = ( Gwen::Controls::PropertyRow* ) pControl;
		enabled_ = pRow->GetProperty()->GetPropertyValue().GetUnicode() == L"1";
	}
	
public:
	virtual ~Plugin() {};
	
	virtual void Initialize(Gwen::Controls::Properties*) = 0;
	
	// Render the plugin to the canvas
	virtual void Render() = 0;
	
	// Update any topics we have and mark for redraws if necessary
	virtual void Update() = 0;
	
	// Returns if the plugin is enabled and should be rendered
	bool Enabled()
	{
		return enabled_;
	}
	
	// Get the node for this view
	ps_node_t* GetNode()
	{
		return node_;
	}

	// Get our canvas
	OpenGLCanvas* GetCanvas()
	{
		return canvas_;
	}
	
	// Indicate that we want a redraw
	void Redraw()
	{
		props_->Redraw();
	}
	
	virtual std::string GetTitle() = 0;
	
	std::string GetConfiguration()
	{
		std::string out;
		// lets just write it as CSV
		int i = 0;
		for (auto& prop: properties_)
		{
			if (i++ != 0)
			{
				out += ",";
			}
			out += prop.first;
			out += ",";
			out += prop.second->Serialize();
		}
		return out;
	}
	
	void LoadConfiguration(const std::string& config)
	{
		auto pts = split(config, ",");
		if (pts.size() % 2 != 0)
		{
			printf("Invalid config\n");
			return;
		}
		
		for (int i = 0; i < pts.size(); i+=2)
		{
			auto iter = properties_.find(pts[i]);
			if (iter == properties_.end())
			{
				printf("Invalid property: %s\n", pts[i].c_str());
				continue;
			}
			
			iter->second->Deserialize(pts[i+1]);
		}
	}
	
	std::map<std::string, PropertyBase*> properties_;
	
	NumberProperty* AddNumberProperty(Gwen::Controls::Properties* tree, const char* name, int num,
	  int min = 0,
	  int max = 100,
	  int increment = 1)
	{
		auto prop = new NumberProperty(tree, name, num, min, max, increment);
		properties_[name] = prop;
		return prop;
	}
	
	FloatProperty* AddFloatProperty(Gwen::Controls::Properties* tree, const char* name, double num,
	  double min = 0.0,
	  double max = 100.0,
	  double increment = 1.0)
	{
		auto prop = new FloatProperty(tree, name, num, min, max, increment);
		properties_[name] = prop;
		return prop;
	}
	
	ColorProperty* AddColorProperty(Gwen::Controls::Properties* tree, const char* name, Gwen::Color color)
	{
		auto prop = new ColorProperty(tree, name, color);
		properties_[name] = prop;
		return prop;
	}
	
	BooleanProperty* AddBooleanProperty(Gwen::Controls::Properties* tree, const char* name, bool val)
	{
		auto prop = new BooleanProperty(tree, name, val);
		properties_[name] = prop;
		return prop;
	}
	
	TopicProperty* AddTopicProperty(Gwen::Controls::Properties* tree, const char* name, std::string topic)
	{
		auto prop = new TopicProperty(tree, name, topic);
		properties_[name] = prop;
		return prop;
	}
	
	StringProperty* AddStringProperty(Gwen::Controls::Properties* tree, const char* name, std::string val)
	{
		auto prop = new StringProperty(tree, name, val);
		properties_[name] = prop;
		return prop;
	}
};

#endif
