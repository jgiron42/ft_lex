
#ifndef FT_LEX_NFA_HPP
#define FT_LEX_NFA_HPP
#include <string>
#include <list>
#include <variant>
#include <iostream>
#include <exception>
#include <map>
#include <array>
#include <vector>
#include <set>
#include <bitset>

template <size_t alphabet_size>
class Nfa {
public:
	typedef uint32_t state_id;
	template <size_t a>
	friend class Dfa;
	struct nfa_state {
		int link; // link counter to this state
		std::array<std::set<state_id>, alphabet_size>	transitions; // should be replaced by flat_sets and maybe a stable_vector
		std::set<state_id> 								epsilon_transitions;
		int	accept; // the id of the rules accepted by this state
	};
	static std::vector<nfa_state>	all_states;
private:
	state_id						entrance;
	state_id						exit;
public:
	Nfa(){
		this->entrance = new_state();
		this->exit = new_state();
	}
	static state_id	new_state()
	{
		Nfa::all_states.emplace_back();
		Nfa::all_states.back().link = 0;
		return Nfa::all_states.size();
	}
	static void		link(state_id from, state_id to, const std::bitset<alphabet_size> &symbols)
	{
		for (size_t i = 0; i < symbols.size(); i++) {
			if (symbols[i])
				if (Nfa::get(from).transitions[i].insert(to).second)
					Nfa::get(to).link++;
		}
	}
	static void		epsilon_link(state_id from, state_id to)
	{
		if (Nfa::all_states[from - 1].epsilon_transitions.insert(to).second)
			Nfa::all_states[to - 1].link++;
	}
	static void		remove_epsilon()
	{
		for (size_t i = 0; i < all_states.size(); i++)
		{
			auto &current = all_states[i];
//			std::cout << i << " -> " << current.epsilon_transitions.size() << std::endl;
			while (!current.epsilon_transitions.empty())
			{
				state_id child_id = *current.epsilon_transitions.begin();
				current.epsilon_transitions.erase(child_id);
				nfa_state &child = get(child_id);
				if (child_id != i + 1) // if child is different from current
				{
					if (!current.accept)
						current.accept = child.accept;
					for (size_t symbol = 0; symbol < alphabet_size; symbol++)
						current.transitions[symbol].insert(child.transitions[symbol].begin(),
														   child.transitions[symbol].end());
					current.epsilon_transitions.insert(child.epsilon_transitions.begin(),
													   child.epsilon_transitions.end());
				}
				child.link--;
			}
		}
	}
	state_id	get_entrance()
	{
		return this->entrance;
	}
	state_id	get_exit()
	{
		return this->exit;
	}
	static void	set_accept(state_id id, int regex_id)
	{
		all_states[id - 1].accept = regex_id;
//		all_states[id - 1].accept.insert(regex_id);
	}
//private:
	static nfa_state	&get(state_id id)
	{
		return all_states[id - 1];
	}
};


template <size_t alphabet_size>
std::vector<typename Nfa<alphabet_size>::nfa_state> Nfa<alphabet_size>::all_states;

#endif //FT_LEX_NFA_HPP
