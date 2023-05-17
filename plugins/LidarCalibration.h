
#ifndef PUBVIZ_PLUGIN_CAL_POINTCLOUD_H
#define PUBVIZ_PLUGIN_CAL_POINTCLOUD_H

#include <pubsub/Node.h>
#include <pubsub/Subscriber.h>

#include "../Plugin.h"
#include "../properties.h"

#include <Gwen/Gwen.h>
#include <Gwen/Align.h>
#include <Gwen/Utility.h>
#include <Gwen/Controls/WindowControl.h>
#include <Gwen/Controls/TabControl.h>
#include <Gwen/Controls/ListBox.h>
#include <Gwen/Controls/DockBase.h>
#include <Gwen/Controls/StatusBar.h>
#include <Gwen/Controls/PropertyTree.h>
#include <Gwen/Controls/Property/Numeric.h>
#include <Gwen/Controls/Property/Button.h>

#include <Gwen/Controls/Dialogs/FileOpen.h>

#include <fstream>
#include <sstream>

#include "LidarScan.msg.h"

#include "math.h"

#include <pcl/console/parse.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/sample_consensus/ransac.h>
#include <pcl/sample_consensus/sac_model_plane.h>
#include <pcl/sample_consensus/sac_model_sphere.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/features/normal_3d.h>


#define GLEW_STATIC
#include <GL/glew.h>

#ifndef _WIN32
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#endif

#undef max
#undef min

#pragma pack(1)
/** Dual extend cartesian coordinate format.
/*typedef struct {
	int32_t x1;            //< X axis, Unit:mm
	int32_t y1;            //< Y axis, Unit:mm
	int32_t z1;            //< Z axis, Unit:mm
	uint8_t reflectivity1; //< Reflectivity
	uint8_t tag1;          //< Tag
	int32_t x2;            //< X axis, Unit:mm
	int32_t y2;            //< Y axis, Unit:mm
	int32_t z2;            //< Z axis, Unit:mm
	uint8_t reflectivity2; //< Reflectivit
	uint8_t tag2;          //< Tag 
} LivoxDualExtendRawPoint;*/

struct PointXYZI
{
	float x, y, z, i;
};
#pragma pack()


struct Point3d
{
	float x;
	float y;
	float z;
};

struct Cloud
{
	int num_points = 0;
	unsigned int point_vbo;
	unsigned int color_vbo;
	void* data = 0;

	Vec3 normal;

