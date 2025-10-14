# Makefile to compile and run the MIPS assembler

# Compiler
CXX = g++

# Compiler flags
# -std=c++11: Use the C++11 standard
# -Wall: Enable all warnings
# -o: Specify the output executable name
CXXFLAGS = -std=c++11 -Wall -o

# Target executable name
TARGET = assembler

# Source file
SOURCE = assembler.cpp

# Default target, called when you just run `make`
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) $(TARGET) $(SOURCE)
	@echo "Assembler compiled successfully. Run with: ./$(TARGET) <dir containing main.asm>"

# Rule to clean up generated files
clear:clean
	clear
clean:
	rm -f $(TARGET)

# Phony targets are not real files
.PHONY: all run clean clear
