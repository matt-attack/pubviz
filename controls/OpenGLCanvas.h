// Todo license

#ifndef PUBVIZ_CANVAS_H
#define PUBVIZ_CANVAS_H

#include <Gwen/Controls/Base.h>
#include <Gwen/Controls/Label.h>
#include <Gwen/Gwen.h>
#include <Gwen/Skin.h>
#include "../properties.h"

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
	public:

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

		void ResetViewOrigin()
		{
			view_x_->SetValue(0.0);
			view_y_->SetValue(0.0);
			view_z_->SetValue(0.0);

			view_abs_x_ = 0.0;
			view_abs_y_ = 0.0;
			view_abs_z_ = 0.0;
			view_lat_ = local_xy_.OriginLatitude();
			view_lon_ = local_xy_.OriginLongitude();
			view_alt_ = 0.0;
			Redraw();
		}

		double view_abs_x_ = 0.0;
		double view_abs_y_ = 0.0;
		double view_abs_z_ = 0.0;

		double view_lat_ = 0.0;
		double view_lon_ = 0.0;
		double view_alt_ = 0.0;


		// sets the origin if it hasnt already been set
		void SetLocalXY(double lat, double lon)
		{
			if (!local_xy_.Initialized() && lat != 0.0 && lon != 0.0)
			{
				local_xy_ = LocalXYUtil(lat, lon);
			}
		}
		
		void SetViewOrigin(double x, double y, double z, double lat, double lon, double alt)
		{
			view_x_->SetValue(x);
			view_y_->SetValue(y);
			view_z_->SetValue(z);
			view_lat_ = lat;
			view_lon_ = lon;
			view_alt_ = alt;
			view_abs_z_ = alt;
			SetLocalXY(lat, lon);
			local_xy_.FromLatLon(view_lat_, view_lon_, view_abs_x_, view_abs_y_);

			Redraw();
		}

		void ResetOrigin()
		{
			local_xy_ = LocalXYUtil();
		}

		void Screenshot();

		bool wgs84_mode_ = false;
		void SetFrame(bool wgs84)
		{
			wgs84_mode_ = wgs84;
		}

		bool show_origin_ = true;
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

		std::map<std::string, PropertyBase*> CreateProperties(Gwen::Controls::Properties* props);

	protected:
	
		void OnMouseMoved(int x, int y, int dx, int dy) override;
		bool OnMouseWheeled( int iDelta ) override;
		void OnMouseClickLeft( int /*x*/, int /*y*/, bool /*bDown*/ ) override;
		void OnMouseClickRight( int /*x*/, int /*y*/, bool /*bDown*/ ) override;

		Gwen::Color	m_Color;
		double view_height_m_;
		bool mouse_down_ = false;

		Gwen::Point select_start_,select_end_;
		bool selecting_ = false;
		std::vector<pubviz::AABB> selected_aabbs_;
		
		//double view_x_ = 0.0;
		//double view_y_ = 0.0;
		//double view_z_ = 0.0;

		bool paused_ = false;
		
		//ViewType view_type_ = ViewType::Orbit;

		// Properties
		EnumProperty* view_type_;
		FloatProperty* pitch_;
		FloatProperty* yaw_;
		FloatProperty* view_x_;
		FloatProperty* view_y_;
		FloatProperty* view_z_;
		
		double x_mouse_position_ = 0.0;
		double y_mouse_position_ = 0.0;
};

#endif
