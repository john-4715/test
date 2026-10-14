// addr_resolver.h - 把 --addr-list 导出的地址清单换算成函数名（主机上使用）
#pragma once

#include "options.h"

// 读取 opt.resolveAddrs 指定的地址清单，用 opt.symfs 下的未 strip 文件查函数名，
// 输出按地址和按函数汇总的两张表。返回进程退出码。
int resolveAddrList(const Options& opt);
