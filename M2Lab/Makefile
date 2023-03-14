# 定义参数， := 表示常量，不能在用 = 或 += 修改变量的值
TARGET = test 
OBJ = test.o co.o
CC := gcc
CFLAGS = -g -Wall


# 格式：
# 目标文件 : 依赖文件
#   指令
#
# 说明：第一个目标文件就我的最终目标。递归执行子目标
# $(TARGET):$(OBJ)
# # $^: 表示依赖项;   $@：表示目标项
#     $(CC) $(CFLAGS) test.c co.c -o test

# %.o:%.c
#     $(CC) $(CFLAGS) -c $^ -o $@

# # 伪目标: .PHONY: 只没有依赖文件的目标
# .PHONY:
# clean: 
#     rm -rf $(TARGET) $(OBJ)
all:
	gcc test.c co.c -o test -g