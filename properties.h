
#ifndef PUBVIZ_PROPERTIES_H
#define PUBVIZ_PROPERTIES_H

#include <Gwen/Controls/PropertyTree.h>
#include <Gwen/Controls/Property/Checkbox.h>
#include <Gwen/Controls/Property/ColorSelector.h>
#include <Gwen/Controls/Property/ComboBox.h>
#include <Gwen/Controls/Property/Numeric.h>
#include <Gwen/Controls/Property/Folder.h>

#include <functional>

class PropertyBase: public Gwen::Event::Handler
{
public:

	virtual std::string Serialize() = 0;
	
	virtual void Deserialize(const std::string& str) = 0;
};

class BooleanProperty: public PropertyBase
{
	Gwen::Controls::Property::Checkbox* property_;
	
	bool value_;
	
	void OnChange(Gwen::Controls::Base* prop)
	{
		property_->Redraw();
		value_ = std::atoi(property_->GetPropertyValue().c_str()) > 0 ? true : false;
		if (onChange)
		{
			onChange(value_);
		}
	}
	
public:

	BooleanProperty(Gwen::Controls::Properties* tree, const std::string& name, bool val, const std::string& description = "")
	{
		property_ = new Gwen::Controls::Property::Checkbox(tree);
		auto item = tree->Add(name, property_, val ? "1" : "0");
		item->onChange.Add(this, &BooleanProperty::OnChange);
		if (description.length())
		{
			item->SetToolTip(description);
		}
		value_ = val;
	}
	
	bool GetValue()
	{
		return value_;
	}
	
	virtual std::string Serialize()
	{
		return value_ ? "true" : "false";
	}
	
	virtual void Deserialize(const std::string& str)
	{
		if (str == "true")
		{
			value_ = true;
		}
		else
		{
			value_ = false;
		}
		property_->SetPropertyValue(value_ ? "1" : "0", true);
		property_->Redraw();
	}

    void Hide()
    {
        property_->GetParent()->Hide();
    }

    void Show()
    {
        property_->GetParent()->Show();
    }

	std::function<void(bool)> onChange;
};

class NumberProperty: public PropertyBase
{
	Gwen::Controls::Property::Numeric* property_;
	
	int value_;
	
	void cbOnChange(Gwen::Controls::Base* prop)
	{
		property_->Redraw();
		value_ = std::atoi(property_->GetPropertyValue().c_str());
		if (onChange)
		{
			onChange(value_);
		}
	}
	
public:

	NumberProperty(Gwen::Controls::Properties* tree, const std::string& name, int num,
	  int min = 0,
	  int max = 100,
	  int increment = 1, const std::string& description = "")
	{
		property_ = new Gwen::Controls::Property::Numeric(tree);
		property_->m_Numeric->SetMin(min);
		property_->m_Numeric->SetMax(max);
		property_->m_Numeric->SetIncrement(increment);
		auto item = tree->Add(name, property_, std::to_string(num));
		item->onChange.Add(this, &NumberProperty::cbOnChange);
		if (description.length())
		{
			item->SetToolTip(description);
		}
		value_ = num;
	}
	
	int GetValue()
	{
		return value_;
	}
	
	virtual std::string Serialize()
	{
		return std::to_string(value_);
	}
	
	virtual void Deserialize(const std::string& str)
	{
		value_ = std::atoi(str.c_str());
		property_->SetPropertyValue(str, true);
	}

    void Hide()
    {
        property_->GetParent()->Hide();
    }

    void Show()
    {
        property_->GetParent()->Show();
    }
	
	std::function<void(int)> onChange;
};

class FloatProperty: public PropertyBase
{
	Gwen::Controls::Property::Float* property_;
	
	double value_;
	
	void cbOnChange(Gwen::Controls::Base* prop)
	{
		property_->Redraw();
		value_ = std::atof(property_->GetPropertyValue().c_str());
		if (onChange)
		{
			onChange(value_);
		}
	}
	
public:

	FloatProperty(Gwen::Controls::Properties* tree, const std::string& name, double num,
	  double min = 0.0,
	  double max = 100.0,
	  double increment = 1.0, const std::string& description = "")
	{
		property_ = new Gwen::Controls::Property::Float(tree);
		property_->m_Numeric->SetMin(min);
		property_->m_Numeric->SetMax(max);
		property_->m_Numeric->SetIncrement(increment);
		auto item = tree->Add(name, property_, std::to_string(num));
		item->onChange.Add(this, &FloatProperty::cbOnChange);
		if (description.length())
		{
			item->SetToolTip(description);
		}
		value_ = num;
	}
	
	double GetValue()
	{
		return value_;
	}
	
	virtual std::string Serialize()
	{
		return std::to_string(value_);
	}
	
	virtual void Deserialize(const std::string& str)
	{
		value_ = std::atof(str.c_str());
		property_->SetPropertyValue(str, true);
	}

    void Hide()
    {
        property_->GetParent()->Hide();
    }

    void Show()
    {
        property_->GetParent()->Show();
    }

	std::function<void(int)> onChange;
};

class TopicProperty: public PropertyBase
{
	Gwen::Controls::Property::Text* property_;
	
	std::string value_;
	
