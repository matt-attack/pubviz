// Todo license

#ifndef PUBVIZ_CANVAS_H
#define PUBVIZ_CANVAS_H

#include <Gwen/Controls/Base.h>
#include <Gwen/Controls/Label.h>
#include <Gwen/Gwen.h>
#include <Gwen/Skin.h>
#include "../properties.h"

#include <pubsub_cpp/Matrix3x4.h>

#include "../LocalXY.h"

#include "../AABB.h"

namespace ViewType
{
	constexpr const char* Orbit = "Orbit";
	constexpr const char* TopDown = "Top Down";
	constexpr const char* FPS = "FPS";
}

namespace pubviz
{
	class Plugin;
}
class PubViz;
class OpenGLCanvas : public Gwen::Controls::Base
{
	friend class PubViz;
		unsigned int selection_texture_ = 0;
		unsigned int selection_frame_buffer_ = 0;

		void SetupViewMatrices();

		bool wgs84_mode_ = false;

		bool show_origin_ = true;

		double center_x_ = 0;
		double center_y_ = 0;
		double center_z_ = 0;


		double view_width_ = 0;
		double view_height_ = 0;

		Matrix3x4d map_to_odom_;
		Matrix3x4d odom_to_map_;
	public:

		inline double view_width() { return view_width_; }
		inline double view_height() { return view_height_; }

		LocalXYUtil local_xy_;

		GWEN_CONTROL( OpenGLCanvas, Gwen::Controls::Base );

		virtual void Render( Gwen::Skin::Base* skin );

		const Gwen::Color & GetColor() { return m_Color; }
		void SetColor( const Gwen::Color & col ) { m_Color = col; }
		
		std::vector<pubviz::Plugin*> plugins_;
		
		void GetMousePosition(double& x, double& y)
		{
			x = x_mouse_position_;
			y = y_mouse_position_;
		}
		
		void ResetView();
		
		void SetViewType(std::string type)
		{
			view_type_->SetValue(type);
			
			ResetView();
			
			Redraw();
		}

		enum Frame
		{
			Map,
			Odom,
			WGS84
		};

		void TransformToFrame(Frame src, Frame dst, Vec3d& pos) const
		{
			if (dst == Map)
			{
				if (src == Map)
				{
					// do nothing
				}
				else if (src == Odom)
				{
					// transform to map
					pos = odom_to_map_.transform(pos);
				}
				else//wgs84
				{
					// wgs84 to map
					double x, y;
					local_xy_.FromLatLon(pos.x, pos.y, x, y);
					pos.x = x;
					pos.y = y;
				}
			}
			else if (dst == WGS84)
			{
				if (src == Map)
				{
					double lat, lon;
					local_xy_.ToLatLon(pos.x, pos.y, lat, lon);
					pos.x = lat;
					pos.y = lon;
				}
				else if (src == Odom)
				{
					pos = odom_to_map_.transform(Vec3d(pos.x, pos.y, pos.z));
					double lat, lon;
					local_xy_.ToLatLon(pos.x, pos.y, lat, lon);
					pos.x = lat;
					pos.y = lon;
				}
				else// wgs84
				{
					// do nothing
				}
			}
			else// odom
			{
				if (src == Map)
				{
					// transform to odom
					pos = map_to_odom_.transform(pos);
				}
				else if (src == Odom)
				{
					// do nothing
				}
				else// wgs84
				{
					// transform to map then odom
					double x, y;
					local_xy_.FromLatLon(pos.x, pos.y, x, y);
					pos = map_to_odom_.transform(Vec3d(x, y, pos.z));
				}
			}
		}

		void TransformToView(Frame frame, Vec3d& pos) const
		{
			TransformToFrame(frame, wgs84_mode_ ? Map : Odom, pos);
		}

		inline std::string GetViewType()
		{
			return view_type_->GetValue();
		}

		inline bool Paused()
		{
			return paused_;
		}

		inline void SetPaused(bool paused)
		{
			paused_ = paused;
		}

		void ResetViewPosition()
		{
			view_x_->SetValue(0.0);
			view_y_->SetValue(0.0);
			view_z_->SetValue(0.0);

			Redraw();
		}

		// okay, so the view has two parts, an origin (either 0 or set by a pose)
		// then an offset

		void GetViewCenter(double& x, double& y, double& z)
		{
			x = center_x_ + view_x_->GetValue();
			y = center_y_ + view_y_->GetValue();
			z = center_z_ + view_z_->GetValue();
		}

