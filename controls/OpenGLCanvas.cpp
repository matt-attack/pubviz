// todo license

#include <Gwen/Platform.h>

#include "OpenGLCanvas.h"

#include <GL/glew.h>

#include "../Plugin.h"
#include "pubviz.h"

#ifndef _WIN32
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#endif

#include <cmath>

#undef near

using namespace Gwen;
using namespace Gwen::Controls;

GWEN_CONTROL_CONSTRUCTOR( OpenGLCanvas )
{
	view_height_m_ = 150.0;
	m_Color = Gwen::Color( 50, 50, 50, 255 );
}

bool OpenGLCanvas::OnMouseWheeled( int delta )
{
	if (view_type_->GetValue() == ViewType::FPS)
	{
		// move view along view axis
		double dx = cos(-yaw_->GetValue()*M_PI/180.0)*cos(pitch_->GetValue()*M_PI/180.0);
		double dy = sin(-yaw_->GetValue()*M_PI/180.0)*cos(pitch_->GetValue()*M_PI/180.0);
		double dz = sin(pitch_->GetValue()*M_PI/180.0);
		view_x_->SetValue(view_x_->GetValue() + dx*delta*0.01);
		view_y_->SetValue(view_y_->GetValue() + dy*delta*0.01);
		view_z_->SetValue(view_z_->GetValue() + dz*delta*0.01);

		Redraw();

		return true;
	}

	if (delta < 0)
	{
		view_height_m_ += 0.1*(double)delta;
		view_height_m_ = std::max(1.0, view_height_m_);
	}
	else
	{
		view_height_m_ += 0.1*(double)delta;
	}
	
	// Mark the window as dirty so it redraws
	Redraw();
	
	return true;
}

void OpenGLCanvas::ResetView()
{
	view_height_m_ = 150.0;
	view_x_->SetValue(0.0);
	view_y_->SetValue(0.0);
	view_z_->SetValue(0.0);
	view_abs_x_ = 0.0;
	view_abs_y_ = 0.0;
	view_abs_z_ = 0.0;
	pitch_->SetValue(0.0);
	yaw_->SetValue(0.0);
	
	Redraw();
}

void OpenGLCanvas::OnMouseClickLeft( int /*x*/, int /*y*/, bool down )
{
	mouse_down_ = down;
}

