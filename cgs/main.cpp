#include "cgs/real_time_engine.hpp"
#include "cgs/log.hpp"

#include <stdexcept>

int main()
{
    try {
        cgs::log_init();
        cgs::real_time_engine engine;
        engine.initialize();
        engine.process();
        engine.finalize();
    } catch(std::exception& ex) {
        cgs::log(cgs::LOG_LEVEL_ERROR, "real_time_engine: closing application");
    }

	return 0;
}
