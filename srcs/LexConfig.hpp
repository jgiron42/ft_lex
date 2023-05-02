#ifndef FT_LEX_LEXCONFIG_HPP
# define FT_LEX_LEXCONFIG_HPP
# include <string>
# include <fstream>
# include <list>
# include <map>
# include <set>
# include "LexRegex.hpp"
# include "generator.hpp"
# include "Dfa.hpp"
# define REG_ESCSEQ R"(\\([^)""\n" R"(]|[0-7]{1,3}|x[[:xdigit:]]{1,2}))"
# define REG_CESCSEQ R"(\\(.|[0-7]{1,3}|x[[:xdigit:]]{1,2}))"
# define REG_PARTIAL_CSTRING "\"(" REG_ESCSEQ "|[^\n\"\\\\])*"
# define REG_CSTRING REG_PARTIAL_CSTRING "\""
# define REG_C_PARTIAL_MULTILINE_COMMENT  R"(/\*([^*]|\*[^/])*)"
# define REG_C_MULTILINE_COMMENT REG_C_PARTIAL_MULTILINE_COMMENT "\\*/"
// regex that match a lex style regex (does not check the validity of the regex)
# define REG_REG R"(([^[:space:]"[/\$\\]|)" REG_ESCSEQ R"(|"([^\\"]|)" REG_ESCSEQ R"(|\\")*"|\[.?([^]]|[:=.]])*])+)"
// regex that match a c identifier
# define REG_C_ID R"([[:alpha:]_][[:alnum:]_]*)"
# define REG_STATES  "<(" REG_C_ID "(," REG_C_ID ")*)>"
# define REG_RULE "^(" REG_STATES ")?(" REG_REG ")(/(" REG_REG ")|(\\$))?" "([[:blank:]]+" "(\\|[[:blank:]]*|(\\{.*|[^|].*)))?$"
# define REG_BLANK_LINE "^[[:blank:]]*$"
# define REG_DEF "^(" REG_C_ID ")[[:blank:]]+(.+)$"

// this class parse the .l file
class LexConfig {
public:
	static const int alphabet_size = 258;
	typedef Nfa<alphabet_size> nfa_type;
	typedef Dfa<alphabet_size> dfa_type;
	struct rule {
		LexRegex				regex;
		std::string				frag;
		std::list<std::string>	start_conditions;
		bool					bol;
		bool					user_rule;
	};
	struct start_condition {
		bool				exclusive;
		nfa_type::state_id	nfa_state;
	};
	std::list<std::string>				files;
	std::ifstream						filestream;

	std::string								yylex_user_content;
	std::string								header_content;
	std::map<std::string, std::string>		defines;
	std::list<rule>							rules;
	std::list<rule>							extra_rules;
	std::map<std::string, start_condition>	states;
	std::string								user_subroutines;
	bool									is_yytext_an_array;
	LexConfig(std::list<std::string> files);
	class InvalidStartCondition : public std::runtime_error {
	public:
		InvalidStartCondition(const std::string &s) : std::runtime_error("Invalid start condition: " + s) {}
	};
private:
	bool					open_file();

	void					parseDefSection();
	void					parseRulesSection();
	void					parseUserSubroutinesSection();
	std::string				parseInline();
	std::string 			parse_frag(std::string );
	std::list<std::string>	parse_states(std::string );
	void					parseDefOption(std::string &line);
	void					parseDefiniton(const std::string &line);
	void					parse();

	void					serialize_states(generator &g);
	void					serialize_routines(generator &g);
	void					serialize_accept(generator &g);
	void					serialize_transitions(generator &g);
	void					serialize_rules(generator &g);
	void 					serialize_yylex(generator &g);
public:
	void					serialize(std::ostream &out);
};


#endif
