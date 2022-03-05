// Todo license

#ifndef PUBVIZ_CANVAS_H
#define PUBVIZ_CANVAS_H

#include <Gwen/Controls/Base.h>
#include <Gwen/Controls/Label.h>
#include <Gwen/Gwen.h>
#include <Gwen/Skin.h>

#include "Plugin.h"

class OpenGLCanvas : public Gwen::Controls::Base
{
	public:

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

	protected:
	
		void OnMouseMoved(int x, int y, int dx, int dy) override;
		bool OnMouseWheeled( int iDelta ) override;
		void OnMouseClickLeft( int /*x*/, int /*y*/, bool /*bDown*/ ) override;

		Gwen::Color		m_Color;
		double view_height_m_;
		bool mouse_down_ = false;
		
		double view_x_ = 0.0;
		double view_y_ = 0.0;
		
		double x_mouse_position_ = 0.0;
		double y_mouse_position_ = 0.0;
};

#endif