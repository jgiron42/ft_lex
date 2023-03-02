#include <iostream>
#include "LexParser.hpp"

int main(int argc, char **argv)
{
	if (argc != 2)
		return 1;
	LexParser lexParser(argv[1]);
	try{
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
	}
}