EXTENSION_FILE = cpp
BIN_DIR = bin
SRC_DIR = src
INCLUDE_DIR = include
TARGET = App

ifdef OS
	RM = del /Q
else
	ifeq ($(shell uname), Linux)
		RM = rm -f
	endif
endif

ifeq ($(EXTENSION_FILE),cpp)
	COMPILER = g++
else
	COMPILER = gcc
endif
