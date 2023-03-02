#include "RegexWrapper.hpp"

RegexWrapper regw;

RegexWrapper::reg_match_function_object RegexWrapper::operator[](const RegexWrapper::key_type &k) {
	auto it = this->cache.find(k);
	if (it == this->cache.end())
	{
		this->cache[k] = std::regex(get<0>(k), get<1>(k));
		return (this->cache[k]);
	}
	return it->second;
}

RegexWrapper::reg_match_function_object RegexWrapper::operator[](const std::string &s) {
	return (*this)[key_type{s, this->default_type}];
}
