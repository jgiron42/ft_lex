//
// Created by citron on 15/03/23.
//

#ifndef FT_LEX_GENERATOR_HPP
#define FT_LEX_GENERATOR_HPP
#include <ostream>

class generator {
private:
	std::ostream	&out;
	std::string		indentation;
public:
	generator(std::ostream &out);
	generator	&indent(int n = 1);
	generator	&dedent(int n = 1);
	generator	&put(const std::string & = std::string());
private:
	std::string	indent_block(std::string);
};


#endif //FT_LEX_GENERATOR_HPP