	void UpdatePoints(const Matrix3x4& lidar_to_vehicle, const std::string& color_mode, int alpha, Gwen::Color single_color, int lidar_id, Gwen::Color* color_table)
	{
		static std::vector<Point3d> point_buf_;
		static std::vector<int> color_buf_;

		point_buf_.resize(num_points);
		color_buf_.resize(num_points);

		auto pt = (PointXYZI*)data;
		if (color_mode == "Intensity")
		{
			for (int i = 0; i < num_points; i++)
			{
				auto pos = Vec3(pt[i].x, pt[i].y, pt[i].z);
				pos = lidar_to_vehicle.transform(pos);
				point_buf_[i].x = pos.x;
				point_buf_[i].y = pos.y;
				point_buf_[i].z = pos.z;

				Gwen::Color& color = color_table[(uint8_t)pt[i].i];
				uint8_t r = color.r;
				uint8_t g = color.g;
				uint8_t b = color.b;
				color_buf_[i] = (alpha << 24) | r | (g << 8) | (b << 16);// todo intensity

				if (point_buf_[i].x == 0 && point_buf_[i].y == 0 && point_buf_[i].z == 0)
				{
					color_buf_[i] = 0;
				}
			}
		}
		// todo add depth
		else if (color_mode == "Height")
		{
			for (int i = 0; i < num_points; i++)
			{
				auto pos = Vec3(pt[i].x, pt[i].y, pt[i].z);
				pos = lidar_to_vehicle.transform(pos);
				point_buf_[i].x = pos.x;
				point_buf_[i].y = pos.y;
				point_buf_[i].z = pos.z;

				Gwen::Color& color = color_table[(uint8_t)((int)(pos.z*90.0)%255)];
				uint8_t r = color.r;
				uint8_t g = color.g;
				uint8_t b = color.b;
				color_buf_[i] = (alpha << 24) | r | (g << 8) | (b << 16);// todo intensity

				if (point_buf_[i].x == 0 && point_buf_[i].y == 0 && point_buf_[i].z == 0)
				{
					color_buf_[i] = 0;
				}
			}
		}
		else if (color_mode == "Lidar ID")
		{
			int colors[4];
			colors[0] = 0x00FF0000;
			colors[1] = 0x00FF00;
			colors[2] = 0x00FF;
			colors[3] = 0x00FFFF00;
			for (int i = 0; i < num_points; i++)
			{
				auto pos = Vec3(pt[i].x, pt[i].y, pt[i].z);
				pos = lidar_to_vehicle.transform(pos);
				point_buf_[i].x = pos.x;
				point_buf_[i].y = pos.y;
				point_buf_[i].z = pos.z;

				color_buf_[i] = (alpha << 24) | colors[lidar_id];

				if (point_buf_[i].x == 0 && point_buf_[i].y == 0 && point_buf_[i].z == 0)
				{
					color_buf_[i] = 0;
				}
			}
		}
		else
		{
			uint8_t r = single_color.r;
			uint8_t g = single_color.g;
			uint8_t b = single_color.b;

			auto color_int = (alpha << 24) | r | (g << 8) | (b << 16);

			// single color
			for (int i = 0; i < num_points; i++)
			{
				auto pos = Vec3(pt[i].x, pt[i].y, pt[i].z);
				pos = lidar_to_vehicle.transform(pos);
				point_buf_[i].x = pos.x;
				point_buf_[i].y = pos.y;
				point_buf_[i].z = pos.z;

				color_buf_[i] = color_int;

				if (point_buf_[i].x == 0 && point_buf_[i].y == 0 && point_buf_[i].z == 0)
				{
					color_buf_[i] = 0;
				}
			}
		}

		glBindBuffer(GL_ARRAY_BUFFER, point_vbo);
		glBufferData(GL_ARRAY_BUFFER, point_buf_.size() * 12, point_buf_.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, color_vbo);
		glBufferData(GL_ARRAY_BUFFER, color_buf_.size() * 4, color_buf_.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
};

struct Lidar
{
	int index;
	FloatProperty* proll, * ppitch, * pyaw;
	FloatProperty* x, * y, * z;
	BooleanProperty* show;
	Vec3 position;
	double roll, pitch, yaw;
	std::deque<Cloud> clouds;
	Cloud inliers;
	std::function<void(void)> on_change;

	Vec3 normal;

	Quaternion GetQuaternion()
	{
		Quaternion qyaw = Quaternion::fromAngleAxis(Vec3(0.0f, 0.0f, 1.0f), yaw * 3.1415926535895f / 180.0f);
		Quaternion qpitch = Quaternion::fromAngleAxis(Vec3(0.0f, 1.0f, 0.0f), pitch * 3.1415926535895f / 180.0f);
		Quaternion qroll = Quaternion::fromAngleAxis(Vec3(1.0f, 0.0f, 0.0f), roll * 3.1415926535895f / 180.0f);

		return (qyaw * qpitch) * qroll;
	}

	bool Show()
	{
		return show->GetValue();
	}

	void CreateProperties(Plugin* plugin, int id);

	// returns if the value changed
	void UpdateFromProperties()
	{
		position.x = x->GetValue();
		position.y = y->GetValue();
		position.z = z->GetValue();

		yaw = pyaw->GetValue();
		pitch = ppitch->GetValue();
		roll = proll->GetValue();
	}
};

class LidarCalibrationPlugin : public Plugin
{
	std::map<std::string, Lidar> lidars_;

	FloatProperty* alpha_;
	NumberProperty* point_size_;
	NumberProperty* history_length_;

	// Coloring Properties
	EnumProperty* coloring_mode_;

	FloatProperty* min_value_;

	// Used in single color mode
	ColorProperty* single_color_;

	BooleanProperty* show_inliers_;

	bool sub_open_ = false;
	ps_sub_t subscriber_;

	void OnShowToggled(bool y)
	{
		Redraw();
	}

	void HistoryLengthChange(int length)
	{
		for (auto& lidar : lidars_)
		{
			while (lidar.second.clouds.size() > length)
			{
				const auto& cloud = lidar.second.clouds.back();
				glDeleteBuffers(1, &cloud.point_vbo);
				glDeleteBuffers(1, &cloud.color_vbo);
				lidar.second.clouds.pop_back();
			}
		}

		Redraw();
	}

	void ColoringModeChange(std::string mode)
	{
		// hide all properties to start
		//auto canvas = min_color_->GetCanvas();
		single_color_->Hide();
		// show the desired properties
		if (mode == "Single Color")
		{
			single_color_->Show();
		}
		else if (mode == "Intensity")
		{
			//coloring_field_->Show();  
			//auto_min_max_->Show();          
		}
		else if (mode == "Lidar ID")
		{
			//coloring_field_->Show();
			//auto_min_max_->Show();
		}
	}

	std::string current_topic_;
	void Subscribe(std::string str)
	{
		if (sub_open_)
		{
			ps_sub_destroy(&subscriber_);
		}

		/*for (auto cloud : clouds_)
		{
			// delete the buffers
			glDeleteBuffers(1, &cloud.point_vbo);
			glDeleteBuffers(1, &cloud.color_vbo);
		}
		clouds_.clear();*/

		current_topic_ = str;
		struct ps_subscriber_options options;
		ps_subscriber_options_init(&options);
		options.preferred_transport = 1;// tcp yo
		ps_node_create_subscriber_adv(GetNode(), current_topic_.c_str(), &lidar_processing__LidarScan_def, &subscriber_, &options);
		sub_open_ = true;
	}

public:

	LidarCalibrationPlugin()
	{
		// dont use pubsub here
		glewInit();
	}

	void OnLoad(Gwen::Controls::Base* control)
	{
		Gwen::Dialogs::FileOpen(true, "Open Map", "", "My Map Format | *.txt", this, &LidarCalibrationPlugin::AfterOpen);
	}

	void AfterOpen(Gwen::Event::Info info)
	{
		auto file = info.String;
		std::ifstream infile(file.c_str());
		std::string line;
		while (std::getline(infile, line))
		{
			// Skip empty lines
			if (line.length() == 0)
			{
				continue;
			}

			// process a lidar
			std::istringstream iss(line);
			std::string name;
			double x, y, z;
			double yaw, pitch, roll;
			iss >> name >> x >> y >> z >> yaw >> pitch >> roll;
			printf("got line\n");

			// okay, now update the configs
			auto lidar = lidars_.find(name);
			if (lidar != lidars_.end())
			{
				lidar->second.x->SetValue(x);
				lidar->second.y->SetValue(y);
				lidar->second.z->SetValue(z);

				lidar->second.pyaw->SetValue(yaw);
				lidar->second.ppitch->SetValue(pitch);
				lidar->second.proll->SetValue(roll);
			}
		}
	}

	void OnSave(Gwen::Controls::Base* control)
	{
		// just write it to file atm
		std::ofstream save("lidar_calibration.txt");
		for (auto& lidar : lidars_)
		{
			lidar.second.UpdateFromProperties();

			save << lidar.first;
			save << " " << lidar.second.position.x << " " << lidar.second.position.y << " " << lidar.second.position.z;
			save << " " << lidar.second.yaw << " " << lidar.second.pitch << " " << lidar.second.roll << "\n";
		}
		save.close();
	}

	void AfterSave(Gwen::Event::Info info)
	{

	}

	void OnPosition(Gwen::Controls::Base* control)
	{
		// okay, so based on the angle, lets position the lidar
		Lidar* lidar = control->UserData.Get<Lidar*>("lidar");
		if (std::abs(lidar->pitch) > 60)
		{
			// rear lidar
			lidar->x->SetValue(-0.504445009);
			lidar->y->SetValue(0.0);
			lidar->z->SetValue(1.742189484);

			lidar->pyaw->SetValue(180.0);
		}
		else if (std::abs(lidar->pitch) > 30)
		{
			// sides
			lidar->x->SetValue(-0.233680467);
			if (lidar->roll > 0)
			{
				// right?
				lidar->y->SetValue(-0.356362713);
				lidar->pyaw->SetValue(-44);
			}
			else
			{
				// left?
				lidar->y->SetValue(0.356362713);
				lidar->pyaw->SetValue(44);
			}
			lidar->z->SetValue(1.693421387);
		}
		else // front?
		{
			lidar->x->SetValue(-0.087376175);
			lidar->y->SetValue(0.0);
			lidar->z->SetValue(1.798069596);

			lidar->pyaw->SetValue(0.0);
		}

			/*
						if (lidar->yaw > 100.0 || lidar->yaw < -100)
			{
				// rear
				lidar->identity = LidarRear;
				lidar->human_readable_identifier = "rear";
			}
			else if (lidar->yaw > 10.0)
			{
				// left side
				lidar->identity = LidarLeft;
				lidar->human_readable_identifier = "left";
			}
			else if (lidar->yaw < -10.0)
			{
				// right side
				lidar->identity = LidarRight;
				lidar->human_readable_identifier = "right";
			}
			else
			{
				// front
				lidar->identity = LidarFront;
				lidar->human_readable_identifier = "front";
			}*/
	}

	void OnCalibrate(Gwen::Controls::Base* control)
	{
		Lidar* lidar = control->UserData.Get<Lidar*>("lidar");

		int num_points = 0;
		for (auto& cloud : lidar->clouds)
		{
			num_points += cloud.num_points;
		}

		pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
		cloud->width = num_points;
		cloud->height = 1;
		cloud->is_dense = false;
		cloud->points.resize(cloud->width * cloud->height);

		Matrix3x4 lidar_to_vehicle(lidar->GetQuaternion(), lidar->position);
		int p = 0;
		for (auto& cld : lidar->clouds)
		{
			auto pt = (PointXYZI*)cld.data;
			for (int i = 0; i < cld.num_points; i++)// um, wat, why did it error at a high number?
			{
				auto pos = Vec3(pt[i].x * 0.001, pt[i].y * 0.001, pt[i].z * 0.001);
				//if (pt[i].x == 0.0 && pt[i].y == 0.0 && )
				//pos = lidar_to_vehicle.transform(pos);
				cloud->points[p].x = pos.x;
				cloud->points[p].y = pos.y;
				cloud->points[p].z = pos.z;
				p++;
			}
		}

		pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients);
		pcl::PointIndices inliers;
		// Create the segmentation object
		pcl::SACSegmentation<pcl::PointXYZ> seg;
		// Optional
		seg.setOptimizeCoefficients(true);
		// Mandatory
		seg.setModelType(pcl::SACMODEL_PLANE);
		seg.setMethodType(pcl::SAC_RANSAC);
		seg.setDistanceThreshold(0.04);
		seg.setMaxIterations(2500);
		seg.setInputCloud(cloud);
		seg.segment(inliers, *coefficients);
		//okay, now run this on many clouds at once and only when a button is pressed for "leveling"

		// Optimize with least-squares
		Eigen::Vector4f plane; float curvature;
		pcl::computePointNormal(*cloud, inliers.indices, plane, curvature);

		// update inliers
		if (lidar->inliers.num_points == 0)
		{
			glGenBuffers(1, &lidar->inliers.point_vbo);
			glGenBuffers(1, &lidar->inliers.color_vbo);
		}
		static std::vector<Point3d> point_buf_;
		static std::vector<int> color_buf_;

		point_buf_.resize(num_points);
		color_buf_.resize(num_points);


		lidar->inliers.num_points = inliers.indices.size();
		for (int i = 0; i < inliers.indices.size(); i++)
		{
			auto& pt = cloud->points[inliers.indices[i]];
			point_buf_[i].x = pt.x;
			point_buf_[i].y = pt.y;
			point_buf_[i].z = pt.z;
			color_buf_[i] = 0xFFFFFFFF;
		}

		glBindBuffer(GL_ARRAY_BUFFER, lidar->inliers.point_vbo);
		glBufferData(GL_ARRAY_BUFFER, point_buf_.size() * 12, point_buf_.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, lidar->inliers.color_vbo);
		glBufferData(GL_ARRAY_BUFFER, color_buf_.size() * 4, color_buf_.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		lidar->normal.x = plane[0];// coefficients->values[0];
		lidar->normal.y = plane[1];// coefficients->values[1];
		lidar->normal.z = plane[2];// coefficients->values[2];
		// flip the plane until z is positive
		if (lidar->normal.z < 0)
		{
			lidar->normal.x *= -1.0;
			lidar->normal.y *= -1.0;
			lidar->normal.z *= -1.0;
		}
		// set orientation to match the pitch and roll of the lidar
		double pitch = atan2(lidar->normal.z, lidar->normal.x) * 180.0 / M_PI - 90;
		double roll = 90-atan2(lidar->normal.z, lidar->normal.y) * 180.0 / M_PI;

		lidar->proll->SetValue(roll);
		lidar->ppitch->SetValue(pitch);
	}

	virtual ~LidarCalibrationPlugin()
	{
		delete alpha_;
		delete point_size_;
		delete history_length_;
		delete coloring_mode_;
		delete single_color_;
		// todo use smart pointers

		glDeleteFramebuffers(1, &frame_buffer_);
		glDeleteTextures(1, &render_texture_);

		if (sub_open_)
		{
			ps_sub_destroy(&subscriber_);
			sub_open_ = false;
		}

		// free any buffers
		for (auto& lidar : lidars_)
		{
			for (auto cloud : lidar.second.clouds)
			{
				glDeleteBuffers(1, &cloud.point_vbo);
				glDeleteBuffers(1, &cloud.color_vbo);
			}
		}
	}

	Gwen::Color rainbow_table_[256];
	Gwen::Color jet_table_[256];

	virtual void Update()
	{
		// process any messages
		// our sub has a message definition, so the queue contains real messages
		lidar_processing::msg::LidarScan* data;
		if (sub_open_)
		{
			while (data = (lidar_processing::msg::LidarScan*)ps_sub_deque(&subscriber_))
			{
				// user is responsible for freeing the message and its arrays
				if (Paused())
				{
					free(data->points);
					free(data);//todo use allocator free
					continue;
				}
				
				auto lidar_name = std::string((const char*)data->lidar_id);
				auto lidar = lidars_.find(lidar_name);
				if (lidar == lidars_.end())
				{
					lidars_.insert({ lidar_name, Lidar() });
					lidar = lidars_.find(lidar_name);

					lidar->second.position = Vec3(data->lidar_x, data->lidar_y, data->lidar_z);
					lidar->second.roll = data->lidar_roll;
					lidar->second.pitch = data->lidar_pitch;
					lidar->second.yaw = data->lidar_yaw;

					int index = lidars_.size() - 1;
					lidar->second.index = index;
					lidar->second.CreateProperties(this, index);

					lidar->second.show->onChange = std::bind(&LidarCalibrationPlugin::OnShowToggled, this, std::placeholders::_1);
					lidar->second.on_change = [this, lidar, index]()
					{
						lidar->second.UpdateFromProperties();

						uint8_t alpha = 255.0 * alpha_->GetValue();
						std::string color_mode = coloring_mode_->GetValue();
						auto single_color = single_color_->GetValue();
						Matrix3x4 lidar_to_vehicle(lidar->second.GetQuaternion(), lidar->second.position);

						// retransform all the points in old clouds
						for (auto& cloud : lidar->second.clouds)
						{
							if (cloud.data == 0)
							{
								continue;
							}

							cloud.UpdatePoints(lidar_to_vehicle, color_mode, alpha, single_color, index, jet_table_);
						}
						Redraw();
					};
				}

				auto& clouds = lidar->second.clouds;
				// make a new cloud with this, reusing the last ones buffers if necessary
				Cloud* cloud = 0;
				if (clouds.size() >= history_length_->GetValue())
				{
					// pop back and push it to the front
					clouds.push_front(clouds.back());
					clouds.pop_back();
					cloud = &clouds[0];
					free(cloud->data);
					cloud->data = 0;
				}
				else
				{
					Cloud c;
					glGenBuffers(1, &c.point_vbo);
					glGenBuffers(1, &c.color_vbo);
					clouds.push_back(c);
					cloud = &clouds[clouds.size() - 1];
				}

				uint8_t alpha = 255.0 * alpha_->GetValue();

				lidar->second.UpdateFromProperties();

				// transform
				Matrix3x4 lidar_to_vehicle(lidar->second.GetQuaternion(), lidar->second.position);

				cloud->num_points = data->num_points;
				cloud->data = data->points;
				data->points = (uint8_t*)malloc(1);

				std::string color_mode = coloring_mode_->GetValue();
				auto single_color = single_color_->GetValue();
				cloud->UpdatePoints(lidar_to_vehicle, color_mode, alpha, single_color, lidar->second.index, jet_table_);

				free(data->points);
				free(data);//todo use allocator free

				Redraw();
			}
		}
	}

	virtual void Render()
	{
		if (GetCanvas()->wgs84_mode_)
		{
			return;// not supported for the moment
		}

		glPointSize(point_size_->GetValue());

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);

		bool show_inliers = show_inliers_->GetValue();
		for (auto& lidar : lidars_)
		{
			auto inliers = lidar.second.inliers;
			if (inliers.num_points && show_inliers)
			{
				glBindBuffer(GL_ARRAY_BUFFER, inliers.point_vbo);
				glVertexPointer(3, GL_FLOAT, 0, 0);
				glBindBuffer(GL_ARRAY_BUFFER, inliers.color_vbo);
				glColorPointer(4, GL_UNSIGNED_BYTE, 0, 0);

				glDrawArrays(GL_POINTS, 0, inliers.num_points);
			}

			if (!lidar.second.Show())
			{
				continue;
			}

			for (auto& cloud : lidar.second.clouds)
			{
				// render it
				glBindBuffer(GL_ARRAY_BUFFER, cloud.point_vbo);
				glVertexPointer(3, GL_FLOAT, 0, 0);
				glBindBuffer(GL_ARRAY_BUFFER, cloud.color_vbo);
				glColorPointer(4, GL_UNSIGNED_BYTE, 0, 0);

				glDrawArrays(GL_POINTS, 0, cloud.num_points);
			}
		}

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);

		for (auto& lidar : lidars_)
		{
			// render lidar position
			Matrix3x4 transform(lidar.second.GetQuaternion(), Vec3(0, 0, 0));
			Vec3 center = lidar.second.position;
			Vec3 x_axis = transform.transform(Vec3(0.5, 0, 0));
			Vec3 y_axis = transform.transform(Vec3(0, 0.5, 0));
			Vec3 z_axis = transform.transform(Vec3(0, 0, 0.5));

			//if (show_origin_)
			{
				glLineWidth(6.0f);
				glBegin(GL_LINES);
				// Red line to the right (x)
				glColor3f(1, 0, 0);
				glVertex3f(center.x, center.y, center.z);
				glVertex3f(center.x + x_axis.x, center.y + x_axis.y, center.z + x_axis.z);

				// Green line to the top (y)
				glColor3f(0, 1, 0);
				glVertex3f(center.x, center.y, center.z);
				glVertex3f(center.x + y_axis.x, center.y + y_axis.y, center.z + y_axis.z);

				// Blue line up (z)
				glColor3f(0, 0, 1);
				glVertex3f(center.x, center.y, center.z);
				glVertex3f(center.x + z_axis.x, center.y + z_axis.y, center.z + z_axis.z);

			}

			auto normal = lidar.second.normal * 5;
			glColor3f(1, 1, 1);
			glVertex3f(center.x, center.y, center.z);
			glVertex3f(center.x + normal.x, center.y + normal.y, center.z + normal.z);
			glEnd();
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	GLuint frame_buffer_ = 0;
	GLuint render_texture_ = 0;
	Gwen::Controls::Properties* tree;
	virtual void Initialize(Gwen::Controls::Properties* tree)
	{
		this->tree = tree;

		// Create our render texture for fonts
		glGenTextures(1, &render_texture_);

		// "Bind" the newly created texture : all future texture functions will modify this texture
		glBindTexture(GL_TEXTURE_2D, render_texture_);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 50, 50, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Now create the framebuffer using that texture as the color buffer
		glGenFramebuffers(1, &frame_buffer_);
		glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, render_texture_, 0);

		// Do a dummy check
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			printf("Error creating framebuffer, it is incomplete!\n");
		}

		GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, DrawBuffers);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// add any properties
		alpha_ = AddFloatProperty(tree, "Alpha", 1.0, 0.0, 1.0, 0.1, "Point transparency.");

