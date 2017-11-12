#ifndef REAL_TIME_ENGINE
#define REAL_TIME_ENGINE

#include <memory>

namespace cgs
{
    class real_time_engine
    {
    public:
        real_time_engine(unsigned int max_errors = 200U);
        ~real_time_engine();
        void initialize();
        void process();
        void finalize();

    private:
        class real_time_engine_impl;
        std::unique_ptr<real_time_engine_impl> m_impl;
    };
}

#endif
