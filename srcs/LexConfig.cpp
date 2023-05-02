#include "LexConfig.hpp"
#include <utility>
#include <exception>
#include "RegexWrapper.hpp"
#include <iostream>

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
//		std::cout << line << std::endl;
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

void LexConfig::serialize_states(generator &g) {
	for (auto &p : this->states)
		g.put("#define " + p.first + " " + std::to_string(p.second.nfa_state - 1));
}

void LexConfig::serialize_accept(generator &g) {
	g.put("int yyaccept_table[] = { ").indent();
	std::string line;
	for (auto &s : dfa_type::all_states)
		line.append(std::to_string(s.accept) + ", ");
	g.put(line).dedent().put("};");
}

void LexConfig::serialize_transitions(generator &g) {
	g.put("int yytransitions[][258] = {").indent();
	for (size_t i = 0; i < dfa_type::all_states.size(); i++)
	{
		auto &s = dfa_type::all_states[i];
		g.put("{").indent();
		std::string line;
		for (int sym = 0; sym < alphabet_size; sym++)
		{
			if (sym != 0)
				line.append(", ");
			if (!(sym % 16))
			{
				g.put(line);
				line.clear();
			}
			line += std::to_string(s.transitions[sym]);
		}
		g.put(line);
		line.clear();
		g.dedent().put("}" + std::string(i + 1 == dfa_type::all_states.size() ? "" : ","));
	}
	g.dedent().put("};");
}

void LexConfig::serialize_rules(generator &g)
{
	g.put("switch(yyaccepted)")
	.put("{")
	.indent()
	.put("case 0:")
	.put("ECHO;")
	.put("break;");
	for (auto &e : extra_rules)
	{
		g.put("case " + std::to_string(e.regex.id) + ":");
		g.indent().put(e.frag).dedent();
		g.put("break;");
	}
	for (auto &e : rules)
	{
		g.put("case " + std::to_string(e.regex.id) + ":");
		if (!regw["\\|[[:blank:]]*"](e.frag)) {
			g.indent().put(e.frag).dedent();
			g.put("break;");
		}
		else
			g.put(";");
	}
	g.dedent().put("}");
}

void LexConfig::serialize_routines(generator &g) {
	g.put(R"(
typedef struct {
	size_t pos;
	int rule;
}		yymatch;

yymatch	*yy_match_stack = NULL;
int		yy_match_stack_size = 0;
int		yy_match_stack_cap = 0;

void	yy_push_accept(yymatch m)
{
	if (yy_match_stack_size + 1 > yy_match_stack_cap)
	{
		if (!yy_match_stack_cap)
			yy_match_stack_cap = 1;
		else
			yy_match_stack_cap *= 2;
		yy_match_stack = (yymatch *)realloc(yy_match_stack, yy_match_stack_cap * sizeof(yymatch));
	}
	yy_match_stack[yy_match_stack_size] = m;
	yy_match_stack_size++;
}

void yy_pop_match()
{
	if (yy_match_stack_size > 0)
		yy_match_stack_size--;
}

void yy_clear_stack()
{
	yy_match_stack_size = 0;
}

yymatch yy_top_match()
{
	yymatch ret = yy_match_stack[yy_match_stack_size - 1];
	return ret;
}

void yy_restore_save()
{
	if (yysave)
	{
		yy_buffer[yyindex] = yysave;
		yysave = 0;
	}
}

int  yywrap(void);

int yy_read_more()
{
	begin:;
	if (!yyin && yywrap())
		return 0;
	static char *line = NULL;
	static size_t n = 0;
	int ret = getline(&line, &n, yyin);
	if (ret == -1 && errno)
		return 0;
	if (ret == -1)
	{
		if (yywrap())
			return 0;
		goto begin;
	}
	if (!yy_buffer)
		yy_buffer = (char*)calloc(ret + 1, 1);
	else
		yy_buffer = (char*)realloc(yy_buffer, yyindex + strlen(yy_buffer + yyindex) + ret + 1);
	strcat(yy_buffer + yyindex, line);
	return ret;
}

void	yy_flush_buffer()
{
	if (yysave)
	{
		yy_buffer[yyindex] = yysave;
		yysave = 0;
	}
	if (yynoflush)
	{
		yynoflush = false;
		return;
	}
	memmove(yy_buffer, yy_buffer + yyindex, strlen(yy_buffer + yyindex) + 1);
	yyindex = 0;
}

int input(void)
{
	if (yysave)
	{
		char ret = yysave;
		yysave = 0;
		yyindex++;
		return ret;
	}
	if (!yy_buffer[yyindex] && !yy_read_more())
		return 0;
	char ret = yy_buffer[yyindex];
	yyindex++;
	return ret;
}

int	unput(char c)
{
	if (yyindex == 0)
		return 0;
	if (yysave)
	{
		yy_buffer[yyindex] = yysave;
		yysave = c;
		yyindex--;
		yy_buffer[yyindex] = 0;
		return 0;
	}
	yyindex--;
	yy_buffer[yyindex] = c;
	return 0;
}

int	yymore()
{
	yynoflush = true;
	return 0;
}

int  yyless(int n)
{
	if (n > yyindex)
		return 1;
	if (yysave)
	{
		yy_buffer[yyindex] = yysave;
		yysave = yy_buffer[n];
		yy_buffer[n] = 0;
	}
	yyleng = n;
	yyindex = n;
	return 0;
}

)");
}

