
#ifndef PUBVIZ_PLUGIN_MAP_H
#define PUBVIZ_PLUGIN_MAP_H

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

#include "../Plugin.h"
#include "../properties.h"

#include <thread>
#include <mutex>
#include <deque>
#include <map>

struct Coord
{
  int x, y, z;

  bool operator <(const Coord& o) const
  {
    return key() < o.key();
  }

  int64_t key() const
  {
	return z*100000000000000 + x + y*10000000;
  }
};


#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h> /* getprotobyname */
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

std::string http_request(std::string hostname, int server_port, std::string url) {
    char buffer[BUFSIZ];
    enum CONSTEXPR { MAX_REQUEST_LEN = 1024};
    char request[MAX_REQUEST_LEN];
    char request_template[] = "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: pubviz\r\nConnection: close\r\n\r\n";
    struct protoent *protoent;
    int request_len;
    int socket_file_descriptor;
    ssize_t nbytes_total, nbytes_last;
    struct sockaddr_in sockaddr_in;

    request_len = snprintf(request, MAX_REQUEST_LEN, request_template, url.c_str(), hostname.c_str());
    if (request_len >= MAX_REQUEST_LEN) {
        fprintf(stderr, "request length large: %d\n", request_len);
        return "";// fail
    }

    /* Build the socket. */
    protoent = getprotobyname("tcp");
    if (protoent == NULL) {
        perror("getprotobyname");
        //exit(EXIT_FAILURE);
		return "";// fail
    }

    /* Build the address in sockaddr_in
     * Possibly does a DNS query to get the IP from a hostanme. */
    {
        struct hostent *hostent = gethostbyname(hostname.c_str());
        if (hostent == NULL) {
            fprintf(stderr, "error: gethostbyname(\"%s\")\n", hostname.c_str());
            //exit(EXIT_FAILURE);
			return "";// fail
        }
        in_addr_t in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
        if (in_addr == (in_addr_t)-1) {
            fprintf(stderr, "error: inet_addr(\"%s\")\n", *(hostent->h_addr_list));
            //exit(EXIT_FAILURE);
			return "";// fail
        }
        sockaddr_in.sin_addr.s_addr = in_addr;
        sockaddr_in.sin_family = AF_INET;
        sockaddr_in.sin_port = htons(server_port);
        //fprintf(stderr, "debug: IP: %s\n", inet_ntoa(sockaddr_in.sin_addr));
    }

    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
    if (socket_file_descriptor == -1) {
        perror("socket");
        //exit(EXIT_FAILURE);
		return "";// fail
    }

    /* Actually connect. */
    if (connect(socket_file_descriptor, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) == -1) {
        perror("connect");
        //exit(EXIT_FAILURE);
		close(socket_file_descriptor);
		return "";// fail
    }

    /* Send HTTP request. */
    nbytes_total = 0;
    while (nbytes_total < request_len) {
        nbytes_last = write(socket_file_descriptor, request + nbytes_total, request_len - nbytes_total);
        if (nbytes_last == -1) {
            perror("write");
			close(socket_file_descriptor);
            //exit(EXIT_FAILURE);
			return "";// fail
        }
        nbytes_total += nbytes_last;
    }

    /* Read the response. */
    //fprintf(stderr, "debug: before first read\n");
	std::string output;
    while ((nbytes_total = read(socket_file_descriptor, buffer, BUFSIZ)) > 0) {
        //fprintf(stderr, "debug: after a read\n");
		for (int i = 0; i < nbytes_total; i++)
		{
			output += buffer[i];
		}
        //write(STDOUT_FILENO, buffer, nbytes_total);
    }
    //fprintf(stderr, "debug: after last read\n");
    if (nbytes_total == -1) {
        perror("read");
		close(socket_file_descriptor);
        //exit(EXIT_FAILURE);
		return "";// fail
    }

    close(socket_file_descriptor);

	auto start = output.find("\r\n\r\n");
	return output.substr(start+4);
}

struct Result
{
	Coord coord;
	int texture;
};

