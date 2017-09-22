#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include <memory>

namespace samples
{
namespace model_viewer
{
    class controller
    {
    public:
        static void initialize();
        static void finalize();
        controller();
        ~controller();
        bool process();

    private:
        class controller_impl;
        std::unique_ptr<controller_impl> m_impl;
    };
} // namespace model_viewer
} // namespace samples

#endif
