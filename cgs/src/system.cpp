#include "cgs/system.hpp"
#include "cgs/log.hpp"
#include "FreeImage.h"
#include "glfw3.h"

#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <map>

namespace cgs
{
    namespace
    {
        //---------------------------------------------------------------------------------------------
        // Internal declarations
        //---------------------------------------------------------------------------------------------
        typedef std::map<int, event_type>            key_event_type_map;
        typedef key_event_type_map::iterator         event_type_it;
        typedef std::map<int, std::string>           error_code_map;

        struct window
        {
            window() :
                m_glfw_window(nullptr),
                m_events(),
                m_last_mouse_x(0.0f),
                m_last_mouse_y(0.0f),
                m_used(true) {}

            GLFWwindow*        m_glfw_window;
            std::vector<event> m_events;
            float              m_last_mouse_x;
            float              m_last_mouse_y;
            bool               m_used;
        };

        //---------------------------------------------------------------------------------------------
        // Internal data structures
        //---------------------------------------------------------------------------------------------
        key_event_type_map       event_types;                  // map from GLFW key action value to event_type value
        error_code_map           error_codes;                  // map from GLFW error codes to their names
        std::vector<window>      windows;                      // collection of all windows created
        bool                     is_glfw_initialized = false;  // has GLFW been initialized?

        //---------------------------------------------------------------------------------------------
        // Helper functions
        //---------------------------------------------------------------------------------------------
        void fill_error_code_map()
        {
            if (error_codes.empty()) {
                error_codes[GLFW_NOT_INITIALIZED]     = "GLFW_NOT_INITIALIZED";
                error_codes[GLFW_NO_CURRENT_CONTEXT]  = "GLFW_NO_CURRENT_CONTEXT";
                error_codes[GLFW_INVALID_ENUM]        = "GLFW_INVALID_ENUM";
                error_codes[GLFW_INVALID_VALUE]       = "GLFW_INVALID_VALUE";
                error_codes[GLFW_OUT_OF_MEMORY]       = "GLFW_OUT_OF_MEMORY";
                error_codes[GLFW_API_UNAVAILABLE]     = "GLFW_API_UNAVAILABLE";
                error_codes[GLFW_VERSION_UNAVAILABLE] = "GLFW_VERSION_UNAVAILABLE";
                error_codes[GLFW_PLATFORM_ERROR]      = "GLFW_PLATFORM_ERROR";
                error_codes[GLFW_FORMAT_UNAVAILABLE]  = "GLFW_FORMAT_UNAVAILABLE";
                //error_codes[GLFW_NO_WINDOW_CONTEXT]   = "GLFW_NO_WINDOW_CONTEXT"; Not defined in our version of GLFW
            }
        }

        void fill_event_type_map() {
            if (event_types.empty()) {
                event_types[GLFW_PRESS]   = EVENT_KEY_PRESS;
                event_types[GLFW_REPEAT]  = EVENT_KEY_HOLD;
                event_types[GLFW_RELEASE] = EVENT_KEY_RELEASE;
            }
        }

        void key_callback(GLFWwindow* glfw_window, int key, int, int action, int)
        {
            if (!is_glfw_initialized) return;

            auto it = std::find_if(windows.begin(), windows.end(), [glfw_window](const window& m) {
                return m.m_used && m.m_glfw_window == glfw_window;
            });

            if (it == windows.end()) return;

            // Our key code is the same value that GLFW gives us. This works because we have defined our
            // keycode constants with the same values as GLFW's constants. If we update GLFW and its
            // key constants change, we need to update our keycode constants accordingly.
            key_code key_code = key;

            // Translate GLFW key action value to an event type
            fill_event_type_map();
            event_type event_type = EVENT_KEY_PRESS;
            event_type_it eit = event_types.find(action);
            if (eit != event_types.end()) {
                event_type = eit->second;
                event e = {event_type, key_code, 0.0f, 0.0f, 0.0f, 0.0f};
                it->m_events.push_back(e);
            }
        }

        void mouse_move_callback(GLFWwindow* glfw_window, double x, double y)
        {
            if (!is_glfw_initialized) return;

            auto it = std::find_if(windows.begin(), windows.end(), [glfw_window](const window& m) {
                return m.m_used && m.m_glfw_window == glfw_window;
            });

            if (it == windows.end()) return;

            event e{EVENT_MOUSE_MOVE, KEY_UNKNOWN, (float) x, (float) y, (float) x - it->m_last_mouse_x, (float) y - it->   m_last_mouse_y};
            it->m_events.push_back(e);
            it->m_last_mouse_x = x;
            it->m_last_mouse_y = y;
        }