struct ImageCache
{
	std::thread request_thread;
	std::mutex map_mutex;
	std::mutex queue_mutex;
	std::deque<Coord> request_queue;
	struct CacheItem
	{
		int texture;
		pubsub::Time last_time_used;
	};
	std::map<Coord, CacheItem> textures;// todo limit size

	struct Incoming
	{
		Coord coord;
		char* data;
		int width, height;
		GLenum format;
	};
	std::deque<Incoming> texture_queue;

	bool run_thread_ = true;
	void StartThread()
	{
		request_thread = std::thread([&](){
			// create cache directory if it doesnt exist
			mkdir(".tile_cache", 0700);
			while (ps_okay() && run_thread_)
			{
				queue_mutex.unlock();
				if (request_queue.size() == 0)
				{
					queue_mutex.unlock();
					ps_sleep(10);
					continue;
				}

				auto coord = request_queue.front();
				request_queue.pop_front();
				queue_mutex.unlock();

				// Load whatever we got from the filesystem first
				if (LoadClosestCached(coord))
				{
					//printf("Not loading from server since cache hit.\n");
					continue;
				}

				// then load from the server
				char url[500];
				sprintf(url, "/%i/%i/%i.png", coord.z, coord.x, coord.y);
				std::string res = http_request("tile.openstreetmap.org", 80, url);
				//printf("Got %i bytes\n", res.size());
				if (res.size() == 0)
				{
					printf("Request failed.\n");
					continue;
				}

				auto i = LoadPNG(res.data(), res.size());
				i.coord = coord;
				queue_mutex.lock();
				texture_queue.push_back(i);
				queue_mutex.unlock();

				// save to cache
				char buf[500];
				sprintf(buf, ".tile_cache/%i_%i_%i.png", coord.z, coord.x, coord.y);
				FILE* f = fopen(buf, "wb");
				fwrite(res.data(), res.size(), 1, f);
				fclose(f);
			}
		});
	}

	~ImageCache()
	{
		for (auto& i: textures)
		{
			if (i.second.texture >= 0)
			{
				GLuint tex = i.second.texture;
				glDeleteTextures(1, &tex);
			}
		}
		run_thread_ = false;
		if (request_thread.joinable())
		{
			request_thread.join();
		}
	}

	// returns a coord and texture
	Result GetTile(int x, int y, int zoom)
	{
		Coord coord = {x,y,zoom};
		map_mutex.lock();

		// request the tile and insert if we havent already
		if (textures.find(coord) == textures.end())
		{
			// request this texture and mark it as requested
			CacheItem item;
			item.texture = -1;
			item.last_time_used = pubsub::Time(0);// todo 
			textures[coord] = item;

			queue_mutex.lock();
			request_queue.push_front(coord);
			queue_mutex.unlock();
		}

		// try and find a close enough texture
		Coord ncoord = coord;
		int range = 4;
		for (int z = zoom; z >= std::max(zoom-range, 0); z--)
		{
			auto r = textures.find(ncoord);
			if (r != textures.end() && r->second.texture >= 0)
			{
				map_mutex.unlock();
				return {ncoord, r->second.texture};
			}
			ncoord.x /= 2;
			ncoord.y /= 2;
			ncoord.z -= 1;
		}
		map_mutex.unlock();
		return {coord, -1};
	}

