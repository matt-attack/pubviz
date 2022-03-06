
#ifndef PUBVIZ_PROPERTIES_H
#define PUBVIZ_PROPERTIES_H

#include <Gwen/Controls/PropertyTree.h>
#include <Gwen/Controls/Property/Numeric.h>

class NumberProperty: public Gwen::Event::Handler
{
	Gwen::Controls::Property::Numeric* property_;
	
	int value_;
	
	void onChange(Gwen::Controls::Base* prop)
	{
		value_ = std::atoi(property_->GetPropertyValue().c_str());
	}
	
public:

	NumberProperty(Gwen::Controls::Properties* tree, const std::string& name, int num)
	{
		property_ = new Gwen::Controls::Property::Numeric(tree);
		auto item = tree->Add(name, property_, std::to_string(num));
		item->onChange.Add(this, &NumberProperty::onChange);
		value_ = num;
	}
	
	int GetValue()
	{
		return value_;
	}
};

class FloatProperty: public Gwen::Event::Handler
{
	Gwen::Controls::Property::Float* property_;
	
	double value_;
	
	void onChange(Gwen::Controls::Base* prop)
	{
		value_ = std::atof(property_->GetPropertyValue().c_str());
	}
	
public:

	FloatProperty(Gwen::Controls::Properties* tree, const std::string& name, double num)
	{
		property_ = new Gwen::Controls::Property::Float(tree);
		auto item = tree->Add(name, property_, std::to_string(num));
		item->onChange.Add(this, &FloatProperty::onChange);
		value_ = num;
	}
	
	double GetValue()
	{
		return value_;
	}
};

class ColorProperty: public Gwen::Event::Handler
{
	Gwen::Controls::Property::ColorSelector* property_;
	
	Gwen::Color value_;
	
	void onChange(Gwen::Controls::Base* control)
	{
		auto prop = ((Gwen::Controls::PropertyRow*)control)->GetProperty();
		Gwen::Controls::Property::ColorSelector* selector = (Gwen::Controls::Property::ColorSelector*)prop;
		value_ = selector->m_Button->m_Color;
	}
	
public:

	ColorProperty(Gwen::Controls::Properties* tree, const std::string& name, Gwen::Color color)
	{
		std::string c_str = std::to_string(color.r) + " ";
		c_str += std::to_string(color.g) + " " + std::to_string(color.b);
		property_ = new Gwen::Controls::Property::ColorSelector(tree);
		auto item = tree->Add(name, property_, c_str);
		item->onChange.Add(this, &ColorProperty::onChange);
		value_ = color;
	}
	
	Gwen::Color GetValue()
	{
		return value_;
	}
};

#endif
