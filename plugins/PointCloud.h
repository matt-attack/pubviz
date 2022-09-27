
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



class PointCloudPlugin: public Plugin
{
	FloatProperty* alpha_;
	StringProperty* point_text_;
	NumberProperty* point_size_;
	NumberProperty* history_length_;

    // Coloring Properties
    EnumProperty* coloring_mode_;
    
    // Used in interpolated mode and clamped mode
	ColorProperty* min_color_;
	ColorProperty* max_color_;

    // Used in clamped mode
    ColorProperty* floored_color_;
    ColorProperty* ceiled_color_;
	
	TopicProperty* topic_;
	
	bool sub_open_ = false;
	ps_sub_t subscriber_;
	
	pubsub::msg::Marker last_msg_;
	
	struct Cloud
	{
		int num_points;
		unsigned int point_vbo;
		unsigned int color_vbo;
	};
	
	std::deque<Cloud> clouds_;
	
	std::map<int, pubsub::msg::Marker> markers_;
	
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
        // show the desired properties
        if (mode == "Interpolated")
        {
            min_color_->Show();
            max_color_->Show();
        }
        else if (mode == "Clamped")
        {
            min_color_->Show();
            max_color_->Show();
            ceiled_color_->Show();
            floored_color_->Show();
        }
        else if (mode == "Jet")
        {
            
        }
        else if (mode == "Rainbow")
        {
            
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
		
		markers_.clear();
		
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

    const unsigned char JET_COLOR_MAP[768] =
    { 150, 150, 150,
      107, 0, 12,
      106, 0, 18,
      105, 0, 24,
      103, 0, 30,
      102, 0, 36,
      101, 0, 42,
      99, 0, 48,
      98, 0, 54,
      97, 0, 60,
      96, 0, 66,
      94, 0, 72,
      93, 0, 78,
      92, 0, 84,
      91, 0, 90,
      89, 0, 96,
      88, 0, 102,
      87, 0, 108,
      85, 0, 114,
      84, 0, 120,
      83, 0, 126,
      82, 0, 131,
      80, 0, 137,
      79, 0, 143,
      78, 0, 149,
      77, 0, 155,
      75, 0, 161,
      74, 0, 167,
      73, 0, 173,
      71, 0, 179,
      70, 0, 185,
      69, 0, 191,
      68, 0, 197,
      66, 0, 203,
      65, 0, 209,
      64, 0, 215,
      62, 0, 221,
      61, 0, 227,
      60, 0, 233,
      59, 0, 239,
      57, 0, 245,
      56, 0, 251,
      55, 0, 255,
      54, 0, 255,
      52, 0, 255,
      51, 0, 255,
      50, 0, 255,
      48, 0, 255,
      47, 0, 255,
      46, 0, 255,
      45, 0, 255,
      43, 0, 255,
      42, 0, 255,
      41, 0, 255,
      40, 0, 255,
      38, 0, 255,
      37, 0, 255,
      36, 0, 255,
      34, 0, 255,
      33, 0, 255,
      32, 0, 255,
      31, 0, 255,
      29, 0, 255,
      28, 0, 255,
      27, 0, 255,
      26, 0, 255,
      24, 0, 255,
      23, 0, 255,
      22, 0, 255,
      20, 0, 255,
      19, 0, 255,
      18, 0, 255,
      17, 0, 255,
      15, 0, 255,
      14, 0, 255,
      13, 0, 255,
      11, 0, 255,
      10, 0, 255,
      9, 0, 255,
      8, 0, 255,
      6, 0, 255,
      5, 0, 255,
      4, 0, 255,
      3, 0, 255,
      1, 0, 255,
      0, 4, 255,
      0, 10, 255,
      0, 16, 255,
      0, 22, 255,
      0, 28, 255,
      0, 34, 255,
      0, 40, 255,
      0, 46, 255,
      0, 52, 255,
      0, 58, 255,
      0, 64, 255,
      0, 70, 255,
      0, 76, 255,
      0, 82, 255,
      0, 88, 255,
      0, 94, 255,
      0, 100, 255,
      0, 106, 255,
      0, 112, 255,
      0, 118, 255,
      0, 124, 255,
      0, 129, 255,
      0, 135, 255,
      0, 141, 255,
      0, 147, 255,
      0, 153, 255,
      0, 159, 255,
      0, 165, 255,
      0, 171, 255,
      0, 177, 255,
      0, 183, 255,
      0, 189, 255,
      0, 195, 255,
      0, 201, 255,
      0, 207, 255,
      0, 213, 255,
      0, 219, 255,
      0, 225, 255,
      0, 231, 255,
      0, 237, 255,
      0, 243, 255,
      0, 249, 255,
      0, 255, 255,
      0, 255, 249,
      0, 255, 243,
      0, 255, 237,
      0, 255, 231,
      0, 255, 225,
      0, 255, 219,
      0, 255, 213,
      0, 255, 207,
      0, 255, 201,
      0, 255, 195,
      0, 255, 189,
      0, 255, 183,
      0, 255, 177,
      0, 255, 171,
      0, 255, 165,
      0, 255, 159,
      0, 255, 153,
      0, 255, 147,
      0, 255, 141,
      0, 255, 135,
      0, 255, 129,
      0, 255, 124,
      0, 255, 118,
      0, 255, 112,
      0, 255, 106,
      0, 255, 100,
      0, 255, 94,
      0, 255, 88,
      0, 255, 82,
      0, 255, 76,
      0, 255, 70,
      0, 255, 64,
      0, 255, 58,
      0, 255, 52,
      0, 255, 46,
      0, 255, 40,
      0, 255, 34,
      0, 255, 28,
      0, 255, 22,
      0, 255, 16,
      0, 255, 10,
      0, 255, 4,
      2, 255, 0,
      8, 255, 0,
      14, 255, 0,
      20, 255, 0,
      26, 255, 0,
      32, 255, 0,
      38, 255, 0,
      44, 255, 0,
      50, 255, 0,
      56, 255, 0,
      62, 255, 0,
      68, 255, 0,
      74, 255, 0,
      80, 255, 0,
      86, 255, 0,
      92, 255, 0,
      98, 255, 0,
      104, 255, 0,
      110, 255, 0,
      116, 255, 0,
      122, 255, 0,
      128, 255, 0,
      133, 255, 0,
      139, 255, 0,
      145, 255, 0,
      151, 255, 0,
      157, 255, 0,
      163, 255, 0,
      169, 255, 0,
      175, 255, 0,
      181, 255, 0,
      187, 255, 0,
      193, 255, 0,
      199, 255, 0,
      205, 255, 0,
      211, 255, 0,
      217, 255, 0,
      223, 255, 0,
      229, 255, 0,
      235, 255, 0,
      241, 255, 0,
      247, 255, 0,
      253, 255, 0,
      255, 251, 0,
      255, 245, 0,
      255, 239, 0,
      255, 233, 0,
      255, 227, 0,
      255, 221, 0,
      255, 215, 0,
      255, 209, 0,
      255, 203, 0,
      255, 197, 0,
      255, 191, 0,
      255, 185, 0,
      255, 179, 0,
      255, 173, 0,
      255, 167, 0,
      255, 161, 0,
      255, 155, 0,
      255, 149, 0,
      255, 143, 0,
      255, 137, 0,
      255, 131, 0,
      255, 126, 0,
      255, 120, 0,
      255, 114, 0,
      255, 108, 0,
      255, 102, 0,
      255, 96, 0,
      255, 90, 0,
      255, 84, 0,
      255, 78, 0,
      255, 72, 0,
      255, 66, 0,
      255, 60, 0,
      255, 54, 0,
      255, 48, 0,
      255, 42, 0,
      255, 36, 0,
      255, 30, 0,
      255, 24, 0,
      255, 18, 0,
      255, 12, 0,
      255,  6, 0,
      255,  0, 0,
    };
	
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
                    if (coloring_mode_->GetValue() == "Jet")
                    {
					    for (int i = 0; i < data->num_points; i++)
					    {
					    	point_buf_[i].x = ((float*)data->data)[i*4];
						    point_buf_[i].y = ((float*)data->data)[i*4+1];
					    	point_buf_[i].z = ((float*)data->data)[i*4+2];
						    int index = ((float*)data->data)[i*4+3]*255.0;
						    uint8_t r = jet_table_[index].r;
						    uint8_t g = jet_table_[index].g;
						    uint8_t b = jet_table_[index].b;
																		
						    color_buf_[i] = (alpha << 24) | r | (g << 8) | (b << 16);
					    }
                    }
                    else if (coloring_mode_->GetValue() == "Rainbow")
                    {
					    for (int i = 0; i < data->num_points; i++)
					    {
					    	point_buf_[i].x = ((float*)data->data)[i*4];
						    point_buf_[i].y = ((float*)data->data)[i*4+1];
					    	point_buf_[i].z = ((float*)data->data)[i*4+2];
						    int index = ((float*)data->data)[i*4+3]*255.0;
						    uint8_t r = rainbow_table_[index].r;
						    uint8_t g = rainbow_table_[index].g;
						    uint8_t b = rainbow_table_[index].b;
																		
						    color_buf_[i] = (alpha << 24) | r | (g << 8) | (b << 16);
					    }
                    }
                    else
                    {
					    for (int i = 0; i < data->num_points; i++)
					    {
					    	point_buf_[i].x = ((float*)data->data)[i*4];
						    point_buf_[i].y = ((float*)data->data)[i*4+1];
					    	point_buf_[i].z = ((float*)data->data)[i*4+2];
						    float frac = ((float*)data->data)[i*4+3];
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
		alpha_ = AddFloatProperty(tree, "Alpha", 1.0, 0.0, 1.0, 0.1);
		
		topic_ = AddTopicProperty(tree, "Topic", "/pointcloud");
		topic_->onChange = std::bind(&PointCloudPlugin::Subscribe, this, std::placeholders::_1);
		
		point_text_ = AddStringProperty(tree, "Text", "");
		point_text_->onChange = std::bind(&PointCloudPlugin::TextChange, this, std::placeholders::_1);
		TextChange(point_text_->GetValue());
		
		history_length_ = AddNumberProperty(tree, "History Length", 1, 1, 100, 1);
		history_length_->onChange = std::bind(&PointCloudPlugin::HistoryLengthChange, this, std::placeholders::_1);
		
		point_size_ = AddNumberProperty(tree, "Point Size", 4, 1, 100, 2);

        coloring_mode_ = AddEnumProperty(tree, "Coloring Mode", "Interpolated", {"Interpolated", "Clamped", "Jet", "Rainbow"});
		coloring_mode_->onChange = std::bind(&PointCloudPlugin::ColoringModeChange, this, std::placeholders::_1);

		floored_color_ = AddColorProperty(tree, "Floor Color", Gwen::Color(0,255,0));
		ceiled_color_ = AddColorProperty(tree, "Ceiling Color", Gwen::Color(255,255,0));
		
		min_color_ = AddColorProperty(tree, "Max Color", Gwen::Color(255,255,255));
		max_color_ = AddColorProperty(tree, "Min Color", Gwen::Color(255,0,0));

        // build color lookup tables
        for (int i = 0; i < 256; i++)
        {
            auto rainbow = Gwen::Utility::HSVToColor(i*360.0/256.0, 1.0, 1.0);
            rainbow_table_[i] = rainbow;
            jet_table_[i] = Gwen::Color(JET_COLOR_MAP[i*3], JET_COLOR_MAP[i*3 + 1], JET_COLOR_MAP[i*3 + 2]);
        }

        ColoringModeChange("Interpolated");
		
		Subscribe(topic_->GetValue());
	}
	
	std::string GetTitle() override
	{
		return "Point Cloud";
	}
};

#endif
