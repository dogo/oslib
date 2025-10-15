#include "shapes.hpp"

int main(int argc, char** argv)
{
	sys::init();

	while(sys::feed()) if(!sys::skip) { sys::draw(); sys::swap(); }

	sys::halt();

	exit(EXIT_SUCCESS);
}