void OpenGLCanvas::OnMouseClickRight( int x, int y, bool bDown )
{
	if (selecting_)
	{
		auto r = GetSkin()->GetRender();
		// finish selecting. do a render

		// first create render target
		if (selection_texture_ == 0)
		{
			glGenTextures(1, &selection_texture_);
			glGenFramebuffers(1, &selection_frame_buffer_);
		}

		// "Bind" the newly created texture : all future texture functions will modify this texture
		glBindTexture(GL_TEXTURE_2D, selection_texture_);
		
		// todo size it properly
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 50, 50, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Now create the framebuffer using that texture as the color buffer
		glBindFramebuffer(GL_FRAMEBUFFER, selection_frame_buffer_);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, selection_texture_, 0);  
		
		// Do a dummy check
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			printf("Error creating framebuffer, it is incomplete!\n");
		}
		
		GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
		glDrawBuffers(1, DrawBuffers);
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
				
		// Resize the image to our new target size
		glBindTexture(GL_TEXTURE_2D, selection_texture_);

		auto scale = GetCanvas()->Scale();
    	int width = Width()*scale;
    	int height = Height()*scale;
		
		// Give an empty image to OpenGL ( the last "0" )
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
		
		glBindFramebuffer(GL_FRAMEBUFFER, selection_frame_buffer_);

    	float vp[4];
    	glGetFloatv(GL_VIEWPORT, vp);
		glViewport(0, 0, width, height);

		glClearColor(1.0, 1.0, 1.0, 1.0);
		
		glClear( GL_COLOR_BUFFER_BIT );
		
		auto origin = LocalPosToCanvas();
		origin.y -= 20;// skip past menu bar
	
		// force a flush essentially
		r->EndClip();
		r->StartClip();
        
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glPushAttrib(GL_ALL_ATTRIB_BITS);

		SetupViewMatrices();

		// now render
		uint32_t current_id = 0xFF000000;
		struct plugin_info
		{
			uint32_t start_id;
			uint32_t end_id;
			pubviz::Plugin* plugin;
		};
		std::vector<plugin_info> visible_plugins;
		for (auto plugin: plugins_)
		{
			if (plugin->Enabled())
			{
				uint32_t start_id = current_id;
				current_id = plugin->RenderSelect(current_id);
				uint32_t end_id = current_id;
				current_id += 1;
				// save the id ranges for this plugin so we can map from select id to plugin
				if (start_id != end_id)
				{
					plugin_info info;
					info.start_id = start_id & 0xFFFFFF;
					info.end_id = end_id & 0xFFFFFF;
					info.plugin = plugin;
					visible_plugins.push_back(info);
				}
			}
		}
		
		// Force a flush
		r->EndClip();


		glFlush();
		glFinish(); 

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


		// Now read out the selected area
		auto start = CanvasPosToLocal(select_start_);
		auto end = CanvasPosToLocal(select_end_);
		int32_t start_x = std::min(start.x, end.x)*scale;
		int32_t sw = std::max<int>(std::abs(start.x - end.x)*scale,1);
		int32_t sh = std::max<int>(std::abs(start.y - end.y)*scale,1);
		int32_t start_y = std::min(height - start.y, height - end.y)*scale;
		uint32_t* pixels = new uint32_t[sw*sh];
		glReadPixels(start_x, start_y, sw, sh, GL_RGBA, GL_UNSIGNED_BYTE, (uint8_t*)pixels);

		// Find all selections
		std::unordered_map<uint32_t, bool> selected;
		for (int i = 0; i < sw*sh; i++)
		{
			if (pixels[i] != 0xFFFFFFFF)
			{
				selected[pixels[i] & 0x00FFFFFF] = true;
			}
		}
		// Then select all selections
		PubViz* p = (PubViz*)GetParent();

		// add to selection if shift is pressed
		// todo remove from selection if ctrl is pressed
		if (!Gwen::Input::IsKeyDown(Gwen::Key::Shift))
		{
			// todo dont allow double selection in shift mode
			p->GetSelection()->Clear();
			selected_aabbs_.clear();
		}
		auto sel = p->GetSelection();
		for (auto id: selected)
		{
			// Find the associated plugin with this index
			for (const auto& p: visible_plugins)
			{
				if (id.first <= p.end_id && id.first >= p.start_id)
				{
					//printf("Selected %i from plugin %s\n", id.first, p.plugin->GetTitle().c_str());
					// Add properties about this selected item to our selection list
					auto node = sel->AddNode("Point (" + std::to_string(id.first) + ")");
					pubviz::AABB aabb;
					auto map = p.plugin->Select(id.first - p.start_id, aabb);
					for (auto& kv: map)
					{
						node->AddNode(kv.first + ": " + kv.second);
					}

					// add it to the list of selections to render
					selected_aabbs_.push_back(aabb);
					//todo need selection size so we can draw it
					break;
				}
			}
		}
		delete[] pixels;

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// restore matrices and viewport
		glPopAttrib();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
	
		glDisable(GL_DEPTH_TEST);

    	glViewport(vp[0], vp[1], vp[2], vp[3]);

		Redraw();
	}

	selecting_ = bDown;
	select_start_ = select_end_ = Gwen::Point(x, y);
}

void OpenGLCanvas::OnMouseMoved(int x, int y, int dx, int dy)
{
	// now convert to units
	double pixels_per_meter = GetCanvas()->Height()/view_height_m_;
	x_mouse_position_ = (x - GetCanvas()->Width()*0.5)/pixels_per_meter + view_x_->GetValue();
	y_mouse_position_ = (GetCanvas()->Height()*0.5 - y)/pixels_per_meter + view_y_->GetValue();
	
	// now apply offset
	if (mouse_down_)
	{
		if (view_type_->GetValue() == ViewType::TopDown)
		{
			view_x_->SetValue(view_x_->GetValue() - dx/pixels_per_meter);
			view_y_->SetValue(view_y_->GetValue() + dy/pixels_per_meter);

			view_abs_x_ -= dx / pixels_per_meter;
			view_abs_y_ += dy / pixels_per_meter;
		}
		else
		{
			double new_pitch = pitch_->GetValue() + dy;
			double new_yaw = yaw_->GetValue() + dx;

			pitch_->SetValue(new_pitch);
			yaw_->SetValue(new_yaw);
		}
		
		// Mark the window as dirty so it redraws
		Redraw();
	}

	if (selecting_)
	{
		select_end_ = Gwen::Point(x,y);
		Redraw();
	}
}

#include <pubsub_cpp/Time.h>
#include <Gwen/../../Renderers/OpenGL/FreeImage/FreeImage.h>

