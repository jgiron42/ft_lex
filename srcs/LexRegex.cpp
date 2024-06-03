#include "LexRegex.hpp"
#include <iostream>
#include "RegexWrapper.hpp"
#include <variant>

LexRegex::LexRegex(const std::map<std::string, std::string> &d,const std::string &regex) : defines(d), id(get_id()) {
	this->parse(regex);
	this->compile();
}

std::bitset<LexRegex::alphabet_size> LexRegex::get_bitset_char(const std::string &s) {
	int c = (unsigned char)s[0];
	if (c == '\\')
		c = get_escaped_val(s);
	return std::bitset<alphabet_size>().flip(c);
}

	std::bitset<LexRegex::alphabet_size> LexRegex::get_bitset_dot() {
	return std::bitset<LexRegex::alphabet_size>().flip().set('\n', false).set(256, false).set(257, false);
}

std::bitset<LexRegex::alphabet_size> LexRegex::get_bitset_class(const std::string &class_name) {
	std::bitset<alphabet_size> ret;
	wctype_t type = wctype(class_name.c_str());
	for (int i = 0; i < 256; i++)
		if (iswctype(i, type))
			ret[i] = true;
	return ret;
}

// strictly posix conforming, does not handle locales
std::bitset<LexRegex::alphabet_size> LexRegex::get_bitset_equivalence_class(const std::string &s) {
	if (s.size() == 1)
		return std::bitset<alphabet_size>().flip((unsigned char)s[0]);
	return {};
}

// strictly posix conforming, does not handle locales
std::bitset<LexRegex::alphabet_size> LexRegex::get_bitset_collating_element(const std::string &s) {
	if (s.size() == 1)
		return std::bitset<alphabet_size>().flip(s[0]);
	return {};
}

std::bitset<LexRegex::alphabet_size> LexRegex::get_bitset_range(const std::string &s1, const std::string &s2) {
	std::bitset<alphabet_size> ret;
	int c1 = (unsigned char)s1[0];
	if (c1 == '\\')
		c1 = get_escaped_val(s1);
	int c2 = (unsigned char)s2[0];
	if (c2 == '\\')
		c2 = get_escaped_val(s2);
	for (; c1 <= c2; c1++)
		ret[c1] = true;
	return ret;
}

int LexRegex::get_escaped_val(const std::string &s) {
	if (s[1] == 'x')
	{
		int n = 0;
		for (int i = 2;s[i]; i++)
		{
			n *= 16;
			if (isdigit(s[i]))
				n += s[i] - '0';
			else if (islower(s[i]))
				n += s[i] - 'a';
			else if (isupper(s[i]))
				n += s[i] - 'A';
		}
		return n;
	}
	else if (s[1] >= '0' && s[1] <= '8')
	{
		int n = 0;
		for (int i = 1;s[i]; i++)
			n = n * 8 + s[i] - '0';
		return n;
	}
	else switch (s[1])
		{
			case '\\':
				return '\\';
			case 'a':
				return '\a';
			case 'b':
				return '\b';
			case 'f':
				return '\f';
			case 'n':
				return '\n';
			case 't':
				return '\t';
			case 'r':
				return '\r';
			case 'v':
				return '\v';
			default:
				return s[1];
		}
}