		// sets the origin if it hasnt already been set
		void SetLocalXY(double lat, double lon)
		{
			if (!local_xy_.Initialized() && lat != 0.0 && lon != 0.0)
			{
				printf("initialized local xy to %f %f\n", lat, lon);
				local_xy_ = LocalXYUtil(lat, lon);
			}
		}

		// Sets the transform between wgs84 and odom
		void SetTransform(double x, double y, double z, double yaw, double lat, double lon, double alt, double ayaw)
		{
			SetLocalXY(lat, lon);
			// okay, setup map to odom tran
			// okay, lets calculate the transform between map and odom

			// todo change all this to doubles later

			// first we want to get the position in vehicle frame
			// todo add angles
			Quaternion odom_rot = Quaternion::FromAngleAxis(yaw, Vec3f(0,0,1));
			Matrix3x4d odom_to_vehicle(Quaternion(), Vec3d(-x,-y,-z));
			Matrix3x4d rot_vehicle(odom_rot.inverse(), Vec3d(0,0,0));
			Quaternion map_rot = Quaternion::FromAngleAxis(ayaw, Vec3f(0,0,1));
//ah ha, missing these
			double abs_x, abs_y;
			local_xy_.FromLatLon(lat, lon, abs_x, abs_y);
			Matrix3x4d vehicle_to_map(map_rot, Vec3d(abs_x, abs_y, alt));

			odom_to_map_ = (odom_to_vehicle*rot_vehicle)*vehicle_to_map;

			// then we want to add the vehicl
			map_to_odom_ = odom_to_map_;
			map_to_odom_.invert();
		}
		
		// Sets the view origin
		void SetViewOrigin(double x, double y, double z, double lat, double lon, double alt)
		{
			SetLocalXY(lat, lon);

			if (!wgs84_mode_) {
				center_x_ = x;
				center_y_ = y;
				center_z_ = z;
			} else {
				local_xy_.FromLatLon(lat, lon, center_x_, center_y_);
				center_z_ = alt;
			}

			Redraw();
		}

		void ResetOrigin()
		{
			local_xy_ = LocalXYUtil();
		}

		void Screenshot();

		void SetFrame(bool wgs84)
		{
			if (wgs84 != wgs84_mode_)
			{
				// recenter if we have one
				// todo
				center_x_ = 0;
				center_y_ = 0;
				center_z_ = 0;
				view_x_->SetValue(0.0);
				view_y_->SetValue(0.0);
				view_z_->SetValue(0.0);
			}
			wgs84_mode_ = wgs84;
			Redraw();
		}

		inline bool wgs84_mode() { return wgs84_mode_; }

		void ShowOrigin(bool yn)
		{
			show_origin_ = yn;
		}
		
		virtual void Layout( Gwen::Skin::Base* skin ) override;
		
		void SetViewAngle(double pitch, double yaw)
		{
			if (view_type_->GetValue() == ViewType::Orbit)
			{
				if (pitch_->GetValue() != pitch || yaw_->GetValue() != yaw)
				{
					Redraw();
				}
				pitch_->SetValue(pitch);
				yaw_->SetValue(yaw);
			}
		}

		void WorldToPixel(double x, double y, double z, int& px, int& py);

		// always on 0 plane
		//void PixelToWorld(int px, int py, double x, double y)

		std::map<std::string, PropertyBase*> CreateProperties(Gwen::Controls::Properties* props);

	protected:

		bool shift_select_ = false;
		void DoPick();
	
		void OnMouseMoved(int x, int y, int dx, int dy) override;
		bool OnMouseWheeled( int iDelta ) override;
		void OnMouseClickLeft( int /*x*/, int /*y*/, bool /*bDown*/ ) override;
		void OnMouseClickRight( int /*x*/, int /*y*/, bool /*bDown*/ ) override;
		void OnMouseDoubleClickLeft(int, int) override;
		void OnMouseLeave() override;
		
		void OnClear(Gwen::Controls::Base* c);

		Gwen::Color	m_Color;
		//double view_height_m_;
		bool mouse_down_ = false;

		Gwen::Point select_start_,select_end_;
		bool selecting_ = false;
		std::vector<pubviz::AABB> selected_aabbs_;

		bool paused_ = false;

		// Properties
		EnumProperty* view_type_;
		FloatProperty* pitch_;
		FloatProperty* yaw_;
		FloatProperty* view_x_;
		FloatProperty* view_y_;
		FloatProperty* view_z_;
		FloatProperty* view_h_;
		
		double x_mouse_position_ = 0.0;
		double y_mouse_position_ = 0.0;

		float proj_[16];
		float model_[16];
		int vp_[4];
};

#endif
