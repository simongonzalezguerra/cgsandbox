#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include <memory>
#include <string>
#include <vector>

namespace cgs
{
    //-----------------------------------------------------------------------------------------------
    // Types
    //-----------------------------------------------------------------------------------------------
    enum event_type
    {
        EVENT_KEY_PRESS = 0,
        EVENT_KEY_HOLD,
        EVENT_KEY_RELEASE,
        EVENT_MOUSE_MOVE
    };

    // Based on GLFW's key codes (we use the same values and names)
    typedef int key_code;

    /* The unknown key */
    constexpr key_code KEY_UNKNOWN           = 0; /* The unknown key */

    /* Printable keys */
    constexpr key_code KEY_SPACE             = 32;
    constexpr key_code KEY_APOSTROPHE        = 39;  /* ' */
    constexpr key_code KEY_COMMA             = 44;  /* , */
    constexpr key_code KEY_MINUS             = 45;  /* - */
    constexpr key_code KEY_PERIOD            = 46;  /* . */
    constexpr key_code KEY_SLASH             = 47;  /* / */
    constexpr key_code KEY_0                 = 48;
    constexpr key_code KEY_1                 = 49;
    constexpr key_code KEY_2                 = 50;
    constexpr key_code KEY_3                 = 51;
    constexpr key_code KEY_4                 = 52;
    constexpr key_code KEY_5                 = 53;
    constexpr key_code KEY_6                 = 54;
    constexpr key_code KEY_7                 = 55;
    constexpr key_code KEY_8                 = 56;
    constexpr key_code KEY_9                 = 57;
    constexpr key_code KEY_SEMICOLON         = 59;  /* ; */
    constexpr key_code KEY_EQUAL             = 61;  /* = */
    constexpr key_code KEY_A                 = 65;
    constexpr key_code KEY_B                 = 66;
    constexpr key_code KEY_C                 = 67;
    constexpr key_code KEY_D                 = 68;
    constexpr key_code KEY_E                 = 69;
    constexpr key_code KEY_F                 = 70;
    constexpr key_code KEY_G                 = 71;
    constexpr key_code KEY_H                 = 72;
    constexpr key_code KEY_I                 = 73;
    constexpr key_code KEY_J                 = 74;
    constexpr key_code KEY_K                 = 75;
    constexpr key_code KEY_L                 = 76;
    constexpr key_code KEY_M                 = 77;
    constexpr key_code KEY_N                 = 78;
    constexpr key_code KEY_O                 = 79;
    constexpr key_code KEY_P                 = 80;
    constexpr key_code KEY_Q                 = 81;
    constexpr key_code KEY_R                 = 82;
    constexpr key_code KEY_S                 = 83;
    constexpr key_code KEY_T                 = 84;
    constexpr key_code KEY_U                 = 85;
    constexpr key_code KEY_V                 = 86;
    constexpr key_code KEY_W                 = 87;
    constexpr key_code KEY_X                 = 88;
    constexpr key_code KEY_Y                 = 89;
    constexpr key_code KEY_Z                 = 90;
    constexpr key_code KEY_LEFT_BRACKET      = 91;  /* [ */
    constexpr key_code KEY_BACKSLASH         = 92;  /* \ */
    constexpr key_code KEY_RIGHT_BRACKET     = 93;  /* ] */
    constexpr key_code KEY_GRAVE_ACCENT      = 96;  /* ` */
    constexpr key_code KEY_WORLD_1           = 161; /* non-US #1 */
    constexpr key_code KEY_WORLD_2           = 162; /* non-US #2 */

