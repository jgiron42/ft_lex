#include "LexParser.hpp"
#include <utility>
#include <exception>
#include "RegexWrapper.hpp"
#include <iostream>

LexParser::LexParser(std::string filename) : filename(std::move(filename)) {
	this->filestream.open(this->filename);
	if (!this->filestream.is_open())
		throw std::exception(); // TODO: better exception
	this->parse();
}

void LexParser::parse() {
	this->parseDefSection();
	for (const auto& a : this->defines)
		std::cout << "le define: " << a.first << " -> " << a.second << std::endl;
	for (const auto& a : this->exclusive_states)
		std::cout << "le exclusive state: " << a << std::endl;
	for (const auto& a : this->non_exclusive_states)
		std::cout << "le non exclusive state: " << a << std::endl;
	this->parseRulesSection();
	for (const auto& a : this->rules)
	{
		std::cout << "le rule: " << std::endl
		<< "le regex de le rule: " << a.regex << std::endl
		<< "le type de le rule: " << a.type
		<< std::endl;
		if (a.type == rule::ACTION_FRAG)
			std::cout << "le frag de le rule: " << a.frag << std::endl;
	}
}

void LexParser::parseInline() {
	while (this->filestream.good())
	{
		std::string line;
		std::getline(this->filestream, line);
		if (line == "%}")
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
	else if (key == "s" || key == "S")
		this->non_exclusive_states.insert(
				std::regex_token_iterator(line.begin() + key.length(), line.end(), (const std::regex&)regw["[[:blank:]]+(" REG_C_ID ")"],1),
				std::regex_token_iterator<std::string::iterator>());
	else if (key == "x" || key == "X")
		this->exclusive_states.insert(
				std::regex_token_iterator(line.begin() + key.length(), line.end(), (const std::regex&)regw["[[:blank:]]+(" REG_C_ID ")"],1),
				std::regex_token_iterator<std::string::iterator>());
	else if (key.size() == 1 && !strchr("pnaeko", key[0]))
	{}//todo: error
	else
	{}//todo: error
}

void LexParser::parseDefiniton(const std::string &line) {
	std::match_results< std::string ::const_iterator> m;
	if (!(regw[REG_DEF](line, m)))
		{throw std::exception();}//todo: error
	this->defines[m[1]] = m[2];
}

void LexParser::parseDefSection() {
	while (this->filestream.good())
	{
		std::string line;
		std::getline(this->filestream, line);
		if (line.empty())
			continue;
		else if (line == "%%")
			return;
		else if (line == "%{")
			parseInline();
		else if (line == "%}")
		{}//error
		else if (line.starts_with("%"))
			parseDefOption(line);
		else
			parseDefiniton(line);
	}
	throw std::exception(); // TODO: better exception
}

bool		is_bracket_balanced(std::string s)
{
	s = std::regex_replace(s, (const std::regex &)regw["(" REG_CSTRING ")|(" REG_C_MULTILINE_COMMENT ")"], ""); // get rid of c strings and comments
	if (regw["(" REG_PARTIAL_CSTRING ")|(" REG_C_PARTIAL_MULTILINE_COMMENT ")"](s)) // if the string contain an unfinished comment or string
		return false;
	int bracket = 0;
	for (auto c : s)
	{
		if (c == '{')
			bracket++;
		if (c == '}')
			bracket--;
	}
	return bracket == 0;
}


std::string LexParser::parse_frag(std::string s) {
	while (this->filestream.good() && !is_bracket_balanced(s)) {
		{
			std::string line;
			std::getline(this->filestream, line);
			s.append("\n");
			s.append(line);
		}
		if (this->filestream.eof()) {}// todo: error
		else if (this->filestream.bad()) {}// todo: error
	}
	return s;
}


void LexParser::parseRulesSection()
{
	while (this->filestream.good()) {
		std::string line;
		std::getline(this->filestream, line);
		if (regw[REG_BLANK_LINE](line))
			continue;
		if (line == "%%")
			break;
		std::match_results<std::string::const_iterator> results;
		if (!regw[REG_RULE](line, results))
		{throw std::exception();}//todo: error
		LexParser::rule new_rule;
		new_rule.regex = results[1];
		if (results[6].matched)
			new_rule.type = rule::ACTION_ECHO;
		else if (results[7].matched)
			new_rule.type = rule::ACTION_REJECT;
		else if (results[8].matched)
		{
			new_rule.type = rule::ACTION_BEGIN;
			new_rule.begin_target = results[9];
		}
		else if (results[10].matched)
			new_rule.type = rule::ACTION_PIPE;
		else
		{
			new_rule.type = rule::ACTION_FRAG;
			new_rule.frag = parse_frag(results[11]);
		}
		this->rules.push_back(new_rule);
	}
}