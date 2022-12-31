// Todo license

#ifndef PUBVIZ_CANVAS_H
#define PUBVIZ_CANVAS_H

#include <Gwen/Controls/Base.h>
#include <Gwen/Controls/Label.h>
#include <Gwen/Gwen.h>
#include <Gwen/Skin.h>

#include "../LocalXY.h"

enum class ViewType
{
	TopDown = 0,
	Orbit = 1
};

class Plugin;
class PubViz;
class OpenGLCanvas : public Gwen::Controls::Base
{
	friend class PubViz;
	public:

		LocalXYUtil local_xy_;

		GWEN_CONTROL( OpenGLCanvas, Gwen::Controls::Base );

		virtual void Render( Gwen::Skin::Base* skin );

		const Gwen::Color & GetColor() { return m_Color; }
		void SetColor( const Gwen::Color & col ) { m_Color = col; }
		
		std::vector<Plugin*> plugins_;
		
		void GetMousePosition(double& x, double& y)
		{
			x = x_mouse_position_;
			y = y_mouse_position_;
		}
		
		void ResetView();
		
		void SetViewType(ViewType type)
		{
			view_type_ = type;
			
			ResetView();
			
			Redraw();
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
			view_x_ = 0.0;
			view_y_ = 0.0;
			view_z_ = 0.0;

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
			view_x_ = x;
			view_y_ = y;
			view_z_ = z;
			view_lat_ = lat;
			view_lon_ = lon;
			view_alt_ = alt;
			view_abs_z_ = alt;
			SetLocalXY(lat, lon);
			local_xy_.FromLatLon(view_lat_, view_lon_, view_abs_x_, view_abs_y_);

			Redraw();
		}

		bool wgs84_mode_ = false;
		void SetFrame(bool wgs84)
		{
			wgs84_mode_ = wgs84;
		}
		
		virtual void Layout( Gwen::Skin::Base* skin ) override;
		
		void SetViewAngle(double pitch, double yaw)
		{
			if (view_type_ == ViewType::Orbit)
			{
				pitch_ = pitch;
				yaw_ = yaw;
			}
		}

	protected:
	
		void OnMouseMoved(int x, int y, int dx, int dy) override;
		bool OnMouseWheeled( int iDelta ) override;
		void OnMouseClickLeft( int /*x*/, int /*y*/, bool /*bDown*/ ) override;

		Gwen::Color		m_Color;
		double view_height_m_;
		bool mouse_down_ = false;
		
		double view_x_ = 0.0;
		double view_y_ = 0.0;
		double view_z_ = 0.0;
		
		double pitch_ = 0.0;
		double yaw_ = 0.0;

		bool paused_ = false;
		
		ViewType view_type_ = ViewType::Orbit;
		
		double x_mouse_position_ = 0.0;
		double y_mouse_position_ = 0.0;
};

#endif