std::bitset<LexRegex::alphabet_size> LexRegex::get_bitset_bracket(const std::string &s) {
	std::bitset<LexRegex::alphabet_size> ret;
	int i = 1;
	bool invert = false;
	if (s[i] == '^')
	{
		invert = true;
		i++;
	}
	if (s[i] == ']')
	{
		ret |= get_bitset_char("]");
		i++;
	}
	const std::regex &r = regw[R"(([^]\\]|\\([\\abfnrtv]|[0-7]{1,3}|x[0-9a-fA-F]{1,2}|.))|\[:(([^:]|:[^]])*):]|\[\.(([^.]|\.[^]])*)\.]|\[=(([^=]|=[^]])*)=]|(([^]\\]|\\([\\abfnrtv]|[0-7]{1,3}|x[0-9a-fA-F]{1,2}|.))-([^]\\]|\\([\\abfnrtv]|[0-7]{1,3}|x[0-9a-fA-F]{1,2}|.))))"];
	enum {CHAR = 1, CHAR_CLASS = 3, COLLATING_SYMBOL = 5, EQUIVALENCE_CLASS = 7, RANGE = 9, RANGE_CHAR_1 = 10, RANGE_CHAR_2 = 12};
	for (std::regex_iterator it(s.begin() + i, s.end() - 1, r);it != std::regex_iterator<std::string::const_iterator>();it++)
	{
		if ((*it)[CHAR].matched)
			ret |= get_bitset_char((*it)[CHAR].str());
		else if ((*it)[CHAR_CLASS].matched)
			ret |= get_bitset_class((*it)[CHAR_CLASS]);
		else if ((*it)[COLLATING_SYMBOL].matched)
			ret |= get_bitset_collating_element((*it)[COLLATING_SYMBOL]);
		else if ((*it)[EQUIVALENCE_CLASS].matched)
			ret |= get_bitset_equivalence_class((*it)[EQUIVALENCE_CLASS]);
		else if ((*it)[RANGE].matched)
			ret |= get_bitset_range((*it)[RANGE_CHAR_1], (*it)[RANGE_CHAR_2]);
	}
	if (invert)
		ret.flip();
	return ret.set(256, false).set(257, false);
}


std::pair<int, int> LexRegex::get_single_char_dup(char c) {
	switch (c)
	{
		default:
		case '*':
			return {0, -1};
		case '+':
			return {1, -1};
		case '?':
			return {0, 1};
	}
}


std::pair<int, int> LexRegex::get_interval(const std::string & s) {

	std::match_results< std::string ::const_iterator> m;
	if (!(regw[R"(\{([0-9]+)(,([0-9]+)?)?})"](s, m)))
		throw InvalidInterval();
	std::pair<int, int> ret;
	ret.first = std::stoi(m[1].str());
	if (m[2].matched)
		ret.second = -1;
	if (m[3].matched)
		ret.second = std::stoi(m[3].str());
	if (ret.second > RE_DUP_MAX ||
		(ret.second != -1 && ret.second < ret.first))
		throw InvalidInterval();
	return ret;
}

void LexRegex::tokenize_quoted(std::deque<token> &d, const std::string &s) {
	const std::regex &r = regw[R"(([^"\\]|\\([\\abfnrtv]|[0-7]{1,3}|x[0-9a-fA-F]{1,2}|.)))"];
	for (std::regex_iterator it(s.begin(), s.end(), r);it != std::regex_iterator<std::string::const_iterator>();it++)
		d.push_front({token::TERMINAL, get_bitset_char((*it)[1].str())});
}

std::stack<LexRegex::token> LexRegex::tokenize(std::string s) {
	std::deque<token> ret;
	const std::regex &r =
	regw[R"-(([^[".()+?*{|/\\]|\\([\\abfnrtv]|[0-7]{1,3}|x[0-9a-fA-F]+|.))()|(\[\^?]?([^]]|\[:([^:]|:[^]])*:]|\[\.([^.]|\.[^]])*\.]|\[=([^=]|=[^]])*=])*])|"(([^"\\]|\\.)*)"|(\()|(\))|(\{([[:alpha:]_][[:alnum:]_]*)})|([*+?])|(\{[0-9]+(,([0-9]+)?)?})|(\|)|(/)|(\.))-"];
	enum {CHAR = 1, ESCAPED = 2, BRACKET = 4, QUOTED = 9, LPAR = 11, RPAR = 12, DEFINE = 13, DEFINE_CONTENT = 14, SCDUP = 15, INTERVAL = 16, PIPE = 19, SLASH = 20, DOT = 21};
	for (std::regex_iterator it(s.begin(), s.end(), r);it != std::regex_iterator<std::string::iterator>();it++)
	{
		auto &mr = *it;
		if (mr[CHAR].matched)
			ret.push_front({token::TERMINAL, get_bitset_char(mr[CHAR].str())});
		else if (mr[BRACKET].matched)
			ret.push_front({token::TERMINAL, get_bitset_bracket(mr[BRACKET].str())});
		else if (mr[QUOTED].matched)
			this->tokenize_quoted(ret, mr[QUOTED]);
		else if (mr[LPAR].matched)
			ret.push_front({token::LPAR, {}});
		else if (mr[RPAR].matched)
			ret.push_front({token::RPAR, {}});
		else if (mr[DEFINE].matched)
			ret.push_front({token::DEFINE, mr[DEFINE_CONTENT].str()});
		else if (mr[SCDUP].matched)
		{
			token t;
			t.type = token::SINGLE_CHAR_DUP;
			t.value.emplace<token::SINGLE_CHAR_DUP>(get_single_char_dup(mr[SCDUP].str()[0]));
			ret.push_front(t);
		}
		else if (mr[INTERVAL].matched)
		{
			token t;
			t.type = token::INTERVAL;
			t.value.emplace<token::INTERVAL>(get_interval(mr[INTERVAL].str()));
			ret.push_front(t);
		}
		else if (mr[PIPE].matched)
			ret.push_front({token::PIPE, {}});
		else if (mr[SLASH].matched)
			ret.push_front({token::SLASH, {}});
		else if (mr[DOT].matched)
			ret.push_front({token::TERMINAL, get_bitset_dot()});
		else
			throw std::exception();
	}
	return std::stack<token>(ret);
}

