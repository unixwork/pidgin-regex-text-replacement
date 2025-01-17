#
# pidgin-regex-text-replacement
#
# Copyright (C) 2025 Olaf Wintermann
# 
# This program is free software: you can redistribute it and/or modify  
# it under the terms of the GNU General Public License as published by  
# the Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful, but 
# WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License 
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

CC = cc

PLUGIN_CFLAGS = -fPIC `pkg-config --cflags pidgin`
PLUGIN_LDFLAGS = `pkg-config --libs pidgin`


PLUGIN_LIB = regex-text-replacement.so
BUILD_RESULT = build/$(PLUGIN_LIB)
TESTBIN = build/plugin-test

OBJ = build/regex-text-replacement.o build/ui.o

TEST_OBJ = build/test.o

all: build $(BUILD_RESULT) $(TESTBIN)

build:
	mkdir -p build

$(BUILD_RESULT): $(OBJ) 
	$(CC) -o $(BUILD_RESULT) -shared $(OBJ)

$(TESTBIN): $(OBJ) $(TEST_OBJ) 
	$(CC) -o $@ $(OBJ) $(TEST_OBJ) $(LDFLAGS) $(PLUGIN_LDFLAGS)

build/regex-text-replacement.o: regex-text-replacement.c regex-text-replacement.h 
	$(CC) -c -o $@ $< $(CFLAGS) $(PLUGIN_CFLAGS)
	
build/ui.o: ui.c ui.h regex-text-replacement.h 
	$(CC) -c -o $@ $< $(CFLAGS) $(PLUGIN_CFLAGS)

build/test.o: test.c test.h regex-text-replacement.h cx/test.h cx/common.h
	$(CC) -c -o $@ $< $(CFLAGS) $(PLUGIN_CFLAGS)

clean:
	rm -Rf build

install: $(BUILD_RESULT)
	rm -Rf ~/.purple/plugin/$(PLUGIN_LIB)
	cp $(BUILD_RESULT) ~/.purple/plugins/

