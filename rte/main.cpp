#include "real_time_engine.hpp"
#include "log.hpp"

#include <stdexcept>

int main(int argc, char** argv)
{
    try {
        rte::log_init();
        rte::real_time_engine engine;
        engine.initialize(argc, argv);
        engine.process();
        engine.finalize();
    } catch(std::exception& ex) {
        rte::log(rte::LOG_LEVEL_ERROR, ex.what());
        rte::log(rte::LOG_LEVEL_ERROR, "real_time_engine: closing application");
    }

	return 0;
}
