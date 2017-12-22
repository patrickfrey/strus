#include <algorithm>
#include <regex>
#include <iostream>
#include <stdexcept>

#define REGEX_SYNTAX std::regex_constants::extended
#define MATCH_FLAGS std::regex_constants::match_default

bool test_match( const std::string& expr, const std::string& line)
{
	std::match_results<char const*> res;
	std::cerr << "search " << expr << " in " << line << std::endl;
	char const* si = line.c_str();
	char const* se = si + line.size();
	return (std::regex_search( si, se, res, std::regex( expr, std::regex_constants::extended), std::regex_constants::match_default));
}

int main()
{
	bool test = false;
	try
	{
		if (!test_match( "[Aa]", "Abcdes")) return -1;
		if (!test_match( "[Aa]+", "aAbcdes")) return -1;
		if (!test_match( "[Aa]+", "xAbcdes")) return -1;
		if (!test_match( "[Aa]+", "xAaabcdes")) return -1;
		if (!test_match( "b([Aa]+)", "xbAaabcdes")) return -1;
		if (!test_match( "b([Aa]+)b", "xbAaabcdes")) return -1;
		if (!test_match( "([Aa]+)b", "xbAaabcdes")) return -1;
		return 0;
	}
	catch (const std::regex_error& err)
	{
		const char* errstr = 0;
		if (err.code() == std::regex_constants::error_collate) errstr = "The expression contained an invalid collating element name";
		else if (err.code() == std::regex_constants::error_ctype) errstr = "The expression contained an invalid character class name";
		else if (err.code() == std::regex_constants::error_escape) errstr = "The expression contained an invalid escaped character, or a trailing escape";
		else if (err.code() == std::regex_constants::error_backref) errstr = "The expression contained an invalid back reference";
		else if (err.code() == std::regex_constants::error_brack) errstr = "The expression contained mismatched brackets ([ and ])";
		else if (err.code() == std::regex_constants::error_paren) errstr = "The expression contained mismatched parentheses (( and ))";
		else if (err.code() == std::regex_constants::error_brace) errstr = "The expression contained mismatched braces ({ and })";
		else if (err.code() == std::regex_constants::error_badbrace) errstr = "The expression contained an invalid range between braces ({ and })";
		else if (err.code() == std::regex_constants::error_range) errstr = "The expression contained an invalid character range";
		else if (err.code() == std::regex_constants::error_space) errstr = "There was insufficient memory to convert the expression into a finite state machine";
		else if (err.code() == std::regex_constants::error_badrepeat) errstr = "The expression contained a repeat specifier (one of *?+{) that was not preceded by a valid regular expression";
		else if (err.code() == std::regex_constants::error_complexity) errstr = "The complexity of an attempted match against a regular expression exceeded a pre-set level";
		else if (err.code() == std::regex_constants::error_stack) errstr = "There was insufficient memory to determine whether the regular expression could match the specified character sequence";
		std::cerr << "ERROR " << (errstr ? errstr : "unknown") << std::endl;
		return 1;
	}
	catch (...)
	{
		return -1;
	}
}