        void error_callback(int error_code, const char* description)
        {
            fill_error_code_map();
            log(LOG_LEVEL_ERROR, "GLFW error callback");
            std::ostringstream oss;
            oss << "GLFW error " << error_codes[error_code] << ", " << description;
            log(LOG_LEVEL_ERROR, oss.str());
        }
    } // anonymous namespace

    //-----------------------------------------------------------------------------------------------
    // Public functions
    //-----------------------------------------------------------------------------------------------
    class image::image_impl
    {   
    public:
        image_impl() :
            m_img(nullptr) {}

        void log_image_properties()
        {
            auto width = FreeImage_GetWidth(m_img);
            auto height = FreeImage_GetHeight(m_img);
            auto depth = FreeImage_GetBPP(m_img) / 8;
            auto red_mask = FreeImage_GetRedMask(m_img);
            auto green_mask = FreeImage_GetGreenMask(m_img);
            auto blue_mask = FreeImage_GetBlueMask(m_img);
            std::ostringstream oss;
            oss << "loaded image of (width x height) = " << width << " x " << height << " pixels"
                << ", bytes in total: " << depth * width * height
                << ", bytes by pixel: " << depth
                << ", red mask: " << std::hex << red_mask
                << ", green mask: " << std::hex << green_mask
                << ", blue mask: " << std::hex << blue_mask;
            log(LOG_LEVEL_DEBUG, oss.str());
        }

        FIBITMAP* m_img;
    };

    image::image() :
        m_impl(std::make_unique<image_impl>()) {}

    image::~image() {
        if (m_impl->m_img != nullptr) {
            FreeImage_Unload(m_impl->m_img);
        }
    }

    void image::load(const std::string& path)
    {
        if (m_impl->m_img == nullptr) {
            FREE_IMAGE_FORMAT format = FreeImage_GetFileType(path.c_str(), 0); // automatically detects the format(from over 20 formats!)
            m_impl->m_img = FreeImage_Load(format, path.c_str());
            if (m_impl->m_img == nullptr) {
                log(LOG_LEVEL_ERROR, std::string("could not find image in path ") + path);
                throw std::runtime_error("");
            }
            m_impl->log_image_properties();
        }
    }

    void image::flip_vertical()
    {
        // In FreeImage, images are stored upside-down in memory (see the PDF doc about pixel
        // access). For normal textures this is matches what OpenGL expects, but for cubemaps
        // OpenGL follows the RenderMan criteria, which requires flipping the images. You can flip
        // the images on disk, or by software. Here we opted for the second approach.
        if (m_impl->m_img != nullptr) {
            FreeImage_FlipVertical(m_impl->m_img);
        }
    }

    bool image::ok()
    {
        return (m_impl->m_img != nullptr);
    }

    unsigned int image::get_width()
    {
        return (m_impl->m_img != nullptr? FreeImage_GetWidth(m_impl->m_img) : 0U);
    }

    unsigned int image::get_height()
    {
        return (m_impl->m_img != nullptr? FreeImage_GetHeight(m_impl->m_img) : 0U);
    }

    image_format image::get_format()
    {
        if (m_impl->m_img == nullptr) return image_format::none;

        // From the FreeImage doc: the pixel layout used by this model is OS dependant. Using a byte
        // by byte memory order to label the pixel layout, then FreeImage uses a BGR[A] pixel layout
        // under a Little Endian processor (Windows, Linux) and uses a RGB[A] pixel layout under a
        // Big Endian processor (Mac OS X or any Big Endian Linux / Unix). This choice was made to
        // ease the use of FreeImage with graphics API. When runing on Linux, FreeImage stores data
        // in BGR / BGRA format.
        unsigned int depth = FreeImage_GetBPP(m_impl->m_img) / 8U;
        return (depth == 3U? image_format::bgr : image_format::bgra);
    }

    const unsigned char* image::get_data()
    {
        return (m_impl->m_img != nullptr? FreeImage_GetBits(m_impl->m_img) : nullptr);
    }