void LexRegex::parse(const std::string &regex) {
	std::stack<token> s(tokenize(regex));
    this->root = parse_alternation(s);
    if (!s.empty())
        unexpected(s);
}

LexRegex::node LexRegex::parse_terminal(std::stack<token> &s) {
	if (!is_of_type(s, {token::TERMINAL}))
		unexpected(s);
	node ret = {std::get<token::TERMINAL>(s.top().value)};
	s.pop();
	return ret;
}

LexRegex::node LexRegex::parse_grouping(std::stack<token> &s) {
	if (!accept(s, token::LPAR))
		return parse_terminal(s);
	node ret = parse_alternation(s);
	expect(s, token::RPAR);
	return ret;
}

LexRegex::node LexRegex::parse_def(std::stack<token> &s) {
	if (!is_of_type(s, {token::DEFINE}))
		return parse_grouping(s);
	std::string name = std::get<token::DEFINE>(s.top().value);
	auto it = this->defines.find(name);
	if (it == this->defines.end())
		throw InvalidDefine();

	std::stack<token> s2 = tokenize(it->second);
	node ret = parse_alternation(s2);
	s.pop();
	if (!s2.empty())
		unexpected(s2);
	return ret;
}

LexRegex::node LexRegex::parse_single_char_dup(std::stack<token> &s) {
	dup duplicate;

	duplicate.other = parse_def(s);
	if (is_of_type(s, {token::SINGLE_CHAR_DUP}))
	{
		duplicate.from = std::get<token::SINGLE_CHAR_DUP>(s.top().value).first;
		duplicate.to = std::get<token::SINGLE_CHAR_DUP>(s.top().value).second;
	}
	else
		return duplicate.other;
	node ret{node::DUP};
	ret.value.emplace<node::DUP>(new dup(duplicate));
	s.pop();
	return ret;
}

LexRegex::node LexRegex::parse_concat(std::stack<token> &s)
{
	node ret;
	ret.type = node::CONCATENATION;
	ret.value.emplace<node::CONCATENATION>();
	while(!s.empty() && !is_of_type(s, {token::PIPE, token::RPAR, token::SLASH, token::INTERVAL}))
		get<node::CONCATENATION>(ret.value).push_back(parse_single_char_dup(s));

	if (get<node::CONCATENATION>(ret.value).size() == 1)
		return get<node::CONCATENATION>(ret.value).front();
	return ret;
}

LexRegex::node LexRegex::parse_interval(std::stack<token> &s) {
	node tmp = parse_concat(s);
	if (!is_of_type(s, {token::INTERVAL}))
		return tmp;
	dup duplicate;
	duplicate.other = tmp;
	duplicate.from = std::get<token::INTERVAL>(s.top().value).first;
	duplicate.to = std::get<token::INTERVAL>(s.top().value).second;
	node ret;
	ret.type = node::DUP;
	ret.value.emplace<node::DUP>(new dup(duplicate));
	s.pop();

	return ret;
}

