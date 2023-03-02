#ifndef FT_LEX_LEXPARSER_HPP
# define FT_LEX_LEXPARSER_HPP
# include <string>
# include <fstream>
# include <list>
# include <map>
# include <set>
# define REG_ESCSEQ R"(\\([^)""\n" R"(]|[0-7]{1,3}|x[[:xdigit:]]{1,2}))"
# define REG_CESCSEQ R"(\\(.|[0-7]{1,3}|x[[:xdigit:]]{1,2}))"
# define REG_PARTIAL_CSTRING "\"(" REG_ESCSEQ "|[^\n\"\\\\])*"
# define REG_CSTRING REG_PARTIAL_CSTRING "\""
# define REG_C_PARTIAL_MULTILINE_COMMENT  R"(/\*([^*]|\*[^/])*)"
# define REG_C_MULTILINE_COMMENT REG_C_PARTIAL_MULTILINE_COMMENT "\\*/"
// regex that match a lex style regex (does not check the validity of the regex) (contain 3 subexpr)
# define REG_REG R"(([^[:space:]]|"()" REG_ESCSEQ R"(|[^\\"])*"|\[.?([^]]|[:=.]])*])+)"
// regex that match a c identifier
# define REG_C_ID R"([[:alpha:]_][[:alnum:]_]*)"
// regex that match a rule (match result: 1 is the regex, 6: ECHO, 7: REJECT, 8: BEGIN, 9: BEGIN_ARG, 10: PIPE,  11: C_FRAG)
# define REG_RULE "^(" REG_REG ")" "[[:blank:]]+" "((ECHO;)|(REJECT;)|(BEGIN[[:blank:]]+(" REG_C_ID ");)" "|(\\|)|(\\{.*))[[:blank:]]*$"
# define REG_BLANK_LINE "^[[:blank:]]*$"
# define REG_DEF "^(" REG_C_ID ")[[:blank:]]+(.+)$"

// this class parse the .l file
class LexParser {
private:
	struct rule {
		std::string regex;
		enum {
			ACTION_PIPE,
			ACTION_ECHO,
			ACTION_REJECT,
			ACTION_BEGIN,
			ACTION_FRAG,
		} type;
		std::string frag;
		std::string begin_target;
	};
	std::string							filename;
	std::ifstream						filestream;

	std::string							header_content;
	std::map<std::string, std::string>	defines;
	std::list<rule>						rules;
	std::set<std::string>				exclusive_states;
	std::set<std::string>				non_exclusive_states;
	bool			is_yytext_an_array;
public:
	LexParser(std::string filename);
private:
	void	parseDefSection();
	void	parseRulesSection();
	void	parseInline();
	std::string 	parse_frag(std::string );
	void	parseDefOption(std::string &line);
	void	parseDefiniton(const std::string &line);
	void	parse();
};


#endif
