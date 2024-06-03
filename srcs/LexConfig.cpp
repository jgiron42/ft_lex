#include "LexConfig.hpp"
#include <utility>
#include <exception>
#include "RegexWrapper.hpp"
#include <iostream>
#include <string.h>

LexConfig::LexConfig(std::list<std::string> files) : files(std::move(files)), is_yytext_an_array(false) {
	this->states["INITIAL"] = {false, Nfa<258>::new_state()};
	this->states["YY_BOL_INITIAL"] = {false, Nfa<258>::new_state()};
	this->parse();
}

bool LexConfig::open_file() {
	this->filestream.close();
	if (this->files.empty())
		return false;
	this->filestream.open(this->files.front());
	if (!this->filestream.is_open())
		throw std::runtime_error("Can't open " + this->files.front() + ": " + strerror(errno));
	this->files.pop_front();
	return true;
}

void LexConfig::parse() {
	this->parseDefSection();
	this->parseRulesSection();
	this->parseUserSubroutinesSection();
}

std::string LexConfig::parseInline() {
	std::string ret;
	while (this->filestream.good() || this->open_file())
	{
		std::string line;
		std::getline(this->filestream, line);
		if (line == "%}")
			break;
		ret.append(line + '\n');
	}
	if (!this->filestream.good())
		throw std::runtime_error("Unexpected end of input while reading block");
	return ret;
}

void LexConfig::parseDefOption(std::string &line) {
	line.erase(line.begin());
	std::string key = line.substr(0, line.find_first_of(" \t\n"));
	if (key == "array")
		this->is_yytext_an_array = true;
	else if (key == "pointer")
		this->is_yytext_an_array = false;
	else if (key == "s" || key == "S")
		for (auto it = std::regex_token_iterator(line.begin() + key.length(), line.end(), (const std::regex&)regw["[[:blank:]]+(" REG_C_ID ")"],1); it != std::regex_token_iterator<std::string::iterator>(); it++)
		{
			this->states.insert(std::pair(*it, start_condition{false, Nfa<258>::new_state()}));
			this->states.insert(std::pair("YY_BOL_" + it->str(), start_condition{false, Nfa<258>::new_state()}));
		}
	else if (key == "x" || key == "X")
		for (auto it = std::regex_token_iterator(line.begin() + key.length(), line.end(), (const std::regex&)regw["[[:blank:]]+(" REG_C_ID ")"],1); it != std::regex_token_iterator<std::string::iterator>(); it++)
		{
			this->states.insert(std::pair(*it, start_condition{true, Nfa<258>::new_state()}));
			this->states.insert(std::pair("YY_BOL_" + it->str(), start_condition{true, Nfa<258>::new_state()}));
		}
	else if (key.size() == 1 && !strchr("pnaeko", key[0]))
		{}
	else
		throw std::runtime_error("unrecognized option: " + key);

}

void LexConfig::parseDefiniton(const std::string &line) {
	std::match_results< std::string ::const_iterator> m;
	if (!(regw[REG_DEF](line, m)))
		throw std::runtime_error("Invalid definition: " + line);
	this->defines[m[1]] = m[2];
}

void LexConfig::parseDefSection() {
	while (this->filestream.good() || this->open_file())
	{
		std::string line;
		std::getline(this->filestream, line);
		if (line.empty())
			continue;
		else if (line == "%%")
			return;
		else if (line == "%{")
			this->header_content += parseInline();
		else if (line == "%}")
		{}//error
		else if (line.starts_with("%"))
			parseDefOption(line);
		else
			parseDefiniton(line);
	}
	throw std::runtime_error("Unexpected end of input");
}

