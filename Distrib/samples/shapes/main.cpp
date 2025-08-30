#include "shapes.hpp"

int main(int argc, char** argv)
{
	sys::init();

	while(!sys::quit)
	{
		if(!sys::skip)
		{
			sys::keys();
			sys::vid::draw();
			sys::swap();
		}
	}

	sys::halt();

	exit(EXIT_SUCCESS);
}

