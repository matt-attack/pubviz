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
		
		virtual bool OnMouseWheeled( int iDelta );
		
		std::vector<Plugin*> plugins_;
		
		void GetMousePosition(double& x, double& y)
		{
			x = x_position_;
			y = y_position_;
		}

	protected:
	
		void OnMouseMoved(int x, int y, int dx, int dy) override;

		Gwen::Color		m_Color;
		double view_height_m_;
		
		double x_position_ = 0.0;
		double y_position_ = 0.0;

};

#endif