		show_inliers_ = AddBooleanProperty(tree, "Show Inliers", false);

		history_length_ = AddNumberProperty(tree, "History Length", 10, 1, 100, 1, "Number of past pointclouds to show.");
		history_length_->onChange = std::bind(&LidarCalibrationPlugin::HistoryLengthChange, this, std::placeholders::_1);

		point_size_ = AddNumberProperty(tree, "Point Size", 4, 1, 100, 2, "Size in pixels for points.");

		coloring_mode_ = AddEnumProperty(tree, "Coloring Mode", "Lidar ID", { "Intensity", "Height", "Single Color", "Lidar ID" }, "Coloring mode.");
		coloring_mode_->onChange = std::bind(&LidarCalibrationPlugin::ColoringModeChange, this, std::placeholders::_1);

		single_color_ = AddColorProperty(tree, "Point Color", Gwen::Color(255, 255, 255), "Color for points.");

		min_value_ = AddFloatProperty(tree, "Min Value", 0.0, 0, 255.0, 1.0, "Min intensity value to include for target detection.");


		auto pRow2 = tree->Add(L"", new Gwen::Controls::Property::Button(tree), L"Load");
		pRow2->onChange.Add(this, &LidarCalibrationPlugin::OnLoad);

		auto pRow3 = tree->Add(L"", new Gwen::Controls::Property::Button(tree), L"Save");
		pRow3->onChange.Add(this, &LidarCalibrationPlugin::OnSave);

