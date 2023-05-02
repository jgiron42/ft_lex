#include "generator.hpp"
#include <regex>
#include <iostream>

generator::generator(std::ostream &out) : out(out), indentation() {}

generator &generator::indent(int n) {for (;n > 0;n--) indentation.push_back('\t'); return *this;}

generator &generator::dedent(int n) {for (;n > 0;n--) indentation.pop_back(); return *this;}

generator &generator::put(const std::string &s) {
	this->indent_block(s);
	out << indentation << s << std::endl;
	return *this;
}

std::string generator::indent_block(std::string s) {
	size_t pos = 0;
	while ((pos = s.find('\n', pos)) != s.npos)
		s.insert(++pos, indentation);
	return s;
}