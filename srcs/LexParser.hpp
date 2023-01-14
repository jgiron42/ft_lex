#ifndef FT_LEX_LEXPARSER_HPP
# define FT_LEX_LEXPARSER_HPP
# include <string>
# include <fstream>
# include <list>
# include <map>

// this class parse the .l file
class LexParser {
private:
	enum token {
		DPERCENT,
		PERCENT_LBRACKET,
		PERCENT_RBRACKET,
		NEWLINE,

		REGEX,
		ID,
		CBLOCK,
		CLINE,
	};
	std::string							filename;
	std::ifstream						filestream;

	std::string							header_content;
	std::map<std::string, std::string>	defines;
	bool			is_yytext_an_array;
public:
	LexParser(std::string filename);
private:
	void	parseDefSection();
	void	parseInline();
	void	parseDefOption(std::string &line);
	void	parseDefiniton(const std::string &line);
	void	lex();
};


#endif
