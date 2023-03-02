
#ifndef FT_LEX_NFA_HPP
#define FT_LEX_NFA_HPP
#include <string>
#include <list>
#include <variant>
#include <exception>
#include <map>

class Nfa {
private:
	enum token_type{
		LITERAL,
		STATE_LIST,
		SLASH,
		SUBSTITUTION,
		ESCAPE_SEQUENCE,
		BRACKET_EXPRESSION,
		LPARENTHESIS,
		RPARENTHESIS,
		STAR,
		PLUS,
		QMARK,
		INTERVAL,
		PIPE
	};
	struct token {
		token_type type;
		std::variant<std::string, char, std::pair<int, int>, int> value;
	};
	std::string			str;
	std::list<token>	token_list;
	std::map<std::string, std::string>	&defines;
	class invalid_regex : public std::exception {
		const char * what() const noexcept override
		{return "invalid regex";}
	};
	class too_complicated_exception : public std::exception {
		const char * what() const noexcept override
		{return "input rules too complicated";}
	};
	class bad_iteration : public std::exception {
		const char * what() const noexcept override
		{return "bad iteration values";}
	};
	class invalid_define : public std::exception {
		const char * what() const noexcept override
		{return "undefined definition";}
	};
public:
	Nfa(std::map<std::string, std::string>	&defines, std::string regex);
	void lex();
};


#endif //FT_LEX_NFA_HPP