	bool Process()
	{
		// turn any queued memories into textures and add them
		bool loaded = false;
		while (true)
		{
			queue_mutex.lock();
			if (texture_queue.size() == 0)
			{
				queue_mutex.unlock();
				return loaded;
			}
			auto item = texture_queue.front();
			texture_queue.pop_front();
			queue_mutex.unlock();

			// create the texture
			int texture = CreateTexture(item.data, 256, 256, item.format);

			//printf("Created texture for %i %i %i\n", item.coord.z, item.coord.x, item.coord.y);
			map_mutex.lock();
			textures[item.coord] = {texture, pubsub::Time(0)};
			map_mutex.unlock();
			delete[] item.data;
			loaded = true;
		}
		return loaded;// should never get executed
	}

private:
	// returns true if we had the requested tile cached
	bool LoadClosestCached(Coord c)
	{
		// fill in the texture for the closest one to requested if it isnt already
		auto oc = c;
		int range = 4;// try down this many levels
		for (int z = oc.z; z >= std::max(oc.z-range, 0); z--)
		{
			map_mutex.lock();
			auto r = textures.find(c);
			Incoming tex;
			tex.data = 0;
			if (r == textures.end() || r->second.texture < 0)
			{
				map_mutex.unlock();
				tex = TryCached(c.x, c.y, z);
			}
			else
			{
				map_mutex.unlock();
			}
			if (tex.data != 0)
			{
				// add to queue! we got something close enough
				queue_mutex.lock();
				texture_queue.push_back(tex);
				queue_mutex.unlock();
				return oc.z == c.z;
			}
			//printf("http://tile.openstreetmap.org/%i/%i/%i.png missing from cache\n", c.z, c.x, c.y);
			c.x /= 2;
			c.y /= 2;
			c.z -= 1;
		}
		//printf("No usable cached tile found for %i %i %i\n", oc.z, oc.x, oc.y);
		return false;
	}

	Incoming TryCached(int x, int y, int z)
	{
		char buf[500];
		sprintf(buf, ".tile_cache/%i_%i_%i.png", z, x, y);
		FILE* f = fopen(buf, "rb");
		if (f)
		{
			//printf("Loaded %i %i %i from file.\n", z, x, y);
			fseek(f, 0, SEEK_END);
        	auto fsize = ftell(f);
        	rewind(f);

        	auto fcontent = (char*) malloc(sizeof(char) * fsize);
        	fread(fcontent, 1, fsize, f);

			auto i = LoadPNG(fcontent, fsize);
			i.coord.x = x;
			i.coord.y = y;
			i.coord.z = z;
			free(fcontent);
			fclose(f);

			return i;
		}
		else
		{
			return {{0,0,0}, 0, 0};
		}
	}

	static int CreateTexture(char* data, int width, int height, GLenum texture_format)
	{
		GLuint texture = 0;
		glGenTextures(1, &texture);
		//printf("Texture: %i\n", texture);
		
		glBindTexture(GL_TEXTURE_2D, texture);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

		glTexImage2D(
		  GL_TEXTURE_2D,
		  0,
		  GL_RGBA,
		  width,
		  height,
		  0,
		  texture_format,
		  GL_UNSIGNED_BYTE,
		  data);

		glBindTexture(GL_TEXTURE_2D, 0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		return texture;
	}

	Incoming LoadPNG(char* data, int size)
	{
		Incoming i;
		i.width = 256;
		i.height = 256;
		i.data = new char[i.width*i.height*4];
#ifdef FREEIMAGE_BIGENDIAN
		i.format = GL_RGBA;
#else
		i.format = GL_BGRA;
#endif
		// todo get width from file rather than assume
		FIMEMORY* mem = FreeImage_OpenMemory((unsigned char*)data, size);
		FIBITMAP* img = FreeImage_LoadFromMemory(FIF_PNG, mem);
		FIBITMAP* bits32 = FreeImage_ConvertTo32Bits( img );
		FreeImage_FlipVertical( bits32 );
		// copy
		memcpy(i.data, FreeImage_GetBits( bits32 ), 4*i.height*i.width);
		FreeImage_Unload( bits32 );
		FreeImage_Unload( img );
		FreeImage_CloseMemory(mem);

		return i;
	}
};



class MapPlugin: public pubviz::Plugin
{
	std::unique_ptr<FloatProperty> alpha_;
	std::unique_ptr<NumberProperty> max_zoom_, default_zoom_, default_tiles_;
	std::unique_ptr<BooleanProperty> show_outline_;
	
	ImageCache cache_;
	
public:

	MapPlugin()
	{	
		// dont use pubsub here
		cache_.StartThread();
	}
	
	virtual ~MapPlugin()
	{

	}

	// Clear out any historical data so the view gets cleared
	virtual void Clear()
	{
	
	}
		
	virtual void Update()
	{
		// redraw if we loaded a texture
		if (cache_.Process())
		{
			Redraw();
		}
	}

