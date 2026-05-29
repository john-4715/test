CC=arm-buildroot-linux-gnueabihf-g++
#CC=g++
# 设置动态库后缀
SONAME = libTest.so
# 设置编译和链接标志
CFLAGS =  -O0 -fPIC -shared
LDFLAGS = -L../libjson -rdynamic -funwind-tables -ffunction-sections -finstrument-functions -no-pie -lm -ldl -lrt -shared -lJsonObjects
IFLAGS=-I../libjson 
# 设置目标库名称
TARGET = $(SONAME)

# 设置源文件
SRCS = test.c
# 设置目标对象文件
OBJS = ${SRCS:.c=.o}

# 默认目标是库
all: ${TARGET}

# 规则来创建库
${TARGET}: ${OBJS}
	${CC} -o $@ $(IFLAGS) ${LDFLAGS} ${OBJS} -lJsonObjects

# 规则来编译每个源文件
.cpp.o:
	${CC} -c $(IFLAGS) ${CFLAGS} $< -o $@

# 清理规则
clean:
	rm -f ${TARGET} ${OBJS} 