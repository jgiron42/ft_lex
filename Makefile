NAME = ft_lex

SRCS =	main.cpp \
		LexConfig.cpp \
		RegexWrapper.cpp \
		LexRegex.cpp \
		Generator.cpp

SRCS_DIR = srcs

OBJS_DIR = .objs

INCLUDE_DIR = includes

CXXFLAGS = -Wall -Werror -Wextra -g3 -std=c++20 -D SKELETONS_PATHS=\"$(shell pwd)/skeletons\"

LDFLAGS =

include template_cpp.mk