		// build color lookup tables
		for (int i = 0; i < 256; i++)
		{
			auto rainbow = Gwen::Utility::HSVToColor(i * 360.0 / 256.0, 1.0, 1.0);
			rainbow_table_[i] = rainbow;
			jet_table_[i] = GetJetColour(i, 0, 255);
		}

		ColoringModeChange("Interpolated");

		Subscribe("/scans");
	}

	// Returns interpolated color ramp values in a Jet like fashion
	Gwen::Color GetJetColour(double v, double vmin, double vmax)
	{
		Gwen::Color c(255, 255, 255);

		if (v < vmin)
			v = vmin;
		if (v > vmax)
			v = vmax;
		double dv = vmax - vmin;

		double first_split = 0.35;
		double middle_split = 0.5;
		double last_split = 0.65;

		if (v < (vmin + first_split * dv)) {
			c.r = 0;
			c.g = 255 * ((1.0 / first_split) * (v - vmin) / dv);
		}
		else if (v < (vmin + middle_split * dv)) {
			c.r = 0;
			c.b = 255 * (1 + (1.0 / (middle_split - first_split)) * (vmin + first_split * dv - v) / dv);
		}
		else if (v < (vmin + last_split * dv)) {
			c.r = 255 * ((1.0 / (last_split - middle_split)) * (v - vmin - middle_split * dv) / dv);
			c.b = 0;
		}
		else {
			c.g = 255 * (1 + (1.0 / (1.0 - last_split)) * (vmin + last_split * dv - v) / dv);
			c.b = 0;
		}

		return(c);
	}

	std::string GetTitle() override
	{
		return "Lidar Calibration";
	}
};

#endif
