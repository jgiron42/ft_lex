#include "Nfa.hpp"
#include "RegexWrapper.hpp"
#include "LexParser.hpp"
#include <utility>
#include <regex.h>

Nfa::Nfa(std::map<std::string, std::string>	&def, std::string regex) :str(std::move(regex)), defines(def) {}

void Nfa::lex() {
	for (size_t i = 0; i < this->str.size(); i++)
	{
		 switch (this->str[i])
		 {
			 case '+':
				 this->token_list.push_back({token_type::PLUS, 0});
				 break;
			 case '?':
				 this->token_list.push_back({token_type::QMARK, 0});
				 break;
			 case '*':
				 this->token_list.push_back({token_type::STAR, 0});
				 break;
			 case '|':
				 this->token_list.push_back({token_type::PIPE, 0});
				 break;
			 case '(':
				 this->token_list.push_back({token_type::LPARENTHESIS, 0});
				 break;
			 case ')':
				 this->token_list.push_back({token_type::RPARENTHESIS, 0});
				 break;
			 case '/':
				 this->token_list.push_back({token_type::SLASH, 0});
				 break;
			 case '{':
			 {
				 std::match_results<decltype(str.begin())> m;
				 if (std::regex_match(str.begin() + i, str.end(), m, std::regex(regw[R"(\{([0-9]+)(,([0-9]+))?\})"])))
				 {
					 long int a, b;
					 a = strtol(m[1].str().data(), NULL, 10);
					 if (a > RE_DUP_MAX)
						 throw too_complicated_exception();
					 if (m[3].matched)
					 {
						 b = strtol(m[1].str().data(), NULL, 10);
						 if (b > RE_DUP_MAX)
							 throw too_complicated_exception();
						 if (b < a)
							 throw bad_iteration();
					 }
					 else if (m[2].matched)
						 b = -1;
					 else
						 b = a;
					 this->token_list.push_back(
							 (token){
								 .type = BRACKET_EXPRESSION,
								 .value = std::make_pair(a, b)
							 });
					 i += m.length();
				 }
				 else if (std::regex_match(str.begin() + i, str.end(), std::regex(regw["\\{(" REG_C_ID ")\\}"])))
				 {
					 auto it = this->defines.find(m[1]);
					 if (it == this->defines.end())
						 throw invalid_define();
					 str.erase(m.position(), m.length());
					 str.insert(m.position(), it->second);
					 i--;
				 }
				 else
					 throw invalid_regex();
				 break;
			 }
			 case '"':
			 {
				 std::match_results<decltype(str.begin())> m;
				 if (!std::regex_match(str.begin() + i, str.end(), m, std::regex(regw[R"<>("(\\([\abfnrtv]|[0-7]{1,3}|x[[:xdigit:]]+)|[^\"])")<>"])))
					 throw invalid_regex();
				 this->token_list.push_back({.type = LITERAL, .value = m[1]});
				 break;
				 auto it = std::regex_token_iterator(m[1].first, m[1].second, (const std::regex&)regw[R"<>("(\\([\abfnrtv]|[0-7]{1,3}|x[[:xdigit:]]+)|[^\"])")<>"],1);
				 while (it != decltype(it)())
				 {
					 if (*it->first != '\\')
						 this->token_list.push_back({.type = LITERAL, .value = *it->first});
					 switch(it->first[1])
					 {
						 case '\\':
							 this->token_list.push_back({.type = LITERAL, .value = '\\'});
							 break;
						 case 'a':
							 this->token_list.push_back({.type = LITERAL, .value = '\a'});
							 break;
						 case 'b':
							 this->token_list.push_back({.type = LITERAL, .value = '\b'});
							 break;
						 case 'f':
							 this->token_list.push_back({.type = LITERAL, .value = '\f'});
							 break;
						 case 'n':
							 this->token_list.push_back({.type = LITERAL, .value = '\n'});
							 break;
						 case 'r':
							 this->token_list.push_back({.type = LITERAL, .value = '\r'});
							 break;
						 case 't':
							 this->token_list.push_back({.type = LITERAL, .value = '\t'});
							 break;
						 case 'v':
							 this->token_list.push_back({.type = LITERAL, .value = '\v'});
							 break;
						 case 'x':
							 this->token_list.push_back({.type = LITERAL, .value = it->first[2] * 16 + it->first[3]});
							 break;
						 default:
							 std::string tmp(it->first + 2, it->second);
							 long val = strtol(tmp.data(), nullptr, 8);
							 if (val >= 1 << 8)
								 throw invalid_regex();
							 this->token_list.push_back({.type = LITERAL, .value = (char)val});
							 break;
					 }
				 }
				 break;
			 }
			 case '[': {
				 std::match_results<decltype(str.begin())> m;
				 if (!std::regex_match(str.begin() + i, str.end(), m,
									   std::regex(regw[R"<>(\[\^?\]?(\\([\abfnrtv]|[0-7]{1,3}|x[[:xdigit:]]+)|[^]\])\])<>"])))
					 throw invalid_regex();
				 this->token_list.push_back({.type = BRACKET_EXPRESSION, .value = m[1]});
				 break;
			 }
			 case '\\':
			 {
				 std::match_results<decltype(str.begin())> m;
				 if (!std::regex_match(str.begin() + i, str.end(), m,
									   std::regex(regw[R"<>(\\([\abfnrtv]|[0-7]{1,3}|x[[:xdigit:]]+))<>"])))
					 throw invalid_regex();
				 this->token_list.push_back({.type = LITERAL, .value = m[0]});
				 break;
			 }
		 }
	}
}