void OpenGLCanvas::Screenshot()
{
	auto scale = GetCanvas()->Scale();
	auto origin = LocalPosToCanvas();
	origin.y -= 20;// skip past the menu bar
	int width = Width()*scale;
	int height = Height()*scale;
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	uint8_t* pixels = new uint8_t[3*width*height];

	glReadPixels(origin.x * scale, origin.y * scale - 1, width, height - 1, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	FIBITMAP* image = FreeImage_ConvertFromRawBits(pixels, width, height, 3 * width, 24, 0x0000FF, 0xFF0000, 0x00FF00, false);
	char str[500];
	int len = snprintf(str, 500, "Screenshot %s.bmp", pubsub::Time::now().toString().c_str());
	for (int i = 0; i < len; i++)
	{
		if (str[i] == ':' || str[i] == ' ')
			str[i] = '_';
	}

	FreeImage_Save(FIF_BMP, image, str, 0);
	delete[] pixels;
}

void OpenGLCanvas::Layout( Gwen::Skin::Base* skin )
{
	// Update each plugin here to see if they want a redraw
	for (auto plugin: plugins_)
	{
		if (plugin->Enabled())
		{
			plugin->Update();
		}
	}
	Invalidate();// we are being hacky, just always invalidate so we keep laying out
}

std::map<std::string, PropertyBase*> OpenGLCanvas::CreateProperties(Gwen::Controls::Properties* tree)
{
	std::map<std::string, PropertyBase*> props;
	view_type_ = new EnumProperty(tree, "View Type", "Orbit", {"Orbit", "Top Down", "FPS"});
	props["view"] = view_type_;
	yaw_ = new FloatProperty(tree, "Yaw", 0, -100000, 100000);
	props["yaw"] = yaw_;
	pitch_ = new FloatProperty(tree, "Pitch", 0, -100000, 100000);
	props["pitch"] = pitch_;
	view_x_ = new FloatProperty(tree, "View X", 0, -100000, 100000);
	props["View X"] = view_x_;
	view_y_ = new FloatProperty(tree, "View Y", 0, -100000, 100000);
	props["View Y"] = view_y_;
	view_z_ = new FloatProperty(tree, "View Z", 0, -100000, 100000);
	props["View Z"] = view_z_;

	view_type_->onChange = [this](std::string value)
	{
		pitch_->Hide();
		yaw_->Hide();
		view_z_->Hide();
		if (value == ViewType::Orbit)
		{
			pitch_->Show();
			yaw_->Show();
			view_z_->Show();
		}
		else if (value == ViewType::FPS)
		{
			pitch_->Show();
			yaw_->Show();
			view_z_->Show();
		}
		else if (value == ViewType::TopDown)
		{
			
		}
	};

	return props;
}

void OpenGLCanvas::SetupViewMatrices()
{
	auto width = Width();
    auto height = Height();
	double view_x = view_x_->GetValue();
	double view_y = view_y_->GetValue();
	double view_z = view_z_->GetValue();
	if (wgs84_mode_)
	{
		view_x = view_abs_x_;
		view_y = view_abs_y_;
		view_z = view_abs_z_;
	}
	double yaw = yaw_->GetValue();
	double pitch = pitch_->GetValue();
	auto view_type = view_type_->GetValue();
	if (view_type == ViewType::TopDown)
	{
		// set up the view matrix for the current zoom level (ortho, topdown)
		float half_height = view_height_m_/2.0;
		float half_width = half_height*((float)width/(float)height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho( -half_width + view_x, half_width + view_x, -half_height + view_y, half_height + view_y, -10000.0, 10000.0 );
	
		// now apply other transforms
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}
	else if (view_type == ViewType::FPS)
	{
		double near = 1.0;
		double fov = 45*M_PI/180.0;// horizontal
		double aspect_ratio = ((double)width)/((double)height);
		double width = near*tan(fov);
		double height = width/aspect_ratio;
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		// x, y, z
		glFrustum( -width/2, width/2, -height/2, height/2, near, 10000.0 );
	
		// now apply other transforms
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		// eh, I dislike this but whatever
		gluLookAt(view_x, view_y, view_z, /* look from camera XYZ */
             view_x - cos(-yaw*M_PI/180.0)*cos(pitch*M_PI/180.0), view_y - sin(-yaw*M_PI/180.0)*cos(pitch*M_PI/180.0), view_z - sin(pitch*M_PI/180.0), /* look at the origin */
             0, 0, 1); /* positive Z up vector */
		
		// we want depth testing in this mode
		glEnable(GL_DEPTH_TEST);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LEQUAL);
	}
	else if (view_type == ViewType::Orbit)
	{
		// set up the view matrix for the current zoom level (orbit)
		float half_height = view_height_m_/2.0;
		float half_width = half_height*((float)width/(float)height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho( -half_width, half_width, -half_height, half_height, -10000.0, 10000.0 );
	
		// now apply other transforms
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	
		glRotatef(-90, 1, 0, 0);
		glRotatef(yaw, 0, 0, 1);
	
		float ax = cos((-yaw)*3.14159/180.0);
		float ay = sin((-yaw)*3.14159/180.0);
		glRotatef(pitch, ax, ay, 0);
		
		glTranslatef(-view_x, -view_y, -view_z);
		
		// we want depth testing in this mode
		glEnable(GL_DEPTH_TEST);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LEQUAL);
	}
}

void OpenGLCanvas::Render( Skin::Base* skin )
{
	auto r = skin->GetRender();
	
	// do whatever we want here
	skin->GetRender()->SetDrawColor( m_Color );// start by clearing to background color
	skin->GetRender()->DrawFilledRect( GetRenderBounds() );

    auto origin = LocalPosToCanvas();
	origin.y -= 20;// skip past menu bar
    auto width = Width();
    auto height = Height();
	
	// force a flush essentially
	r->EndClip();
	r->StartClip();
        
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glPushAttrib(GL_ALL_ATTRIB_BITS);

    float vp[4];
    glGetFloatv(GL_VIEWPORT, vp);
	auto scale = GetCanvas()->Scale();
	glViewport(origin.x * scale, origin.y * scale, width * scale, height * scale);
	
	SetupViewMatrices();

	// Draw axes at origin
	if (show_origin_)
	{
		glLineWidth(6.0f);
		glBegin(GL_LINES);
		// Red line to the right (x)
		glColor3f(1, 0, 0);
		glVertex3f(0, 0, 0);
		glVertex3f(55, 0, 0);

		// Green line to the top (y)
		glColor3f(0, 1, 0);
		glVertex3f(0, 0, 0);
		glVertex3f(0, 55, 0);
	
		// Blue line up (z)
		glColor3f(0, 0, 1);
		glVertex3f(0, 0, 0);
		glVertex3f(0, 0, 55);
		glEnd();
	}
	
	// Now draw all the plugins
	for (auto plugin: plugins_)
	{
		if (plugin->Enabled())
		{
			plugin->Render();
		}
	}

	// Now draw any selections
	if (selected_aabbs_.size())
	{
		glLineWidth(5.0);
		glBegin(GL_LINES);
		for (const auto& a: selected_aabbs_)
		{
			glColor3f(1, 1, 1);
			// lower xy plane
			glVertex3f(a.x       , a.y       , a.z       );
			glVertex3f(a.x + a.sx, a.y       , a.z       );

			glVertex3f(a.x + a.sx, a.y       , a.z       );
			glVertex3f(a.x + a.sx, a.y + a.sy, a.z       );

			glVertex3f(a.x + a.sx, a.y + a.sy, a.z       );
			glVertex3f(a.x       , a.y + a.sy, a.z       );

			glVertex3f(a.x       , a.y + a.sy, a.z       );
			glVertex3f(a.x       , a.y       , a.z       );

			// upper xy plane
			glVertex3f(a.x       , a.y       , a.z + a.sz);
			glVertex3f(a.x + a.sx, a.y       , a.z + a.sz);

			glVertex3f(a.x + a.sx, a.y       , a.z + a.sz);
			glVertex3f(a.x + a.sx, a.y + a.sy, a.z + a.sz);

			glVertex3f(a.x + a.sx, a.y + a.sy, a.z + a.sz);
			glVertex3f(a.x       , a.y + a.sy, a.z + a.sz);

			glVertex3f(a.x       , a.y + a.sy, a.z + a.sz);
			glVertex3f(a.x       , a.y       , a.z + a.sz);

			// remaining lines
			glVertex3f(a.x       , a.y       , a.z       );
			glVertex3f(a.x       , a.y       , a.z + a.sz);

			glVertex3f(a.x + a.sx, a.y       , a.z       );
			glVertex3f(a.x + a.sx, a.y       , a.z + a.sz);

			glVertex3f(a.x       , a.y + a.sy, a.z       );
			glVertex3f(a.x       , a.y + a.sy, a.z + a.sz);

			glVertex3f(a.x + a.sx, a.y + a.sy, a.z       );
			glVertex3f(a.x + a.sx, a.y + a.sy, a.z + a.sz);
		}
		glEnd();
	}
	
	glPopAttrib();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	
	glDisable(GL_DEPTH_TEST);

    glViewport(vp[0], vp[1], vp[2], vp[3]);
	
	// reset matrices
	r->Begin();

	// force a flush essentially
	r->StartClip();

	// Paint all the plugins
	for (auto plugin: plugins_)
	{
		if (plugin->Enabled())
		{
			plugin->Paint();
		}
	}

	// Draw the selection box
	if (selecting_)
	{
		skin->GetRender()->SetDrawColor( Gwen::Color(255, 0, 0, 128) );// start by clearing to background color
		Gwen::Rect r;
		auto p = CanvasPosToLocal(select_start_);
		r.x = select_start_.x;
		r.y = select_start_.y;
		r.w = select_end_.x - r.x;
		r.h = select_end_.y - r.y;
		r.x = p.x;
		r.y = p.y;
		skin->GetRender()->DrawFilledRect( r );
	}
}