    void system_initialize()
    {
        if (!is_glfw_initialized) {
            log(LOG_LEVEL_DEBUG, "initializing system");
            if(!glfwInit())
            {
                throw std::runtime_error("open_window: failed to initialize GLFW.");
            }
            is_glfw_initialized = true;
        }
    }

    window_id new_window(std::size_t width, std::size_t height, bool fullscreen)
    {
        if (!is_glfw_initialized) return nwindow;

        window new_window;

        glfwWindowHint(GLFW_SAMPLES, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // Open a glfw window and create its OpenGL context
        new_window.m_glfw_window = glfwCreateWindow(width, height, "cgs", fullscreen? glfwGetPrimaryMonitor() : nullptr, nullptr);
        if(new_window.m_glfw_window == nullptr) {
            throw std::runtime_error("new_window: failed to open GLFW new_window.m_glfw_window. If you have an Intel GPU prior to HD 4000, they are not OpenGL 3.3 compatible.");
        }
        glfwMakeContextCurrent(new_window.m_glfw_window);

        // Register callbacks
        glfwSetErrorCallback(error_callback);
        glfwSetKeyCallback(new_window.m_glfw_window, key_callback);
        glfwSetCursorPosCallback(new_window.m_glfw_window, mouse_move_callback);

        // Disable vertical synchronization (results in a noticeable fps gain). This needs to be done
        // after calling glfwMakeContextCurrent, since it acts on the current context, and the context
        // created with glfwCreateWindow is not current until we make it explicitly so with
        // glfwMakeContextCurrent
        glfwSwapInterval(1);

        new_window.m_last_mouse_x = (float) width / 2.0f;
        new_window.m_last_mouse_y = (float) height / 2.0f;

        // Ensure we can capture the escape key being pressed below
        glfwSetInputMode(new_window.m_glfw_window, GLFW_STICKY_KEYS, GL_TRUE);
        // Hide the mouse and enable unlimited mouvement
        glfwSetInputMode(new_window.m_glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // Set the mouse at the center of the screen
        glfwPollEvents();
        glfwSetCursorPos(new_window.m_glfw_window, width / 2, height / 2);

        window_id w = std::find_if(windows.begin(), windows.end(), [](const window& w) { return !w.m_used; }) - windows.begin();
        if (w == windows.size()) {
            windows.push_back(new_window);
        } else {
            windows[w] = new_window;
        }

        return w;
    }

    void delete_window(window_id w)
    {
        if (is_glfw_initialized && w < windows.size() && windows[w].m_used) {
            log(LOG_LEVEL_DEBUG, "closing window");
            glfwDestroyWindow(windows[w].m_glfw_window);
            windows[w] = window{};     // set all members to default values
            windows[w].m_used = false; // soft removal
        }
    }

    window_id get_first_window()
    {
        if (!is_glfw_initialized) return nwindow;
        auto it = std::find_if(windows.begin(), windows.end(), [](const window& w) { return w.m_used; });
        return (it != windows.end() ? it - windows.begin() : nwindow);
    }

    window_id get_next_window(window_id w)
    {
        if (!is_glfw_initialized) return nwindow;
        auto it = std::find_if(windows.begin() + w + 1, windows.end(), [](const window& w) { return w.m_used; });
        return (it != windows.end() ? it - windows.begin() : nwindow);
    }


    void poll_window_events()
    {
        if (!is_glfw_initialized) return;

        for (auto &w : windows) {
            w.m_events.clear();
        }

        glfwPollEvents();
    }

    void get_window_events(window_id w, std::vector<cgs::event>* events)
    {
        if (!is_glfw_initialized) return;

        if (is_glfw_initialized && w < windows.size() && windows[w].m_used) {
            *events = windows[w].m_events;
        }
    }

    float get_time()
    {
        if (!is_glfw_initialized) return 0.0f;

        return (float) glfwGetTime();
    }

    void swap_buffers(window_id w)
    {
        if (is_glfw_initialized && w < windows.size() && windows[w].m_used) {
            glfwSwapBuffers(windows[w].m_glfw_window);
        }
    }

    unique_window make_window(std::size_t width, std::size_t height, bool fullscreen)
    {
        return unique_window(new_window(width, height, fullscreen));
    }

    void system_finalize()
    {
        if (is_glfw_initialized) {
            log(LOG_LEVEL_DEBUG, "finalizing system");
            glfwTerminate();
            is_glfw_initialized = false;
        }
    }
} // namespace cgs
