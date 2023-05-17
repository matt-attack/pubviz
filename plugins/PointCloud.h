
#ifndef PUBVIZ_PLUGIN_POINTCLOUD_H
#define PUBVIZ_PLUGIN_POINTCLOUD_H


#include <pubsub/PointCloud.msg.h>
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

class PointCloudPlugin: public Plugin
{
	FloatProperty* alpha_;
	StringProperty* point_text_;
	NumberProperty* point_size_;
	NumberProperty* history_length_;

    // Coloring Properties
    EnumProperty* coloring_mode_;

    // Used in all modes besides single color
    EnumProperty* coloring_field_;// point field to use for coloring
    
    // Used in interpolated mode and clamped mode
	ColorProperty* min_color_;
	ColorProperty* max_color_;
    BooleanProperty* auto_min_max_;

    // Used when auto min_max is false
    FloatProperty* min_value_;
    FloatProperty* max_value_;

    // Used in clamped mode
    ColorProperty* floored_color_;
    ColorProperty* ceiled_color_;

    // Used 

    // Used in single color mode
    ColorProperty* single_color_;
	
	TopicProperty* topic_;
	
	bool sub_open_ = false;
	ps_sub_t subscriber_;
	
	struct Cloud
	{
		int num_points;
		unsigned int point_vbo;
		unsigned int color_vbo;
	};
	
	std::deque<Cloud> clouds_;
	
	void HistoryLengthChange(int length)
	{
		while (clouds_.size() > length)
		{
			const auto& cloud = clouds_.back();
			glDeleteBuffers(1, &cloud.point_vbo);
			glDeleteBuffers(1, &cloud.color_vbo);
			clouds_.pop_back();
		}
	}

    void ColoringModeChange(std::string mode)
    {
        // hide all properties to start
        //auto canvas = min_color_->GetCanvas();
        min_color_->Hide();
        max_color_->Hide();
        ceiled_color_->Hide();
        floored_color_->Hide();
        single_color_->Hide();
        coloring_field_->Hide();
        auto_min_max_->Hide();
        AutoMinMaxChange(auto_min_max_->GetValue());
        // show the desired properties
        if (mode == "Interpolated")
        {
            min_color_->Show();
            max_color_->Show();
            coloring_field_->Show();
            auto_min_max_->Show();
        }
        else if (mode == "Clamped")
        {
            min_color_->Show();
            max_color_->Show();
            ceiled_color_->Show();
            floored_color_->Show();
            coloring_field_->Show();
            auto_min_max_->Show();
        }
        else if (mode == "Single Color")
        {
            single_color_->Show();
            coloring_field_->Show();
            min_value_->Hide();
            max_value_->Hide();
        }
        else if (mode == "Jet")
        {
            coloring_field_->Show();  
            auto_min_max_->Show();          
        }
        else if (mode == "Rainbow")
        {
            coloring_field_->Show();
            auto_min_max_->Show();
        }
    }

    void AutoMinMaxChange(bool value)
    {
        if (value)
        {
            min_value_->Hide();
            max_value_->Hide();
        }
        else
        {
            min_value_->Show();
            max_value_->Show();
        }
    }
	
	// Update our point texture if this changes
	void TextChange(std::string value)
	{
		if (value.length() == 0)
		{
			return;
		}
		
		// Create the font if we havent already
		static Gwen::Font* f = 0;
		if (!f)
		{
			f = new Gwen::Font;
			*f = *tree->GetSkin()->GetDefaultFont();
			f->size = 60;
			f->facename = L"NotoEmoji-Bold.ttf";// the fallback font will handle normal characters
		}
		
		auto r = tree->GetSkin()->GetRender();
		r->SetDrawColor(Gwen::Color(255, 0, 255, 255));
		
		// Measure the text to determine the best texture size
		auto s = r->MeasureText(f, value);
		
		// Resize the image to our new target size
		glBindTexture(GL_TEXTURE_2D, render_texture_);
		
		int ortho_x_ = s.x;
		int width_ = s.x;
		int height_ = s.y;
		int ortho_y_ = s.y;

		// Give an empty image to OpenGL ( the last "0" )
		char* data = new char[4*50*50];
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
		
		glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_);
		glViewport(0, 0, 50, 50);

		glClearColor(1.0, 1.0, 1.0, 0.0);
		
		glClear( GL_COLOR_BUFFER_BIT );
		
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho( 0, ortho_x_, 0, ortho_y_, -1.0, 1.0 );
		glMatrixMode( GL_MODELVIEW );
		glViewport(0, 0, width_, height_);
		
