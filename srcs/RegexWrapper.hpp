// CURSED SFINAE IN THIS FILE, DO NOT LOOK!
#ifndef FT_LEX_REGEXWRAPPER_HPP
#define FT_LEX_REGEXWRAPPER_HPP
#include <tuple>
#include <string>
#include <regex>
#include <map>

template <typename F, size_t... Is>
auto indices_impl(F f, std::index_sequence<Is...>) {
	return f(std::integral_constant<size_t, Is>()...);
}

template <size_t N, typename F>
auto indices(F f) {
	return indices_impl(f, std::make_index_sequence<N>());
}

template <typename F, typename... Ts>
auto drop_last(F f, Ts... ts) {
	return indices<sizeof...(Ts)-1>([&](auto... Is){
		auto tuple = std::make_tuple(ts...);
		return f(std::get<Is>(tuple)...);
	});
}


class RegexWrapper {
private:
	typedef std::tuple<std::string, std::regex::flag_type> key_type;
	const std::regex::flag_type default_type = std::regex_constants::extended;
	std::map<key_type, std::regex> cache;
	class reg_match_function_object {
		const std::regex &regex;
	public:
		reg_match_function_object(const std::regex &r) : regex(r) {}
		template <typename... ArgsTypes, std::enable_if_t<(std::is_same_v<std::regex_constants::match_flag_type, ArgsTypes> || ...), bool> = true >
		bool operator()(ArgsTypes... args) {
			std::tuple<ArgsTypes...> t{args...};
			return drop_last([&](auto... elems){
				return std::regex_match(elems..., this->regex, std::get<std::regex_constants::match_flag_type>(t));
			}, args...);
		}
		template <typename... ArgsTypes, std::enable_if_t<!(std::is_same_v<std::regex_constants::match_flag_type, ArgsTypes> || ...), bool> = true >
		bool operator()(ArgsTypes... args) {
			return std::regex_match(args..., this->regex, std::regex_constants::match_default);
		}
	};
public:
	RegexWrapper::reg_match_function_object operator[](const std::string &);
	RegexWrapper::reg_match_function_object operator[](const key_type &);
};

extern RegexWrapper regw;

#endif //FT_LEX_REGEXWRAPPER_HPP
