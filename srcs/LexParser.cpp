#include "LexParser.hpp"
#include <utility>
#include <exception>
#include "RegexWrapper.hpp"

LexParser::LexParser(std::string filename) : filename(std::move(filename)) {
	this->filestream.open(filename);
	if (!this->filestream.is_open())
		throw std::exception(); // TODO: better exception
}

void LexParser::lex() {

}

void LexParser::parseInline() {
	while (this->filestream.good())
	{
		std::string line;
		std::getline(this->filestream, line);
		if (line == "%}\n")
			break;
		this->header_content.append(line);
	}
	if (!this->filestream.good())
	{}//todo: error
}

void LexParser::parseDefOption(std::string &line) {
	line.erase(line.begin());
	std::string key = line.substr(0, line.find_first_of(" \t\n"));
	if (key == "array")
		this->is_yytext_an_array = true;
	else if (key == "pointer")
		this->is_yytext_an_array = false;
	else if (key.size() == 1 && !strchr("pnaeko", key[0]))
	{}//todo: error
	else
	{}//todo: error
}

void LexParser::parseDefiniton(const std::string &line) {
	std::match_results< std::string ::const_iterator> m;
	if (!regw["^([[:alpha:]][[:alnum:]])[ \t]+(.+)\n*$"](line, m))
		{throw std::exception();}//todo: error
	this->defines[m[1]] = m[2];
}

void LexParser::parseDefSection() {
	while (this->filestream.good())
	{
		std::string line;
		std::getline(this->filestream, line);
		if (line == "\n")
			continue;
		else if (line == "%%\n")
			break;
		else if (line == "%{\n")
			parseInline();
		else if (line == "%}\n")
		{}//error
		else if (line.starts_with("%"))
			parseDefOption(line);
		else
			parseDefiniton(line);
	}
	throw std::exception(); // TODO: better exception
}