		// Actually render in the text
		r->RenderText(f, Gwen::PointF(0.0, 0.0), value);
		
		// Force a flush
		r->EndClip();
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	
	std::string current_topic_;
	void Subscribe(std::string str)
	{
		if (sub_open_)
		{
			ps_sub_destroy(&subscriber_);
		}
		
		for (auto cloud: clouds_)
		{
			// delete the buffers
			glDeleteBuffers(1, &cloud.point_vbo);
			glDeleteBuffers(1, &cloud.color_vbo);
		}
		clouds_.clear();
		
		current_topic_ = str;
    	struct ps_subscriber_options options;
    	ps_subscriber_options_init(&options);
    	options.preferred_transport = 1;// tcp yo
    	ps_node_create_subscriber_adv(GetNode(), current_topic_.c_str(), &pubsub__PointCloud_def, &subscriber_, &options);
    	sub_open_ = true;
	}
	
public:

	PointCloudPlugin()
	{	
		// dont use pubsub here
		glewInit();
	}
	
	virtual ~PointCloudPlugin()
	{
		delete min_color_;
		delete max_color_;
		delete alpha_;
		delete point_size_;
		delete history_length_;
		delete point_text_;
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
		for (auto cloud: clouds_)
		{
			glDeleteBuffers(1, &cloud.point_vbo);
			glDeleteBuffers(1, &cloud.color_vbo);
		}
	}
	
	struct Point3d
	{
		float x;
		float y;
		float z;
	};
	std::vector<Point3d> point_buf_;
	std::vector<int> color_buf_;

    Gwen::Color rainbow_table_[256];
    Gwen::Color jet_table_[256];
	