	static void get_tile_at(double lat, double lon, int zoom, int&x, int&y)
	{
		double lat_rad = lat*M_PI/180.0;
		double n = pow(2.0, zoom);
		x = int((lon + 180.0)/360.0*n);
		y = int((1.0 - log(tan(lat_rad) + (1/cos(lat_rad)))/M_PI)/2.0*n);
	}

	static void get_tile_ll(int x, int y, int zoom, double&lat, double&lon)
	{
		double n = pow(2.0, zoom);
		lon = x/n*360.0 - 180.0;
		double lat_rad = atan(sinh(M_PI*(1.0-2.0*y/n)));
		lat = lat_rad*180.0/M_PI;
	}

	double pixels_per_meter(double lat, int zoom)
	{
		double tile_width = 40075016.686*cos(lat*M_PI/180.0)/pow(2.0, zoom);
		return 256.0/tile_width;
	}

	virtual void Paint()
	{
		// Draw license attribution
		auto canvas = GetCanvas();
		auto font = canvas->GetSkin()->GetDefaultFont();

		auto r = canvas->GetSkin()->GetRender();
		
		std::string string = "OpenStreetMap";
		auto res = r->MeasureText(font, string);
		r->SetDrawColor( Gwen::Color(255,255,255,255) );
		r->DrawFilledRect(Gwen::Rect(canvas->Width()-res.x, canvas->Height()-res.y, res.x, res.y));
		r->SetDrawColor( Gwen::Color(0,0,0,255) );
		r->RenderText(font, Gwen::PointF(canvas->Width()-res.x, canvas->Height()-res.y ), string);
	}
	
