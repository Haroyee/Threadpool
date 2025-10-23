#设置编译器c用gcc，cpp用g++
CC:=g++
#-Wall开启警告 -I指定头文件路径 -O优化 -std=c++11指定c++标准版本
CFLAGS:=-Wall -Iinclude -std=c++11
#链接选项（如 `-L` 指定库路径）
LDFLAGS:= -Llib
#链接线程库
LDLIBS:= -lpthread

#源文件文件夹
SRC_DIR := src
#可执行文件路径
OUTPUT_DIR:= output
#目标文件路径
BUILD_DIR:= build
#生成的可执行文件名称
EXE_NAME:= main.exe
#源文件名称
SRC := $(wildcard $(SRC_DIR)/*.cpp)
#目标文件名称,.o文件不一定存在，所以用递归展开
OBJ := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC)) #OBJ := $(SRC:.cpp=.o)


#指定默认目标
all: $(OUTPUT_DIR)/$(EXE_NAME)
#链接生成可执行文件 依赖.o文件,目标文件夹，执行文件文件夹
$(OUTPUT_DIR)/$(EXE_NAME): $(OBJ) $(BUILD_DIR) $(OUTPUT_DIR)
	$(CC) $(OBJ) -o $@ $(LDFLAGS) $(LDLIBS)
#编译生成目标文件 依赖.cpp文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(SRC) $(BUILD_DIR)
	$(CC) -c $< -o $@ $(CFLAGS)


ifeq ($(OS), Windows_NT) # Windows 系统
clean:
	if exist $(BUILD_DIR)\*.o del /f /q $(BUILD_DIR)\*.o
	if exist $(OUTPUT_DIR)\$(EXE_NAME) del /f /q $(OUTPUT_DIR)\$(EXE_NAME)
$(BUILD_DIR):
	if not exist $@ mkdir $@
$(OUTPUT_DIR):
	if not exist $@ mkdir $@
run: all
	chcp 65001
	./$(OUTPUT_DIR)/$(EXE_NAME)
else # Linux/macOS 或其他类 Unix 系统
clean:
	@if [  -d "$(BUILD_DIR)/*.o" ]; then rm -f $(BUILD_DIR)/*.o;fi
	@if [  -d "$(OUTPUT_DIR)/$(EXE_NAME)" ]; then rm -f $(OUTPUT_DIR)/$(EXE_NAME);fi
$(BUILD_DIR):
	@if [ ! -d "$@" ]; then mkdir $@; fi
$(OUTPUT_DIR):
	@if [ ! -d "$@" ]; then mkdir $@; fi
run: all
	./$(OUTPUT_DIR)/$(EXE_NAME)

endif

.PHONY:all clean run