	virtual void Update()
	{
		// process any messages
		// our sub has a message definition, so the queue contains real messages
		pubsub::msg::PointCloud* data;
		if (sub_open_)
		{
			while (data = (pubsub::msg::PointCloud*)ps_sub_deque(&subscriber_))
			{
				// user is responsible for freeing the message and its arrays
				if (Paused())
				{
				    free(data->data);
				    free(data);//todo use allocator free
					continue;
				}
				
				// make a new cloud with this, reusing the last ones buffers if necessary
				Cloud* cloud = 0;
				if (clouds_.size() >= history_length_->GetValue())
				{
					// pop back and push it to the front
					clouds_.push_front(clouds_.back());
					clouds_.pop_back();
					cloud = &clouds_[0];
				}
				else
				{
					Cloud c;
					glGenBuffers(1, &c.point_vbo);
					glGenBuffers(1, &c.color_vbo);
					clouds_.push_back(c);
					cloud = &clouds_[clouds_.size()-1];
				}
				
				cloud->num_points = data->num_points;
				
				point_buf_.resize(data->num_points);
				color_buf_.resize(data->num_points);
				
				uint8_t alpha = 255.0*alpha_->GetValue();
				Gwen::Color min_color = min_color_->GetValue();
				Gwen::Color max_color = max_color_->GetValue();
				
				// now make the right kind of points then render
				if (data->point_type == pubsub::msg::PointCloud::POINT_XYZ)
				{
					// x,y,z as floats
					for (int i = 0; i < data->num_points; i++)
					{
						point_buf_[i].x = ((float*)data->data)[i*3];
						point_buf_[i].y = ((float*)data->data)[i*3+1];
						point_buf_[i].z = ((float*)data->data)[i*3+2];
						color_buf_[i] = (alpha << 24) | (max_color.b << 16) | (max_color.g << 8) | max_color.r;
					}
				}
				else if (data->point_type == pubsub::msg::PointCloud::POINT_XYZI)
				{
					// x,y,z,intensity as floats
                    int color_offset = 3;// intensity, give option for other values
                    auto field = coloring_field_->GetValue();
                    if (field == "X")
                    {
                        color_offset = 0;
                    }
                    else if (field == "Y")
                    {
                        color_offset = 1;
                    }     
                    else if (field == "Z")
                    {
                        color_offset = 2;
                    }
                    else if (field == "Intensity")
                    {
                        color_offset = 3;
                    }

                    // get min and max for the field
                    float min = 100000000.0;
                    float max = -100000000.0;
                    if (auto_min_max_->GetValue())
                    {
                        for (int i = 0; i < data->num_points; i++)
					    {
					        auto value = ((float*)data->data)[i*4+color_offset];
                            min = std::min(min, value);
                            max = std::max(max, value);
                        }
                    }
                    else
                    {
                        min = min_value_->GetValue();
                        max = max_value_->GetValue();
                    }

                    float byte_scale = 255.0/(max-min);
                    float scale = 1.0/(max-min);

					if (std::abs(max - min) < 0.0001)
					{
						byte_scale = 0.0;
						scale = 0.0;
					}

                    auto mode = coloring_mode_->GetValue();
                    if (mode == "Jet" || mode == "Rainbow")
                    {
                        auto table = mode == "Jet" ? jet_table_ : rainbow_table_;
					    for (int i = 0; i < data->num_points; i++)
					    {
					    	point_buf_[i].x = ((float*)data->data)[i*4];
						    point_buf_[i].y = ((float*)data->data)[i*4+1];
					    	point_buf_[i].z = ((float*)data->data)[i*4+2];
						    int index = (((float*)data->data)[i*4+color_offset]-min)*byte_scale;
                            if (index < 0)
                                index = 0;
                            else if (index > 255)
                                index = 255;
						    uint8_t r = table[index].r;
						    uint8_t g = table[index].g;
						    uint8_t b = table[index].b;
																		
						    color_buf_[i] = (alpha << 24) | r | (g << 8) | (b << 16);
					    }
                    }
                    else if (mode == "Single Color")
                    {
                        auto color = single_color_->GetValue();
					    for (int i = 0; i < data->num_points; i++)
					    {
					    	point_buf_[i].x = ((float*)data->data)[i*4];
						    point_buf_[i].y = ((float*)data->data)[i*4+1];
					    	point_buf_[i].z = ((float*)data->data)[i*4+2];
						    uint8_t r = color.r;
						    uint8_t g = color.g;
						    uint8_t b = color.b;
																		
						    color_buf_[i] = (alpha << 24) | r | (g << 8) | (b << 16);
					    }
                    }
                    else
                    {
                        Gwen::Color lt_min_color = min_color;
                        Gwen::Color gt_max_color = max_color;
                        if (mode == "Clamped")
                        {
                            lt_min_color = floored_color_->GetValue();
                            gt_max_color = ceiled_color_->GetValue();
                        }
					    for (int i = 0; i < data->num_points; i++)
					    {
					    	point_buf_[i].x = ((float*)data->data)[i*4];
						    point_buf_[i].y = ((float*)data->data)[i*4+1];
					    	point_buf_[i].z = ((float*)data->data)[i*4+2];
						    float frac = (((float*)data->data)[i*4+color_offset]-min)*scale;
                            if (frac < 0)
                            {
                                frac = 0;
                                color_buf_[i] = (alpha << 24) | lt_min_color.r | (lt_min_color.g << 8) | (lt_min_color.b << 16);
                                continue;
                            }
                            else if (frac > 1.0)
                            {
                                frac = 1.0;
color_buf_[i] = (alpha << 24) | gt_max_color.r | (gt_max_color.g << 8) | (gt_max_color.b << 16);
                                continue;
                            }
						    uint8_t r = frac*(max_color.r - min_color.r) + min_color.r;
						    uint8_t g = frac*(max_color.g - min_color.g) + min_color.g;
						    uint8_t b = frac*(max_color.b - min_color.b) + min_color.b;
																		
						    color_buf_[i] = (alpha << 24) | r | (g << 8) | (b << 16);
					    }
                    }
				}
				
				// upload!
				glBindBuffer(GL_ARRAY_BUFFER, cloud->point_vbo);
				glBufferData(GL_ARRAY_BUFFER, point_buf_.size()*12, point_buf_.data(), GL_STATIC_DRAW);
				
				glBindBuffer(GL_ARRAY_BUFFER, cloud->color_vbo);
				glBufferData(GL_ARRAY_BUFFER, color_buf_.size()*4, color_buf_.data(), GL_STATIC_DRAW);
				
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				
				free(data->data);
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
		
		bool render_texture = point_text_->GetValue().length() > 0;
		if (render_texture)
		{
			glEnable(GL_BLEND);
			glEnable(GL_ALPHA_TEST);
			glEnable( GL_TEXTURE_2D );
			glBindTexture(GL_TEXTURE_2D, render_texture_);
			glEnable(GL_POINT_SPRITE);
			glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
		}
		
		for (auto& cloud: clouds_)
		{
			// render it
			glBindBuffer(GL_ARRAY_BUFFER, cloud.point_vbo);
			glVertexPointer(3, GL_FLOAT, 0, 0);
			glBindBuffer(GL_ARRAY_BUFFER, cloud.color_vbo);
			glColorPointer(4, GL_UNSIGNED_BYTE, 0, 0);
			
			glDrawArrays(GL_POINTS, 0, cloud.num_points);
		}
		
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		
		if (render_texture)
		{
			glDisable(GL_BLEND);
			glDisable(GL_ALPHA_TEST);
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
		
		GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
		glDrawBuffers(1, DrawBuffers);
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// add any properties
		alpha_ = AddFloatProperty(tree, "Alpha", 1.0, 0.0, 1.0, 0.1, "Point transparency.");
		
		topic_ = AddTopicProperty(tree, "Topic", "/raw_pointcloud");
		topic_->onChange = std::bind(&PointCloudPlugin::Subscribe, this, std::placeholders::_1);
		
		point_text_ = AddStringProperty(tree, "Text", "", "If set, visualize the points as the given characters.");
		point_text_->onChange = std::bind(&PointCloudPlugin::TextChange, this, std::placeholders::_1);
		TextChange(point_text_->GetValue());
		
		history_length_ = AddNumberProperty(tree, "History Length", 1, 1, 100, 1, "Number of past pointclouds to show.");
		history_length_->onChange = std::bind(&PointCloudPlugin::HistoryLengthChange, this, std::placeholders::_1);
		
		point_size_ = AddNumberProperty(tree, "Point Size", 4, 1, 100, 2, "Size in pixels for points.");

		coloring_mode_ = AddEnumProperty(tree, "Coloring Mode", "Interpolated", {"Interpolated", "Clamped", "Jet", "Rainbow", "Single Color"}, "Coloring mode.");
		coloring_mode_->onChange = std::bind(&PointCloudPlugin::ColoringModeChange, this, std::placeholders::_1);

		coloring_field_ = AddEnumProperty(tree, "Coloring Field", "Intensity", {"Intensity", "X", "Y", "Z"}, "Point field to use for coloring points.");

		floored_color_ = AddColorProperty(tree, "Floor Color", Gwen::Color(0,255,0), "Color for points below the minimum value.");
		ceiled_color_ = AddColorProperty(tree, "Ceiling Color", Gwen::Color(255,255,0), "Color for points above the minimum value.");
		
		min_color_ = AddColorProperty(tree, "Max Color", Gwen::Color(255,255,255), "Color for the minimum value.");
		max_color_ = AddColorProperty(tree, "Min Color", Gwen::Color(255,0,0), "Color for the maximum value.");

		single_color_ = AddColorProperty(tree, "Point Color", Gwen::Color(255,255,255), "Color for points.");

		auto_min_max_ = AddBooleanProperty(tree, "Auto Min/Max", true, "If true, determine the min max values from each pointcloud automatically.");
		auto_min_max_->onChange = std::bind(&PointCloudPlugin::AutoMinMaxChange, this, std::placeholders::_1);

		min_value_ = AddFloatProperty(tree, "Min Value", 0.0,   -1000000.0, 1000000.0, 1.0, "Value at which min color is used.");
		max_value_ = AddFloatProperty(tree, "Max Value", 255.0, -1000000.0, 1000000.0, 1.0, "Value at which max color is used.");


        // build color lookup tables
        for (int i = 0; i < 256; i++)
        {
            auto rainbow = Gwen::Utility::HSVToColor(i*360.0/256.0, 1.0, 1.0);
            rainbow_table_[i] = rainbow;
            jet_table_[i] = GetJetColour(i, 0, 255);
        }

        ColoringModeChange("Interpolated");
		
		Subscribe(topic_->GetValue());
	}

    // Returns interpolated color ramp values in a Jet like fashion
	Gwen::Color GetJetColour(double v,double vmin,double vmax)
	{
		Gwen::Color c(255,255,255);

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
			c.g = 255*((1.0/first_split) * (v - vmin) / dv);
		} else if (v < (vmin + middle_split * dv)) {
			c.r = 0;
			c.b = 255*(1 + (1.0/(middle_split-first_split)) * (vmin + first_split * dv - v) / dv);
		} else if (v < (vmin + last_split * dv)) {
			c.r = 255*((1.0/(last_split-middle_split)) * (v - vmin - middle_split * dv) / dv);
			c.b = 0;
		} else {
			c.g = 255*(1 + (1.0/(1.0-last_split)) * (vmin + last_split * dv - v) / dv);
			c.b = 0;
		}

		return(c);
	}
	
	std::string GetTitle() override
	{
		return "Point Cloud";
	}
};

REGISTER_PLUGIN("pointcloud", PointCloudPlugin)

#endif
