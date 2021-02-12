# Makefile for the Probabilistic Language Parser project
# Created by Lucas Elbert 30/12/2020
#
# $@ = target file
# $^ = space separated list of all prerequisites
# $< = first prerequisite
# -MMD = compiler option to tell the preprocessor (header text replacer) to create dependency files (.d) that list for each .cpp file its dependencies, including .h files. These dependency rules are included by '-include $(DEPS)'. This is needed here as we have template code in header files.

CC = g++
CFLAGS = -Wall -std=c++17 -O3

SRCS = $(wildcard *.cpp)
OBJS = $(addprefix $(DIR)/, $(SRCS:.cpp=.o))
DEPS = $(addprefix $(DIR)/, $(SRCS:.cpp=.d))
DIR = make

all: main

# create the directory to store .o and .d files
$(DIR):
	mkdir -p $@

# main rule
main: $(OBJS) | $(DIR)
	$(CC) $(CFLAGS) -o $@ $^

# compile each .cpp into a .o
$(DIR)/%.o: %.cpp | $(DIR)
	$(CC) $(CFLAGS) -c -MMD -o $@ -c $<

# include automatically generated dependency files
-include $(DEPS)

# PHONY: neither check nor create any files in that rule
.PHONY: clean
clean:
	rm -r $(DIR)