    /* Function keys */
    constexpr key_code KEY_ESCAPE            = 256;
    constexpr key_code KEY_ENTER             = 257;
    constexpr key_code KEY_TAB               = 258;
    constexpr key_code KEY_BACKSPACE         = 259;
    constexpr key_code KEY_INSERT            = 260;
    constexpr key_code KEY_DELETE            = 261;
    constexpr key_code KEY_RIGHT             = 262;
    constexpr key_code KEY_LEFT              = 263;
    constexpr key_code KEY_DOWN              = 264;
    constexpr key_code KEY_UP                = 265;
    constexpr key_code KEY_PAGE_UP           = 266;
    constexpr key_code KEY_PAGE_DOWN         = 267;
    constexpr key_code KEY_HOME              = 268;
    constexpr key_code KEY_END               = 269;
    constexpr key_code KEY_CAPS_LOCK         = 280;
    constexpr key_code KEY_SCROLL_LOCK       = 281;
    constexpr key_code KEY_NUM_LOCK          = 282;
    constexpr key_code KEY_PRINT_SCREEN      = 283;
    constexpr key_code KEY_PAUSE             = 284;
    constexpr key_code KEY_F1                = 290;
    constexpr key_code KEY_F2                = 291;
    constexpr key_code KEY_F3                = 292;
    constexpr key_code KEY_F4                = 293;
    constexpr key_code KEY_F5                = 294;
    constexpr key_code KEY_F6                = 295;
    constexpr key_code KEY_F7                = 296;
    constexpr key_code KEY_F8                = 297;
    constexpr key_code KEY_F9                = 298;
    constexpr key_code KEY_F10               = 299;
    constexpr key_code KEY_F11               = 300;
    constexpr key_code KEY_F12               = 301;
    constexpr key_code KEY_F13               = 302;
    constexpr key_code KEY_F14               = 303;
    constexpr key_code KEY_F15               = 304;
    constexpr key_code KEY_F16               = 305;
    constexpr key_code KEY_F17               = 306;
    constexpr key_code KEY_F18               = 307;
    constexpr key_code KEY_F19               = 308;
    constexpr key_code KEY_F20               = 309;
    constexpr key_code KEY_F21               = 310;
    constexpr key_code KEY_F22               = 311;
    constexpr key_code KEY_F23               = 312;
    constexpr key_code KEY_F24               = 313;
    constexpr key_code KEY_F25               = 314;
    constexpr key_code KEY_KP_0              = 320;
    constexpr key_code KEY_KP_1              = 321;
    constexpr key_code KEY_KP_2              = 322;
    constexpr key_code KEY_KP_3              = 323;
    constexpr key_code KEY_KP_4              = 324;
    constexpr key_code KEY_KP_5              = 325;
    constexpr key_code KEY_KP_6              = 326;
    constexpr key_code KEY_KP_7              = 327;
    constexpr key_code KEY_KP_8              = 328;
    constexpr key_code KEY_KP_9              = 329;
    constexpr key_code KEY_KP_DECIMAL        = 330;
    constexpr key_code KEY_KP_DIVIDE         = 331;
    constexpr key_code KEY_KP_MULTIPLY       = 332;
    constexpr key_code KEY_KP_SUBTRACT       = 333;
    constexpr key_code KEY_KP_ADD            = 334;
    constexpr key_code KEY_KP_ENTER          = 335;
    constexpr key_code KEY_KP_EQUAL          = 336;
    constexpr key_code KEY_LEFT_SHIFT        = 340;
    constexpr key_code KEY_LEFT_CONTROL      = 341;
    constexpr key_code KEY_LEFT_ALT          = 342;
    constexpr key_code KEY_LEFT_SUPER        = 343;
    constexpr key_code KEY_RIGHT_SHIFT       = 344;
    constexpr key_code KEY_RIGHT_CONTROL     = 345;
    constexpr key_code KEY_RIGHT_ALT         = 346;
    constexpr key_code KEY_RIGHT_SUPER       = 347;
    constexpr key_code KEY_MENU              = 348;
    constexpr key_code KEY_LAST              = KEY_MENU;

    struct event
    {
        event_type type;
        key_code value;
        float abs_mouse_x;
        float abs_mouse_y;
        float delta_mouse_x;
        float delta_mouse_y;
    };

    enum class image_format
    {
        none,
        rgb,
        rgba,
        bgr,
        bgra
    };

    class image
    {   
    public:
        image();
        ~image();
        void load(const std::string& path);
        bool ok();
        unsigned int get_width();
        unsigned int get_height();
        image_format get_format();
        const unsigned char* get_data();

    private:
        class image_impl;
        std::unique_ptr<image_impl> m_impl;
    };

    //-----------------------------------------------------------------------------------------------
    // Public functions
    //-----------------------------------------------------------------------------------------------
    bool open_window(std::size_t width, std::size_t height, bool fullscreen);
    void close_window();
    bool is_context_created();
    std::vector<event> poll_events();
    float get_time();
    void swap_buffers();
} // namespace cgs

#endif // SYSTEM_HPP
