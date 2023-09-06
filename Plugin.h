
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
class BaseRegisterObject;
namespace pubviz
{
	class Plugin : public Gwen::Event::Handler
	{
		std::string type_;

		friend class ::PubViz;
		friend class ::BaseRegisterObject;
		ps_node_t* node_;
		Gwen::Controls::Properties* props_;
		OpenGLCanvas* canvas_;
		Gwen::Controls::Button* plugin_button_;

		Gwen::Controls::CheckBox* enabled_;

	public:
		virtual ~Plugin() {};

		virtual void Initialize(Gwen::Controls::Properties*) = 0;

		// Render the plugin to the canvas in world coords
		virtual void Render() = 0;

		// Render the plugin to the canvas in pixel coords
		virtual void Paint() {};

		// Update any topics we have and mark for redraws if necessary
		virtual void Update() = 0;

		// Clear out any historical data so the view gets cleared
		virtual void Clear() = 0;

		// Returns if the plugin is enabled and should be rendered
		bool Enabled()
		{
			return enabled_->IsChecked();
		}

		bool Paused()
		{
			return canvas_->Paused();
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
		std::string GetType() { return type_; }

		std::string GetConfiguration()
		{
			std::string out;
			out += "enabled,";
			out += (enabled_->IsChecked() ? "true" : "false");
			// lets just write it as CSV
			int i = 0;
			for (auto& prop : properties_)
			{
				out += ",";
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

			for (int i = 0; i < pts.size(); i += 2)
			{
				if (pts[i] == "enabled")
				{
					enabled_->SetChecked(pts[i + 1] == "true");
					continue;
				}

				auto iter = properties_.find(pts[i]);
				if (iter == properties_.end())
				{
					printf("Invalid property: %s\n", pts[i].c_str());
					continue;
				}

				iter->second->Deserialize(pts[i + 1]);
			}
		}

		std::map<std::string, PropertyBase*> properties_;

		NumberProperty* AddNumberProperty(Gwen::Controls::Properties* tree, const char* name, int num,
			int min = 0,
			int max = 100,
			int increment = 1,
			const std::string& description = "")
		{
			auto prop = new NumberProperty(tree, name, num, min, max, increment, description);
			properties_[name] = prop;
			return prop;
		}

		FloatProperty* AddFloatProperty(Gwen::Controls::Properties* tree, const char* name, double num,
			double min = 0.0,
			double max = 100.0,
			double increment = 1.0,
			const std::string& description = "")
		{
			auto prop = new FloatProperty(tree, name, num, min, max, increment, description);
			properties_[name] = prop;
			return prop;
		}

		ColorProperty* AddColorProperty(Gwen::Controls::Properties* tree, const char* name, Gwen::Color color,
			const std::string& description = "")
		{
			auto prop = new ColorProperty(tree, name, color, description);
			properties_[name] = prop;
			return prop;
		}

		BooleanProperty* AddBooleanProperty(Gwen::Controls::Properties* tree, const char* name, bool val,
			const std::string& description = "")
		{
			auto prop = new BooleanProperty(tree, name, val, description);
			properties_[name] = prop;
			return prop;
		}

		TopicProperty* AddTopicProperty(Gwen::Controls::Properties* tree, const char* name, std::string topic,
			const std::string& description = "", const std::string& type = "", bool use_for_title = true)
		{
			auto prop = new TopicProperty(tree, name, topic, description, type);
			properties_[name] = prop;
			auto p = (Gwen::Controls::PropertyTreeNode*)tree->GetParent();
			if (use_for_title)
			{
				p->SetText(GetTitle() + " (" + topic + ")");
				prop->onChange2 = [p, this](std::string s) {
					p->SetText(GetTitle() + " (" + s + ")");
				};
			}
			return prop;
		}

		StringProperty* AddStringProperty(Gwen::Controls::Properties* tree, const char* name, std::string val,
			const std::string& description = "")
		{
			auto prop = new StringProperty(tree, name, val, description);
			properties_[name] = prop;
			return prop;
		}

		EnumProperty* AddEnumProperty(Gwen::Controls::Properties* tree, const char* name, std::string def, std::vector<std::string> enums,
			const std::string& description = "")
		{
			auto prop = new EnumProperty(tree, name, def, enums, description);
			properties_[name] = prop;
			return prop;
		}
	};
}
#endif