	virtual void Render()
	{
		auto canvas = GetCanvas();
		if (!canvas->local_xy_.Initialized())
		{
			return;// we need an origin
		}

		int zoom = default_zoom_->GetValue();
		const int max_zoom = max_zoom_->GetValue();
		zoom = std::min(max_zoom, zoom);

		bool show_outline = show_outline_->GetValue();

		double ax, ay, az;
		canvas->GetViewCenter(ax, ay, az);

		auto xy = &canvas->local_xy_;
		int xmin, xmax, ymin, ymax;
		if (canvas->GetViewType() == ViewType::TopDown)
		{
			// okay, lets get each corner of the view in current frame coordinates
			double w2 = canvas->view_width()/2.0;
			double h2 = canvas->view_height()/2.0;
			Vec3d corners[4];
			corners[0] = Vec3d(ax-w2, ay-h2, 0);
			corners[1] = Vec3d(ax-w2, ay+h2, 0);
			corners[2] = Vec3d(ax+w2, ay+h2, 0);
			corners[3] = Vec3d(ax+w2, ay-h2, 0);

			// then loop through and get wgs84 min and max for each corner
			double minlat, minlon;
			double maxlat, maxlon;
			for (int i = 0; i < 4; i++)
			{
				// transform to map if necessary
				auto src_frame = canvas->wgs84_mode() ? OpenGLCanvas::Map : OpenGLCanvas::Odom;
				canvas->TransformToFrame(src_frame, OpenGLCanvas::WGS84, corners[i]);

				double lat = corners[i].x;
				double lon = corners[i].y;
				if (i == 0)
				{
					minlat = maxlat = lat;
					minlon = maxlon = lon;
				}
				else
				{
					minlon = std::min(minlon, lon);
					minlat = std::min(minlat, lat);
					maxlon = std::max(maxlon, lon);
					maxlat = std::max(maxlat, lat);
				}
			}
			
			// determine best zoom level to use
			// find first higher resolution tile
			double view_ppm = canvas->Width()*canvas->GetCanvas()->Scale()/canvas->view_width();
			zoom = max_zoom;
			for (int i = 1; i <= max_zoom; i++)
			{
				double zoom_ppm = pixels_per_meter(minlat, i);
				//printf("ppm %i %f\n", i, zoom_ppm);
				if (zoom_ppm > view_ppm)
				{
					zoom = std::max(1, i-1);// todo, maybe dont do this, but it makes things faster
					break;
				}
			}
			
			double zoom_ppm = pixels_per_meter(minlat, zoom);

			// Finally convert to tile coords
			get_tile_at(minlat, minlon, zoom, xmin, ymin);
			get_tile_at(maxlat, maxlon, zoom, xmax, ymax);
	
			//printf("Lat: %f to %f\nLon: %f to %f\n", minlat, maxlat, minlon, maxlon);
			//printf("Zoom: %i TilePPM: %f ViewPPM: %f\n", zoom, zoom_ppm, view_ppm);
		}
		else
		{
			//set to center + default_zoom with default_tiles on each side
			zoom = default_zoom_->GetValue();
			
			double lat, lon;
			xy->ToLatLon(ax, ay, lat, lon);

			int x,y;
			get_tile_at(lat, lon, zoom, x, y);

			int tiles = default_tiles_->GetValue();
			xmin = std::max(0, x - tiles);
			ymin = std::max(0, y - tiles);
			int maxn = pow(2, zoom) - 1;
			xmax = std::min(maxn, x + tiles);
			ymax = std::min(maxn, y + tiles);

			// to do this i would need to intersect frustum with plane to get a shape
			// then would need to estimate a zoom and intersecting chunks
		}

        if (xmax < xmin) std::swap(xmin, xmax);
		if (ymax < ymin) std::swap(ymin, ymax);
		
        //printf("X: %i to %i\nY: %i to %i\n", xmin, xmax, ymin, ymax);

		// todo this probably craps itself at map edge
		// First get a list of all tiles to render in the area
		std::map<Coord, int> to_render;// todo try something with fewer allocations?
    	for (int x = xmin; x <= xmax; x++)
		{
			for (int y = ymin; y <= ymax; y++)
			{
				// todo need to wrap coords around edges properly
				//printf("Tile: http://tile.openstreetmap.org/%i/%i/%i.png\n", zoom, x, y);

				auto tile = cache_.GetTile(x, y, zoom);
				if (tile.texture >= 0)
				{
					to_render[tile.coord] = tile.texture;
				}

				if (show_outline)
				{
					glLineWidth(4.0f);
					glBegin(GL_LINE_STRIP);

					glColor3f(1.0, 1.0, 1.0);

					double lat, lon;
					Vec3d p;
					get_tile_ll(x, y, zoom, lat, lon);
					p = Vec3d(lat, lon, 0);
					canvas->TransformToView(OpenGLCanvas::WGS84, p);
					glVertex2f(p.x, p.y);
					get_tile_ll(x+1, y, zoom, lat, lon);
					p = Vec3d(lat, lon, 0);
					canvas->TransformToView(OpenGLCanvas::WGS84, p);
					glVertex2f(p.x, p.y);
					get_tile_ll(x+1, y+1, zoom, lat, lon);
					p = Vec3d(lat, lon, 0);
					canvas->TransformToView(OpenGLCanvas::WGS84, p);
					glVertex2f(p.x, p.y);
					get_tile_ll(x, y+1, zoom, lat, lon);
					p = Vec3d(lat, lon, 0);
					canvas->TransformToView(OpenGLCanvas::WGS84, p);
					glVertex2f(p.x, p.y);
					get_tile_ll(x, y, zoom, lat, lon);
					p = Vec3d(lat, lon, 0);
					canvas->TransformToView(OpenGLCanvas::WGS84, p);
					glVertex2f(p.x, p.y);
			
					glEnd();
				}
			}
		}

		// this is kinda silly, but works well
		// Then sort the tiles by zoom level and render
		for (auto tile: to_render)
		{
			int x = tile.first.x;
			int y = tile.first.y;
			int zoom = tile.first.z;
			int texture = tile.second;
			double lat, lon;
			double px, py;
			glEnable(GL_BLEND);

			//printf("RTexture: %i\n", texture);
		
			// Now draw the costmap itself
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, texture);
			glBegin(GL_TRIANGLES);

			glColor4f(1.0f, 1.0f, 1.0f, alpha_->GetValue() );

			Vec3d p;
			glTexCoord2d(0, 0);
			get_tile_ll(x, y, zoom, lat, lon);
			p = Vec3d(lat, lon, 0);
			canvas->TransformToView(OpenGLCanvas::WGS84, p);
			glVertex2f(p.x, p.y);
			glTexCoord2d(1.0, 0);
			get_tile_ll(x+1, y, zoom, lat, lon);
			p = Vec3d(lat, lon, 0);
			canvas->TransformToView(OpenGLCanvas::WGS84, p);
			glVertex2f(p.x, p.y);
			glTexCoord2d(1.0, 1.0);
			get_tile_ll(x+1, y+1, zoom, lat, lon);
			p = Vec3d(lat, lon, 0);
			canvas->TransformToView(OpenGLCanvas::WGS84, p);
			glVertex2f(p.x, p.y);

			glTexCoord2d(0, 0);
			get_tile_ll(x, y, zoom, lat, lon);
			p = Vec3d(lat, lon, 0);
			canvas->TransformToView(OpenGLCanvas::WGS84, p);
			glVertex2f(p.x, p.y);
			glTexCoord2d(1.0, 1.0);
			get_tile_ll(x+1, y+1, zoom, lat, lon);
			p = Vec3d(lat, lon, 0);
			canvas->TransformToView(OpenGLCanvas::WGS84, p);
			glVertex2f(p.x, p.y);
			glTexCoord2d(0, 1.0);
			get_tile_ll(x, y+1, zoom, lat, lon);
			p = Vec3d(lat, lon, 0);
			canvas->TransformToView(OpenGLCanvas::WGS84, p);
			glVertex2f(p.x, p.y);

			glEnd();

			glBindTexture(GL_TEXTURE_2D, 0);
			glDisable(GL_TEXTURE_2D);

			glDisable(GL_BLEND);

			if (show_outline)
			{
				glLineWidth(4.0f);
				glBegin(GL_LINE_STRIP);
				
				glColor3f(1.0, 0.0, 0.0);

				double lat, lon;
				get_tile_ll(x, y, zoom, lat, lon);
				p = Vec3d(lat, lon, 0);
				canvas->TransformToView(OpenGLCanvas::WGS84, p);
				glVertex2f(p.x, p.y);
				get_tile_ll(x+1, y, zoom, lat, lon);
				p = Vec3d(lat, lon, 0);
				canvas->TransformToView(OpenGLCanvas::WGS84, p);
				glVertex2f(p.x, p.y);
				get_tile_ll(x+1, y+1, zoom, lat, lon);
				p = Vec3d(lat, lon, 0);
				canvas->TransformToView(OpenGLCanvas::WGS84, p);
				glVertex2f(p.x, p.y);
				get_tile_ll(x, y+1, zoom, lat, lon);
				p = Vec3d(lat, lon, 0);
				canvas->TransformToView(OpenGLCanvas::WGS84, p);
				glVertex2f(p.x, p.y);
				get_tile_ll(x, y, zoom, lat, lon);
				p = Vec3d(lat, lon, 0);
				canvas->TransformToView(OpenGLCanvas::WGS84, p);
				glVertex2f(p.x, p.y);
			
				glEnd();
			}
		}
	}
	
	virtual void Initialize(Gwen::Controls::Properties* tree)
	{
		// add any properties
		alpha_.reset(AddFloatProperty(tree, "Alpha", 1.0, 0.0, 1.0, 0.1, "Tile transparency."));
		
		show_outline_.reset(AddBooleanProperty(tree, "Show Outline", false, "If true, draw outlines around the tiles."));
		
		max_zoom_.reset(AddNumberProperty(tree, "Max Zoom", 19, 1, 22, 1, "Max zoom level to download/show."));

		default_zoom_.reset(AddNumberProperty(tree, "3D Zoom Level", 17, 1, 22, 1, "Zoom level to use in views other than top down."));

		default_tiles_.reset(AddNumberProperty(tree, "3D Tiles", 2, 0, 5, 1, "Number of tiles to show around center tile in views other than top down."));
	}
	
	std::string GetTitle() override
	{
		return "Map";
	}
};

REGISTER_PLUGIN("map", MapPlugin)

#endif
