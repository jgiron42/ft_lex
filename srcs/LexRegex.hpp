#ifndef FT_LEX_LEXREGEX_HPP
#define FT_LEX_LEXREGEX_HPP
#include <list>
#include <variant>
#include <bitset>
#include <memory>
#include <optional>
#include <map>
#include <stack>
#include "Nfa.hpp"

class LexRegex {
public:
	static const int alphabet_size = 256 + 2;
private:
	typedef Nfa<alphabet_size>	nfa_type;
	struct dup;
	struct node {
		enum {
			TERMINAL,
			CONCATENATION,
			DUP,
			ALTERNATION,
			EMPTY
		}	type;
		std::variant<
		        std::bitset<alphabet_size>,
				std::list<node>,
				std::shared_ptr<dup>,
				std::list<node>,
				std::monostate
		> value;
		node() = default;
		node(decltype(type) t) : type(t) {}
		node(const std::bitset<alphabet_size> &s) : type(TERMINAL) {this->value.emplace<TERMINAL>(s);}
	};
	struct dup {
		int					from;
		int					to;
		node	other;
	};
	struct token {
		enum token_type {
			TERMINAL,
			QUOTE,
			LPAR,
			RPAR,
			DEFINE,
			SINGLE_CHAR_DUP,
			INTERVAL,
			PIPE,
			SLASH
		} type;
		typedef std::variant<
				std::bitset<alphabet_size>,
				std::monostate,
				std::monostate,
				std::monostate,
				std::string,
				std::pair<int, int>,
				std::pair<int, int>,
				std::monostate,
				std::monostate
		>				value_type;
		value_type		value;
	};
	std::list<token>							token_list;
	const std::map<std::string, std::string>	&defines;
	node										root;

public:
	int											id;
	Nfa<alphabet_size>							nfa;
	class UnexpectedToken : public std::runtime_error {
	public:
		UnexpectedToken(const std::string &s) : std::runtime_error("Unexpected token: " + s) {}
	};
	class InvalidInterval : public std::exception {};
	class InvalidDefine : public std::exception {};
	LexRegex(const std::map<std::string, std::string>&, const std::string & = "");
	void parse(const std::string &);
private:
	static int get_id()
	{
		static int current = 1;
		return current++;
	}
	std::bitset<alphabet_size>	get_bitset_char(const std::string &);
	std::bitset<alphabet_size>	get_bitset_dot();
	std::bitset<alphabet_size>	get_bitset_class(const std::string &class_name);
	std::bitset<alphabet_size>	get_bitset_equivalence_class(const std::string &);
	std::bitset<alphabet_size>	get_bitset_collating_element(const std::string &);
	std::pair<int, int>			get_single_char_dup(char);
	std::pair<int, int>			get_interval(const std::string &);
	std::bitset<alphabet_size>	get_bitset_range(const std::string &, const std::string &);
	int							get_escaped_val(const std::string &);
	std::bitset<alphabet_size>	get_bitset_bracket(const std::string &);
	void 						tokenize_quoted(std::deque<token> &, const std::string &);
	std::stack<token>			tokenize(std::string s);
	node parse_terminal(std::stack<token> &);
	node parse_grouping(std::stack<token> &);
	node parse_def(std::stack<token> &);
	node parse_single_char_dup(std::stack<token> &);
	node parse_concat(std::stack<token> &);
	node parse_interval(std::stack<token> &);
	node parse_interval_concat(std::stack<token> &);
	node parse_alternation(std::stack<token> &);
	void	unexpected(std::stack<token> &);
	void	expect(std::stack<token> &, token::token_type);
	bool	accept(std::stack<token> &, token::token_type);
	bool	is_of_type(std::stack<token> &, std::initializer_list<token::token_type> &&);

public:
	void	compile();
private:
	void	compile_terminal(const node &n, nfa_type::state_id from, nfa_type::state_id to);
	void	compile_concatenation(const node &n, nfa_type::state_id from, nfa_type::state_id to);
	void	compile_duplication(const node &n, nfa_type::state_id from, nfa_type::state_id to);
	void	compile_alternation(const node &n, nfa_type::state_id from, nfa_type::state_id to);
	void	compile_node(const node &n, nfa_type::state_id from, nfa_type::state_id to);
};


#endif
