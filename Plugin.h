
#ifndef PUBVIZ_PLUGIN_H
#define PUBVIZ_PLUGIN_H

#include <pubsub_cpp/Node.h>

#include <Gwen/Controls/PropertyTree.h>

#include "properties.h"
#include "AABB.h"

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
		friend class ::OpenGLCanvas;
		pubsub::Node* node_;
		Gwen::Controls::Properties* props_;
		OpenGLCanvas* canvas_;
		Gwen::Controls::Button* plugin_button_;

		Gwen::Controls::CheckBox* enabled_;
		Gwen::Controls::TreeNode* tree_node_;

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

		// Render selection ids into the framebuffer
		virtual uint32_t RenderSelect(uint32_t start_index) { return start_index; }

		// Returns info about a selected item including bounds
		virtual std::map<std::string, std::string> Select(uint32_t index, AABB& size) { return {}; }

		// Applies only for 2d. Return true if event is handled.
		virtual bool OnMapDoubleClick(double x, double y) { return false; }

		// Called on right click to add items to a context menu
		virtual std::vector<std::pair<std::string, std::function<void()>>> ContextMenu(double x, double y) { return {}; }
		
		enum ErrorSeverity
		{
		  WARN = 0,
		  ERROR = 1
		};
		
		// Returns a list of any errors with any plugins
		virtual std::vector<std::pair<ErrorSeverity, std::string>> GetErrors() { return {}; }

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
		pubsub::Node* GetNode()
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
			// todo add a rate limit here
			props_->Redraw();
		}

		virtual std::string GetTitle() = 0;
		std::string GetType() { return type_; }

		std::string GetConfiguration()
		{
			std::string out;
			out += "enabled,";
			out += (enabled_->IsChecked() ? "true" : "false");
			out += ",collapsed,";
			out += tree_node_->GetToggleButton()->GetToggleState() ? "false" : "true";
			// lets just write it as CSV
			int i = 0;

			for (const auto& prop : properties_)
			{
				// skip ButtonProperties
				if (dynamic_cast<ButtonProperty*>(prop.second))
				{
					continue;
				}
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
				else if (pts[i] == "collapsed")
				{
					if (pts[i+1] == "true")
					{
						tree_node_->Close();
					}
					else
					{
						tree_node_->Open();
					}
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

		std::unique_ptr<NumberProperty> AddNumberProperty(Gwen::Controls::Properties* tree, const char* name, int num,
			int min = 0,
			int max = 100,
			int increment = 1,
			const std::string& description = "")
		{
			auto prop = new NumberProperty(tree, name, num, min, max, increment, description);
			properties_[name] = prop;
			return std::unique_ptr<NumberProperty>(prop);
		}

		std::unique_ptr<FloatProperty> AddFloatProperty(Gwen::Controls::Properties* tree, const char* name, double num,
			double min = 0.0,
			double max = 100.0,
			double increment = 1.0,
			const std::string& description = "")
		{
			auto prop = new FloatProperty(tree, name, num, min, max, increment, description);
			properties_[name] = prop;
			return std::unique_ptr<FloatProperty>(prop);
		}

		std::unique_ptr<ColorProperty> AddColorProperty(Gwen::Controls::Properties* tree, const char* name, Gwen::Color color,
			const std::string& description = "")
		{
			auto prop = new ColorProperty(tree, name, color, description);
			properties_[name] = prop;
			return std::unique_ptr<ColorProperty>(prop);
		}

		std::unique_ptr<BooleanProperty> AddBooleanProperty(Gwen::Controls::Properties* tree, const char* name, bool val,
			const std::string& description = "")
		{
			auto prop = new BooleanProperty(tree, name, val, description);
			properties_[name] = prop;
			return std::unique_ptr<BooleanProperty>(prop);
		}

		std::unique_ptr<TopicProperty> AddTopicProperty(Gwen::Controls::Properties* tree, const char* name, std::string topic,
			const std::string& description = "", const std::string& type = "", bool use_for_title = true, bool published = true)
		{
			auto prop = new TopicProperty(tree, name, topic, description, type, published);
			properties_[name] = prop;
			auto p = (Gwen::Controls::PropertyTreeNode*)tree->GetParent();
			if (use_for_title)
			{
				p->SetText(GetTitle() + " (" + topic + ")");
				prop->onChange2 = [p, this](std::string s) {
					p->SetText(GetTitle() + " (" + s + ")");
				};
			}
			return std::unique_ptr<TopicProperty>(prop);
		}

		std::unique_ptr<StringProperty> AddStringProperty(Gwen::Controls::Properties* tree, const char* name, std::string val,
			const std::string& description = "")
		{
			auto prop = new StringProperty(tree, name, val, description);
			properties_[name] = prop;
			return std::unique_ptr<StringProperty>(prop);
		}

		std::unique_ptr<FileProperty> AddFileProperty(Gwen::Controls::Properties* tree, const char* name, std::string val,
			const std::string& description = "")
		{
			auto prop = new FileProperty(tree, name, val, description);
			properties_[name] = prop;
			return std::unique_ptr<FileProperty>(prop);
		}

		std::unique_ptr<EnumProperty> AddEnumProperty(Gwen::Controls::Properties* tree, const char* name, std::string def, std::vector<std::string> enums,
			const std::string& description = "")
		{
			auto prop = new EnumProperty(tree, name, def, enums, description);
			properties_[name] = prop;
			return std::unique_ptr<EnumProperty>(prop);
		}

		std::unique_ptr<ButtonProperty> AddButtonProperty(Gwen::Controls::Properties* tree, const char* name,
			const std::string& description = "")
		{
			auto prop = new ButtonProperty(tree, name, description);
			properties_[name] = prop;
			return std::unique_ptr<ButtonProperty>(prop);
		}
	};
}
#endif
