#include "samples/model_viewer/controller.hpp"

int main()
{
	samples::model_viewer::controller::initialize();
	samples::model_viewer::controller controller;

	bool done = false;
	do {
		done = controller.process();
	} while (!done);

	samples::model_viewer::controller::finalize();

	return 0;
}