	void cbOnChange(Gwen::Controls::Base* prop)
	{
		property_->Redraw();
		value_ = property_->GetPropertyValue().c_str();
		if (onChange)
		{
			onChange(value_);
		}
	}
	
public:

	TopicProperty(Gwen::Controls::Properties* tree, const std::string& name, std::string topic = "", const std::string& description = "")
	{
		property_ = new Gwen::Controls::Property::Text(tree);
		auto item = tree->Add(name, property_, topic);
		item->onChange.Add(this, &TopicProperty::cbOnChange);
		if (description.length())
		{
			item->SetToolTip(description);
		}
		value_ = topic;
	}
	
	std::string GetValue()
	{
		return value_;
	}
	
	virtual std::string Serialize()
	{
		return value_;
	}
	
	virtual void Deserialize(const std::string& str)
	{
		value_ = str;
		property_->SetPropertyValue(str, true);
	}

    void Hide()
    {
        property_->GetParent()->Hide();
    }

    void Show()
    {
        property_->GetParent()->Show();
    }
	
	std::function<void(std::string)> onChange;
};

class StringProperty: public PropertyBase
{
	Gwen::Controls::Property::Text* property_;
	
	std::string value_;
	
	void cbOnChange(Gwen::Controls::Base* prop)
	{
		property_->Redraw();
		value_ = property_->GetPropertyValue().c_str();
		if (onChange)
		{
			onChange(value_);
		}
	}
	
public:

	StringProperty(Gwen::Controls::Properties* tree, const std::string& name, std::string topic = "", const std::string& description = "")
	{
		property_ = new Gwen::Controls::Property::Text(tree);
		auto item = tree->Add(name, property_, topic);
		item->onChange.Add(this, &StringProperty::cbOnChange);
		if (description.length())
		{
			item->SetToolTip(description);
		}
		value_ = topic;
	}
	
	std::string GetValue()
	{
		return value_;
	}
	
	virtual std::string Serialize()
	{
		return value_;
	}
	
	virtual void Deserialize(const std::string& str)
	{
		value_ = str;
		property_->SetPropertyValue(str, true);
	}

    void Hide()
    {
        property_->GetParent()->Hide();
    }

    void Show()
    {
        property_->GetParent()->Show();
    }
	
	std::function<void(std::string)> onChange;
};

class EnumProperty: public PropertyBase
{
	Gwen::Controls::Property::ComboBox* property_;
	
	std::string value_;
	
	void cbOnChange(Gwen::Controls::Base* prop)
	{
		property_->Redraw();
		value_ = property_->GetPropertyValue().c_str();
		if (onChange)
		{
			onChange(value_);
		}
	}
	
public:

	EnumProperty(Gwen::Controls::Properties* tree, const std::string& name, std::string value, std::vector<std::string> enums, const std::string& description = "")
	{
		property_ = new Gwen::Controls::Property::ComboBox(tree);
        auto box = property_->GetComboBox();
        for (auto& item: enums)
        {
            box->AddItem(Gwen::Utility::StringToUnicode(item), item);
        }
		auto item = tree->Add(name, property_, value);
		item->onChange.Add(this, &EnumProperty::cbOnChange);
        if (description.length())
        {
            item->SetToolTip(description);
        }
		value_ = value;
	}
	
	std::string GetValue()
	{
		return value_;
	}
	
	virtual std::string Serialize()
	{
		return value_;
	}
	
	virtual void Deserialize(const std::string& str)
	{
		value_ = str;
		property_->SetPropertyValue(str, true);
	}

    void Hide()
    {
        property_->GetParent()->Hide();
    }

    void Show()
    {
        property_->GetParent()->Show();
    }
	
	std::function<void(std::string)> onChange;
};

class ColorProperty: public PropertyBase
{
	Gwen::Controls::Property::ColorSelector* property_;
	
	Gwen::Color value_;
	
	void onChange(Gwen::Controls::Base* control)
	{
		property_->Redraw();
		auto prop = ((Gwen::Controls::PropertyRow*)control)->GetProperty();
		Gwen::Controls::Property::ColorSelector* selector = (Gwen::Controls::Property::ColorSelector*)prop;
		value_ = selector->m_Button->m_Color;
	}
	
public:

	ColorProperty(Gwen::Controls::Properties* tree, const std::string& name, Gwen::Color color, const std::string& description = "")
	{
		std::string c_str = std::to_string(color.r) + " ";
		c_str += std::to_string(color.g) + " " + std::to_string(color.b);
		property_ = new Gwen::Controls::Property::ColorSelector(tree);
		auto item = tree->Add(name, property_, c_str);
		item->onChange.Add(this, &ColorProperty::onChange);
        if (description.length())
        {
            item->SetToolTip(description);
        }
		value_ = color;
	}
	
	Gwen::Color GetValue()
	{
		return value_;
	}
	
	virtual std::string Serialize()
	{
		return std::to_string(value_.r) + " " + std::to_string(value_.g) + " " + std::to_string(value_.b);
	}
	
	virtual void Deserialize(const std::string& str)
	{
		property_->SetPropertyValue(str, true);
	}

    void Hide()
    {
        property_->GetParent()->Hide();
    }

    void Show()
    {
        property_->GetParent()->Show();
    }
};

#endif
