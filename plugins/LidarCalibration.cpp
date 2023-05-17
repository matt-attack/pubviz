#include "LidarCalibration.h"

#include "../controls/pubviz.h"

#include <Gwen/Controls/Property/Button.h>

REGISTER_PLUGIN("lidar_calibration", LidarCalibrationPlugin)

void Lidar::CreateProperties(Plugin* plugin, int id)
{
	std::string prefix = "Lidar " + std::to_string(id) + ": ";
	if (id == 0)
	{
		prefix = "Blue:  ";
	}
	else if (id == 1)
	{
		prefix = "Green: ";
	}
	else if (id == 2)
	{
		prefix = "Red:   ";
	}
	else if (id == 3)
	{
		prefix = "Cyan:  ";
	}
	auto tree = plugin->GetPropertyTree();
	show = plugin->AddBooleanProperty(tree, prefix + "Show", true, "Show lidar points?");

	x = plugin->AddFloatProperty(tree, prefix + "X", position.x, -10.0, 10.0, 0.1, "X offset of lidar in meters.");
	x->onChange = [this](double) { if (on_change) on_change(); };
	y = plugin->AddFloatProperty(tree, prefix + "Y", position.y, -10.0, 10.0, 0.1, "Y offset of lidar in meters.");
	y->onChange = [this](double) { if (on_change) on_change(); };
	z = plugin->AddFloatProperty(tree, prefix + "Z", position.z, -10.0, 10.0, 0.1, "Z offset of lidar in meters.");
	z->onChange = [this](double) { if (on_change) on_change(); };


	pyaw = plugin->AddFloatProperty(tree, prefix + "Yaw", yaw, -360.0, 360.0, 1.0, "Yaw offset of lidar in degrees.");
	pyaw->onChange = [this](double) { if (on_change) on_change(); };
	ppitch = plugin->AddFloatProperty(tree, prefix + "Pitch", pitch, -360.0, 360.0, 1.0, "Pitch offset of lidar in degrees.");
	ppitch->onChange = [this](double) { if (on_change) on_change(); };
	proll = plugin->AddFloatProperty(tree, prefix + "Roll", roll, -360.0, 360.0, 1.0, "Roll offset of lidar in degrees.");
	proll->onChange = [this](double) { if (on_change) on_change(); };

	// Add calibrate buttons
	auto pRow2 = tree->Add(L"", new Gwen::Controls::Property::Button(tree), L"Level");
	pRow2->onChange.Add(plugin, &LidarCalibrationPlugin::OnCalibrate);
	pRow2->UserData.Set<Lidar*>("lidar", this);

	pRow2 = tree->Add(L"", new Gwen::Controls::Property::Button(tree), L"Position");
	pRow2->onChange.Add(plugin, &LidarCalibrationPlugin::OnPosition);
	pRow2->UserData.Set<Lidar*>("lidar", this);
}