void LexConfig::serialize_yylex(generator &g) {
	g.put("int yylex(void)")
	.put("{").indent()
	.put("static int yy_start_state = INITIAL + 1;")
	.put("static bool yybol = true;")
	.put(this->yylex_user_content)
	.put("if (!yy_buffer && !yy_read_more())")
	.indent().put("return 0;").dedent()
	.put("while (1)")
	.put("{").indent()
	.put("yy_flush_buffer();")
	.put("yy_clear_stack();")
	.put("int yystate = yy_start_state + yybol;")
	.put("yy_match_begin:;")
	.put("int yyaccepted = 0;")
	.put("if (!yy_buffer[0] && !yy_read_more())")
	.indent().put("return 0;").dedent()
	.put("while(yystate && (yy_buffer[yyindex] || yy_read_more()))")
	.put("{")
	.indent()
			.put("yystate = yytransitions[yystate - 1][yy_buffer[yyindex]];")
			.put("yyindex++;")
		.put("if (yystate && yyaccept_table[yystate - 1])")
		.indent()
//			.put("{printf(\"add accept: %c %d\\n\",yy_buffer[yyindex], yyaccept_table[yystate - 1]);yy_push_accept((yymatch){yyindex, yyaccept_table[yystate - 1]});}")
			.put("yy_push_accept((yymatch){yyindex, yyaccept_table[yystate - 1]});")
		.dedent()
	.dedent().put("}")
	.put("yy_handle_match:;")
	.put("if (yy_match_stack_size == 0)")
	.put("{").indent()
		.put("yyleng = 1;")
		.put("yyaccepted = 0;")
	.dedent().put("}")
	.put("else")
	.put("{").indent()
		.put("yyleng = yy_top_match().pos;")
		.put("yyaccepted = yy_top_match().rule;")
	.dedent().put("}")
//	.put("printf(\"accepted: %d\\n\", yyaccepted);")
	.put("yyindex = yyleng;")
	.put("yysave = yy_buffer[yyindex];")
	.put("yybol = (yy_buffer[yyleng - 1] == '\\n');")
	.put("yy_buffer[yyindex] = 0;");
	if (is_yytext_an_array)
	{
		g.put("memcpy(yytext, yy_buffer, yyleng);")
		.put("yytext[yyleng] = 0;");
	}
	else
	{
		g.put("yytext = yy_buffer;");
	}
	this->serialize_rules(g);
	g.dedent().put("}");
	g.dedent().put("}");
}

void LexConfig::serialize(std::ostream &out) {
	generator g(out);
	g.put(R"(
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#define ECHO do{printf("%s", yytext);}while(0)
#define BEGIN yy_start_state = 1 +
#define CURRENT_START_CONDITION (yy_start_state - 1)
#define REJECT do{yy_pop_match();yy_buffer[yyindex] = yysave;yysave = 0;goto yy_handle_match;}while(0)
#ifndef YYLMAX
# define YYLMAX 10000
#endif
)");
	g.put(this->header_content);
	this->serialize_states(g);
	g.put(R"(
char	*yy_buffer = NULL;
size_t	yyindex = 0;
size_t	yyleng = 0;
FILE	*yyin;
bool	yynoflush = false;
char	yysave = 0;
)");
	if (is_yytext_an_array)
		g.put("char yytext[YYLMAX];");
	else
		g.put("char *yytext;");
	this->serialize_transitions(g);
	this->serialize_accept(g);
	this->serialize_routines(g);
	this->serialize_yylex(g);
	g.put(this->user_subroutines);
}