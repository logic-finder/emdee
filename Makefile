### SETTINGS ###
srcdir := src
objdir := $(srcdir)/.object
depdir := $(srcdir)/.depend
include_path := $(srcdir)/header

exec := emdee
sources := $(wildcard $(srcdir)/*.c)
objects := $(sources:$(srcdir)/%.c=$(objdir)/%.o)

SHELL := /bin/sh
CC := gcc
# CFLAGS := -O -Wall -W -pedantic -g
CFLAGS := -g
CPPFLAGS := -I $(include_path)

.SUFFIXES:					# Clear the suffix list.
.SUFFIXES: .c .o .h		# Redefine.

### Colored Strings ###
define cstr
"\033[$1m$2\033[0m"
endef

name_str = $(call cstr,38;5;49;1,$1)
build_completed_str = $(call cstr,48;5;222;30, [ BUILD COMPLETED ] )
notice_str = $(call cstr,48;5;150;30, [ NOTICE ] )
marker_str = $(call cstr,48;5;222;30, > )

### COMPILATION ###
# This is the default goal.
$(exec): $(objects)
	$(CC) $^ $(CFLAGS) -o $@
	@echo "> the executable "$(call name_str,$@)" has been created."

# $$$$ expands to $$, which means "the process ID of the shell." 
$(depdir)/%.d: $(srcdir)/%.c
	@set -e; rm -f $@; \
	 $(CC) $< $(CPPFLAGS) -MM > $@.$$$$; \
	 printf "%s%s" "$(objdir)/" "$$(cat $@.$$$$)" > $@; \
	 rm -f $@.$$$$

# .d file contains a list of files on which the .o file depends.
# .d file looks like: \
	src/.object/filename.o:   src/filename.c   src/header/dependency.h
include $(sources:$(srcdir)/%.c=$(depdir)/%.d)

# This rule provides the recipe for making .o files.
# $< is the first prereq., mentioned in the %.d file, which is the %.c file.
$(objdir)/%.o:
	$(CC) $< $(CPPFLAGS) $(CFLAGS) -c -o $@
	@echo "> "$(call name_str,$@) has been created.

### Miscellaneous Tasks ###
.PHONY: clean
clean:
	rm -f $(exec)
	rm -f $(depdir)/*.d $(objdir)/*.o

.PHONY: test subt
testcase_full := full.test
testcase_part := part.test
Execute.test := ./$(exec) $(testcase_full)
Execute.subtest := ./$(exec) $(testcase_part)

test:
	$(Execute.test)

subt:
	$(Execute.subtest)

.PHONY: help
cmd_color = $(call cstr,38;5;220,$1)
help:
	@echo The below are the list of available commands from this Makefile.
	@echo "	make		builds this program."
	@echo "	make "$(call cmd_color,$(exec))"	is the alias of 'make'."
	@echo "	make "$(call cmd_color,clean)"	deletes '$(exec)', '$(depdir)/*.d' and '$(objdir)/*.o'."
	@echo "	make "$(call cmd_color,test)"	runs '$(Execute.test)'."
	@echo "	make "$(call cmd_color,subt)"	runs '$(Execute.subtest)'."
	@echo "	make "$(call cmd_color,help)"	prints this long manual on the screen that you are reading now."