bool		is_bracket_balanced(std::string s)
{
	s = std::regex_replace(s, (const std::regex &)regw["(" REG_CSTRING ")|(" REG_C_MULTILINE_COMMENT ")|(" REG_CHAR_LITERAL ")"], ""); // get rid of c strings and comments
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


std::string LexConfig::parse_frag(std::string s) {
	while ((this->filestream.good() || this->open_file()) &&
			!is_bracket_balanced(s))
	{
		std::string line;
		std::getline(this->filestream, line);
		s.append("\n");
		s.append(line);
	}
	if (this->filestream.eof())
		throw std::runtime_error("Unexpected end of input while reading fragment");
	else if (this->filestream.bad())
		throw std::runtime_error("Error reading input file: " + std::string(strerror(errno)));
	return s;
}

std::list<std::string> LexConfig::parse_states(std::string s) {
	auto it = std::regex_token_iterator(s.begin(), s.end(), (const std::regex&)regw["[<>,]"], -1);
	auto end = std::regex_token_iterator<std::string::iterator, char>();
	std::list<std::string> ret;


	for (; it != end; it++)
		if (!it->str().empty())
			ret.push_back(*it);
	return ret;
}

void LexConfig::parseRulesSection()
{
	while (this->filestream.good() || this->open_file()) {
		std::string line;
		std::getline(this->filestream, line);
		if (regw[REG_BLANK_LINE](line))
			continue;
		if (line == "%%")
			break;
		else if (line == "%{")
			this->yylex_user_content += parseInline();
		std::match_results<std::string::const_iterator> results;
		if (!regw[REG_RULE](line, results))
			throw std::runtime_error("Invalid rule: " + line);
		bool bol = false;
		std::string reg = results[4].str();
		if (reg[0] == '^')
		{
			reg.erase(0, 1);
			bol = true;
		}
		if (results[10].matched) // this is a hack to allow trailing context without any special handling
		{
			std::string start_condition = "YY_TRAILING_RULE_" + std::to_string(this->rules.size() + 1);
			std::string trailing = results[11].matched ? std::string(results[11]) : "\n";
			this->states.insert({start_condition, {true, nfa_type::new_state()}});
			LexConfig::rule with_trailing{
				.regex = LexRegex(this->defines, (reg + trailing)),
				.frag = parse_frag("{yy_pop_match();yyless(0);yy_restore_save();yystate = 1 + " + start_condition + ";goto yy_match_begin;}"),
				.start_conditions = results[1].matched
						? parse_states(results[1])
						: std::list<std::string>(),
		        .bol = bol,
				.user_rule = false
			};
			this->rules.push_front(with_trailing);
			LexConfig::rule beginning {
				.regex = LexRegex(this->defines, reg),
				.frag = parse_frag(results[19]),
				.start_conditions = std::list<std::string>{start_condition},
				.bol = false,
				.user_rule = true
			};
			this->rules.push_back(beginning);
		}
		else {
			LexConfig::rule new_rule{
					.regex = LexRegex(this->defines, reg),
					.frag = parse_frag(results[19]),
					.start_conditions = results[1].matched
										? parse_states(results[1])
										: std::list<std::string>(),
					.bol = bol,
					.user_rule = true
			};
			this->rules.push_back(new_rule);
		}
	}
}

void LexConfig::parseUserSubroutinesSection() {
	while (this->filestream.good() || this->open_file())
	{
		std::string line;
		std::getline(this->filestream, line);
		this->user_subroutines.append(line).push_back('\n');
	}
}

void LexConfig::serialize_states() {
	std::string s;
	for (auto &p : this->states)
		s.append("#define " + p.first + " " + std::to_string(p.second.nfa_state - 1) + '\n');
	this->generator.set("STATES_DEFINITION", s);
}

void LexConfig::serialize_accept() {
	std::string s;
	s.append("{ ");
	std::string line;
	for (auto &s : dfa_type::all_states)
		line.append(std::to_string(s.accept) + ", ");
	s.append(line).append("}\n");
	this->generator.set("ACCEPTS", s);
}

void LexConfig::serialize_transitions() {

	std::string ret;
	ret += "{\n";
	for (size_t i = 0; i < dfa_type::all_states.size(); i++)
	{
		auto &s = dfa_type::all_states[i];
		ret += "{\n";
		std::string line;
		for (int sym = 0; sym < alphabet_size; sym++)
		{
			if (sym != 0)
				ret.append(", ");
			if (!(sym % 16))
				ret.push_back('\n');
			ret += std::to_string(s.transitions[sym]);
		}
		ret.push_back('\n');
		ret.append("}" +  std::string(i + 1 == dfa_type::all_states.size() ? "" : ","));
	}
	ret.append("}\n");
	this->generator.set("TRANSITIONS", ret);

}

void LexConfig::serialize_rules()
{
	std::string s;
	for (auto &e : extra_rules)
	{
		s += "case " + std::to_string(e.regex.id) + ":";
		s += "\n"  + e.frag + "\n";
		s += "break;";
	}
	for (auto &e : rules)
	{
		s += "case " + std::to_string(e.regex.id) + ":";
		if (!regw["\\|[[:blank:]]*"](e.frag)) {
			s += "\n"  + e.frag + "\n";
			s += "break;";
		}
		else
			s += ";\n";
	}
	this->generator.set("RULES", s);
}

Generator &LexConfig::get_generator() {
	if (this->is_yytext_an_array)
		this->generator.set("DEFINES", "#define YY_TEXT_ARRAY 1");
	this->generator.set("HEADER_CONTENT", this->header_content);
	this->generator.set("YYLEX_USER_CONTENT", this->yylex_user_content);
	this->generator.set("USER_SUBROUTINES", this->user_subroutines);
	this->serialize_accept();
	this->serialize_rules();
	this->serialize_transitions();
	this->serialize_states();
	return this->generator;
}
