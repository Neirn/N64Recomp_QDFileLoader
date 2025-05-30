ZIG = zig c++
TARGET = yazmt_qdfileloader
CXXFLAGS = -shared -fPIC -I ./include

all: linux windows macos

linux:
	$(ZIG) -target x86_64-linux-gnu $(CXXFLAGS) -ldl -o $(TARGET).so ./src/*

windows:
	$(ZIG) -target x86_64-windows $(CXXFLAGS) -s -o $(TARGET).dll ./src/*

macos:
	$(ZIG) -target aarch64-macos $(CXXFLAGS) -o $(TARGET).dylib ./src/*

.PHONY: all linux windows macos
