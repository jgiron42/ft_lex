
#ifndef FT_LEX_DFA_HPP
#define FT_LEX_DFA_HPP
#include <string>
#include <list>
#include <queue>
#include <variant>
#include <exception>
#include <map>
#include <array>
#include <vector>
#include <set>
#include <bitset>
#include <iostream>
#include "Nfa.hpp"

template <size_t alphabet_size>
class Dfa {
	typedef Nfa<alphabet_size> nfa_type;
public:
	typedef uint32_t state_id;
	struct dfa_state {
		int									link; // link counter to this state
		std::array<state_id, alphabet_size>	transitions; // should be replaced by flat_sets and maybe a stable_vector
		int 								accept;
	};
	static std::vector<dfa_state>										all_states;
private:
	static std::map<std::set<typename nfa_type::state_id>, state_id>	m; // could be a flat_map
	static std::queue<typename decltype(m)::iterator>					new_elements;
public:
	static state_id	new_state()
	{
		Dfa::all_states.emplace_back();
		Dfa::all_states.back().link = 0;
		return Dfa::all_states.size();
	}

	static void		add_from_nfa()
	{
		for (state_id i = 1; i < nfa_type::all_states.size(); i++)
			(void)get_subset(std::set<typename nfa_type::state_id>{i});
	}

	static void		subset_construction()
	{
		while (!new_elements.empty())
		{
			auto &p = *new_elements.front();
			new_elements.pop();
			for (auto &s : p.first)
			{
//				std::cout << p.second <<" " << s << " " << get(p.second).accept << " " << nfa_type::get(s).accept << std::endl;
				if (!get(p.second).accept)
					get(p.second).accept = nfa_type::get(s).accept;
			}
			for (size_t symbol = 0; symbol < alphabet_size; symbol++)
			{
				std::set<typename nfa_type::state_id> targets;
				for (auto &s : p.first)
				{
//					std::cout << s << "[" << symbol << "]: " <<  nfa_type::get(s).transitions[symbol].size() << std::endl;
					targets.insert(nfa_type::get(s).transitions[symbol].begin(), nfa_type::get(s).transitions[symbol].end());
				}
				if (targets.empty())
					continue;
//				std::cout << "targets: ";
//				for (auto a : targets)
//					std::cout << a << " ";
//				std::cout << std::endl;
				link(p.second, get_subset(targets), symbol);
			}
		}
	}

	static void		link(state_id from, state_id to, const int symbol)
	{
		get(from).transitions[symbol] = to;
		get(from).link++;
	}
	static void		epsilon_link(state_id from, state_id to)
	{
		if (Dfa::all_states[from - 1].epsilon_transitions.insert(to).second)
			Dfa::all_states[to - 1].link++;
	}
private:
	static dfa_state	&get(state_id id)
	{
		return all_states[id - 1];
	}
	static state_id		get_subset(const std::set<typename nfa_type::state_id> &s)
	{
		typename decltype(m)::iterator it = m.find(s);
		if (it == m.end())
			new_elements.push(it = m.insert({s, new_state()}).first);
		return it->second;
	}
};


template <size_t alphabet_size>
std::vector<typename Dfa<alphabet_size>::dfa_state> Dfa<alphabet_size>::all_states;
template <size_t alphabet_size>
std::map<std::set<typename Nfa<alphabet_size>::state_id>, typename Dfa<alphabet_size>::state_id> Dfa<alphabet_size>::m;
template <size_t alphabet_size>
std::queue<typename decltype(Dfa<alphabet_size>::m)::iterator> Dfa<alphabet_size>::new_elements;

#endif //FT_LEX_DFA_HPP
