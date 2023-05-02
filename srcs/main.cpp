#include <iostream>
#include <cstring>
#include "LexConfig.hpp"

void print_exception(const std::exception& e, int level = 0) // stolen from cppreference
{
	std::cerr << std::string(level, ' ') << "error: " << e.what() << '\n';
	try {
		std::rethrow_if_nested(e);
	} catch(const std::exception& nestedException) {
		print_exception(nestedException, level+1);
	} catch(...) {}
}

void	add_entry(LexConfig &config, LexRegex &reg, const std::string& start_condition)
{
	LexConfig::nfa_type::epsilon_link(config.states[start_condition].nfa_state, reg.nfa.get_entrance());
}

void	compile_nfa(LexConfig &config)
{
	for (auto &r : config.rules)
	{
		r.regex.compile();
		if (r.start_conditions.empty())
			for (auto &s : config.states)
				if (!s.first.starts_with("YY_") && !s.second.exclusive)
				{
					add_entry(config, r.regex, "YY_BOL_" + s.first);
					if (!r.bol)
						add_entry(config, r.regex, s.first);
				}
		for (auto &name : r.start_conditions)
		{
			if (name.starts_with("YY_"))
			{
				add_entry(config, r.regex, name);
				continue;
			}
			if (!config.states.count(name))
				throw LexConfig::InvalidStartCondition(name);
			add_entry(config, r.regex, "YY_BOL_" + name);
			if (!r.bol)
				add_entry(config, r.regex, name);
		}
	}
}

void	compile_dfa()
{
	// init dfa states
	LexConfig::nfa_type::remove_epsilon();
	LexConfig::dfa_type::add_from_nfa();
	LexConfig::dfa_type::subset_construction();

}

void	print_stats(LexConfig &config, std::ostream &out)
{
	out << "stats for ft_lex:" << std::endl;
	out << "  " << LexConfig::nfa_type::all_states.size() << " NFA states" << std::endl;
	out << "  " << LexConfig::dfa_type::all_states.size() << " DFA states" << std::endl;
	out << "  " << config.rules.size() << " rules ("
	<< std::count_if(config.rules.begin(), config.rules.end(), [](const LexConfig::rule &r){return !r.user_rule;})
	<< " generated)" << std::endl;
	out << "  " << config.states.size() << " start conditions ("
	<< std::count_if(config.states.begin(), config.states.end(), [](const std::pair<std::string, LexConfig::start_condition> &p){return p.first.starts_with("YY_");})
	<< " generated)" << std::endl;
}

int main(int argc, char **argv)
{
	std::string output_file = "lex.yy.c";
	std::list<std::string> input_files;
	bool stats = false;
	std::ostream *stat_output = &std::cout;
	int ret;
	while ((ret = getopt(argc, argv, "-tnvo:")) != -1)
		switch (ret)
		{
			case 't':
				output_file = "/dev/stdout";
				stat_output = &std::cerr;
				break;
			case 'n':
				stats = false;
				break;
			case 'v':
				stats = true;
				break;
			case 'o':
				output_file = optarg;
				break;
			case 1:
				input_files.emplace_back(optarg);
				break;
			default:
			case '?':
				return 1;
		}
	if (input_files.empty())
	{
		std::cerr << "No file specified" << std::endl;
		return 1;
	}
	try{
		LexConfig config(input_files);
		compile_nfa(config);
		compile_dfa();
		std::ofstream out;
		out.open(output_file);
		if (!out.is_open())
			throw std::runtime_error("Can't open " + output_file + ": " + strerror(errno));
		config.serialize(out);
		if (stats)
			print_stats(config, *stat_output);
	}
	catch (std::exception &e)
	{
		print_exception(e);
		return 1;
	}
}