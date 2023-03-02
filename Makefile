NAME = ft_lex

SRCS =	main.cpp \
		LexParser.cpp \
		RegexWrapper.cpp \
		Nfa.cpp

SRCS_DIR = srcs

OBJS_DIR = .objs

INCLUDE_DIR = includes

CXXFLAGS = -Wall -Werror -Wextra -g3 -std=c++20

LDFLAGS =

include template_cpp.mk
