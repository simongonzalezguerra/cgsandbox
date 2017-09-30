#include "cgs/system.hpp"
#include "cgs/log.hpp"
#include "FreeImage.h"
#include "glfw3.h"

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

        //---------------------------------------------------------------------------------------------
        // Internal data structures
        //---------------------------------------------------------------------------------------------
        GLFWwindow*              window = nullptr;
        bool                     ok = false;
        std::vector<event>       events;
        key_event_type_map       event_types;     // map from GLFW key action value to event_type value
        error_code_map           error_codes;     // map from GLFW error codes to their names
        float                    last_mouse_x = 0.0f;
        float                    last_mouse_y = 0.0f;

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

        void key_callback(GLFWwindow*, int key, int, int action, int)
        {
            if (ok && window) {
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
                    events.push_back(e);
                }
            }
        }

        void mouse_move_callback(GLFWwindow*, double x, double y)
        {
            if (ok && window) {
                event e{EVENT_MOUSE_MOVE, KEY_UNKNOWN, (float) x, (float) y, (float) x - last_mouse_x, (float) y - last_mouse_y};
                events.push_back(e);
                last_mouse_x = x;
                last_mouse_y = y;

                // std::ostringstream oss;
                // oss << "mouse_move_callback: new delta_mouse_x: " << std::fixed << std::setprecision(2)
                //     << e.delta_mouse_x << ", new delta_mouse_y: " << e.delta_mouse_y;
                // cgs::log(cgs::LOG_LEVEL_DEBUG, oss.str());
            }
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
    bool open_window(std::size_t width, std::size_t height, bool fullscreen)
    {
        // Initialise GLFW
        if(!glfwInit())
        {
            log(LOG_LEVEL_ERROR, "open_window: failed to initialize GLFW.");
            return -1;
        }

        glfwWindowHint(GLFW_SAMPLES, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // Open a window and create its OpenGL context
        window = glfwCreateWindow(width, height, "cgs", fullscreen? glfwGetPrimaryMonitor() : nullptr, nullptr);
        if(window == nullptr){
            log(LOG_LEVEL_ERROR, "open_window: failed to open GLFW window. If you have an Intel GPU prior to HD 4000, they are not OpenGL 3.3 compatible.");
            glfwTerminate();
            return false;
        }
        ok = true;
        glfwMakeContextCurrent(window);

        // Register callbacks
        glfwSetErrorCallback(error_callback);
        glfwSetKeyCallback(window, key_callback);
        glfwSetCursorPosCallback(window, mouse_move_callback);

        // Disable vertical synchronization (results in a noticeable fps gain). This needs to be done
        // after calling glfwMakeContextCurrent, since it acts on the current context, and the context
        // created with glfwCreateWindow is not current until we make it explicitly so with
        // glfwMakeContextCurrent
        glfwSwapInterval(1);

        last_mouse_x = (float) width / 2.0f;
        last_mouse_y = (float) height / 2.0f;

        // Ensure we can capture the escape key being pressed below
        glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
        // Hide the mouse and enable unlimited mouvement
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // Set the mouse at the center of the screen
        glfwPollEvents();
        glfwSetCursorPos(window, width / 2, height / 2);

        return true;
    }

    void close_window()
    {
        if (window) {
            glfwTerminate();
            window = nullptr;
            ok = false;
            events.clear();
        }
    }

    bool is_context_created()
    {
        return (window != nullptr);
    }

    std::vector<cgs::event> poll_events()
    {
        events.clear();
        glfwPollEvents();

        return events;
    }

    float get_time()
    {
        return (float) glfwGetTime();
    }

    void swap_buffers()
    {
        if (window) {
            glfwSwapBuffers(window);        
        }
    }
} // namespace cgs