LexRegex::node LexRegex::parse_interval_concat(std::stack<token> &s) {
	node ret;
	ret.type = node::CONCATENATION;
	ret.value.emplace<node::CONCATENATION>();
	do
		get<node::CONCATENATION>(ret.value).push_back(parse_interval(s));
	while(!s.empty() && !is_of_type(s, {token::SLASH, token::PIPE, token::RPAR}));

	if (get<node::CONCATENATION>(ret.value).size() == 1)
		return get<node::CONCATENATION>(ret.value).front();
	return ret;
}

LexRegex::node LexRegex::parse_alternation(std::stack<token> &s) {
	node ret;
	ret.type = node::ALTERNATION;
	ret.value.emplace<node::ALTERNATION>();
	do
	        get<node::ALTERNATION>(ret.value).push_back(parse_interval_concat(s));
	while (accept(s, token::PIPE));

	if (get<node::ALTERNATION>(ret.value).size() > 1)
		return ret;
	else
		return get<node::ALTERNATION>(ret.value).front();
}


void LexRegex::unexpected(std::stack<token> &s) {
	if (s.empty())
		throw UnexpectedToken("End of regex");
	std::string tname[] =
			{
					"TERMINAL",
					"TOKEN",
					"LPAR",
					"RPAR",
					"DEFINE",
					"SINGLE_CHAR_DUP",
					"INTERVAL",
					"PIPE",
					"SLASH"
			};
	throw UnexpectedToken(tname[s.top().type]);
}

void LexRegex::expect(std::stack<token> &s, token::token_type type) {
	if (s.empty() || s.top().type != type)
		unexpected(s);
	s.pop();
}

bool LexRegex::accept(std::stack<token> &s, token::token_type type) {
	if (s.empty() || s.top().type != type)
		return false;
	s.pop();
	return true;
}

bool LexRegex::is_of_type(std::stack<token> &s, std::initializer_list<token::token_type> &&list) {
	if (s.empty())
		return false;
	for (auto i : list)
		if (i == s.top().type)
			return true;
	return false;
}

void LexRegex::compile_terminal(const node &n, nfa_type::state_id from, nfa_type::state_id to) {
	nfa_type::link(from, to, std::get<std::bitset<alphabet_size> >(n.value));
}

void LexRegex::compile_alternation(const node &n, nfa_type::state_id from, nfa_type::state_id to) {
	for (auto &alternative : get<node::ALTERNATION>(n.value))
		compile_node(alternative, from, to);
}

void LexRegex::compile_concatenation(const node &n, nfa_type::state_id from, nfa_type::state_id to) {
	for (auto &element : get<node::CONCATENATION>(n.value))
	{
		nfa_type::state_id new_state = nfa_type::new_state();
		compile_node(element, from, new_state);
		from = new_state;
	}
	nfa_type::epsilon_link(from, to);
}

void LexRegex::compile_duplication(const node &n, nfa_type::state_id from, nfa_type::state_id to) {
	dup &duplicate = *get<node::DUP>(n.value);
	for (int i = 0; i < duplicate.from; i++)
	{
		nfa_type::state_id new_state = nfa_type::new_state();
		compile_node(duplicate.other, from, new_state);
		from = new_state;
	}
	if (duplicate.to == -1) // kleene star
		compile_node(duplicate.other, from, from);
	else
		for (int i = duplicate.from; i < duplicate.to; i++)
		{
			nfa_type::state_id new_state = nfa_type::new_state();
			compile_node(duplicate.other, from, new_state);
			nfa_type::epsilon_link(from, to);
			from = new_state;
		}
	nfa_type::epsilon_link(from, to);
}

void LexRegex::compile_node(const node &n, nfa_type::state_id from, nfa_type::state_id to) {
	switch (n.type) {
		case node::TERMINAL: return compile_terminal(n, from, to);
		case node::DUP: return compile_duplication(n, from, to);
		case node::ALTERNATION: return compile_alternation(n, from, to);
		case node::CONCATENATION: return compile_concatenation(n, from, to);
		default:
			nfa_type::epsilon_link(from, to);
	}
}

void LexRegex::compile() {
	Nfa<LexRegex::alphabet_size>::set_accept(this->nfa.get_exit(), this->id);
	this->compile_node(this->root, this->nfa.get_entrance(), this->nfa.get_exit());
}