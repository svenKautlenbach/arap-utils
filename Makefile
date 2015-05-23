all: build

arm: CC = arm-none-linux-gnueabi2013.05-g++ 
arm: all

pc: CC = g++-4.8
pc: all

CPPFLAGS = -Wall -Werror -std=gnu++11
LDFLAGS = -pthread

TEST_SOURCES = tests.cpp
LIB_SOURCES = ArapUtils.cpp ArapUtilsNetwork.cpp
SOURCES += $(TEST_SOURCES) $(LIB_SOURCES)
INCLUDES = -I.
CPPFLAGS += $(INCLUDES)

OBJ := $(SOURCES:.cpp=.o)

TARGET = arap-utils-test

build: $(TARGET)

$(TARGET): $(OBJ) 
	$(CC) $^ $(LDFLAGS) -o $@
	
%.o: %.cpp
	$(CC) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -f *.o */*.o $(TARGET)
