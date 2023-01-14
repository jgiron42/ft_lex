NAME = scop

SRCS =	main.cpp \
		LexParser.cpp \
		RegexWrapper.cpp

SRCS_DIR = srcs

OBJS_DIR = .objs

INCLUDE_DIR = includes

CXXFLAGS = -Wall -Werror -Wextra -g3 -std=c++20

LDFLAGS =

include template_cpp.mk
