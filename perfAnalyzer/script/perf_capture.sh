#!/bin/sh
# perf_capture.sh - 在开发板上采集 perf 数据（兼容 BusyBox sh）
#
# 每次采集生成一个目录 <输出目录>/<进程名>_<日期时间>/，包含：
#   perf.data      perf 原始数据
#   out.perf.gz    perf script 文本（含内存映射，可拷到主机用 perf_analyzer --symfs 完整分析）
#   kallsyms       内核符号表
#   maps.txt       进程内存映射快照（备用，配合 perf_analyzer --maps）
#   info.txt       采集信息：时间、内核、perf 版本、程序 md5、采集命令、样本统计
#   report.txt     perf_analyzer 分析报告（板子上有 perf_analyzer 时）
#   addrs.txt      地址清单（拷到主机用 --resolve-addrs 换算函数名）
#   out.folded     折叠栈（拷到主机用 flamegraph.pl 画火焰图）
#
# 用法示例：
#   ./perf_capture.sh                                  # 采集 ApplicationServices 120 秒
#   ./perf_capture.sh ApplicationServices 60 199       # 按位置传参：进程名 采样秒数 采样频率
#   ./perf_capture.sh -p DataServer -t 60              # 指定进程名和时长
#   ./perf_capture.sh -n 24 -i 3600 -t 120 -k 12       # 每小时采 2 分钟，共 24 次，只保留最近 12 次
#   ./perf_capture.sh -a -t 30                         # 全系统采集 30 秒
#   ./perf_capture.sh -S -t 600                        # 只用 perf stat 计数 10 分钟（几乎无开销）
#   ./perf_capture.sh -z                               # 采集后打包成 tar.gz，方便拷贝

set -u

# ---------------------------------------------------------------------------
# 默认参数
# ---------------------------------------------------------------------------
PROC="ApplicationServices"   # 进程名
DURATION=120                 # 每次采集秒数
FREQ=199                     # 采样频率
MMAP_PAGES=64                # 环形缓冲区页数
OUTDIR="/data/perfwork"      # 输出目录（放在持久分区，不要用 /tmp）
ROUNDS=1                     # 采集次数
INTERVAL=0                   # 多次采集时，两次开始之间的间隔秒数
KEEP=0                       # 只保留最近 N 次结果，0 表示不清理
MIN_FREE_MB=200              # 剩余空间低于该值时不采集
SYSTEM_WIDE=0                # 1 = 全系统采集（-a）
STAT_ONLY=0                  # 1 = 只用 perf stat 计数
EVENT="cpu-clock"            # 采样事件
NO_SCRIPT=0                  # 1 = 不生成 out.perf.gz
PACK=0                       # 1 = 每次采集后打包 tar.gz
PERF="${PERF:-perf}"
ANALYZER="${ANALYZER:-perf_analyzer}"

usage() {
    cat << EOF
用法: $0 [选项] [进程名 [采样秒数 [采样频率]]]

按位置传参（可选，写在选项之后）：
  进程名        要采集的进程（默认 $PROC）
  采样秒数      每次采集时长（默认 $DURATION）
  采样频率      采样频率 Hz（默认 $FREQ）
  例：$0 ApplicationServices 60 199

选项：
  -p 进程名     要采集的进程（默认 $PROC）
  -t 秒数       每次采集时长（默认 $DURATION）
  -F 频率       采样频率 Hz（默认 $FREQ）
  -m 页数       环形缓冲区页数，2 的幂（默认 $MMAP_PAGES）
  -e 事件       采样事件（默认 $EVENT）
  -o 目录       输出目录（默认 $OUTDIR）
  -n 次数       采集次数（默认 $ROUNDS）
  -i 秒数       多次采集的间隔，按开始时间计（默认 $INTERVAL）
  -k 个数       只保留最近 N 次结果（默认 0，不清理）
  -f MB         剩余空间低于该值时停止（默认 $MIN_FREE_MB）
  -a            全系统采集（不指定进程）
  -S            只用 perf stat 计数，不采样
  -N            不生成 out.perf.gz（节省时间和空间）
  -z            每次采集后打包成 tar.gz
  -h            显示帮助
环境变量: PERF=perf 命令路径，ANALYZER=perf_analyzer 命令路径
EOF
}

log()  { echo "[$(date '+%H:%M:%S')] $*"; }
warn() { echo "[$(date '+%H:%M:%S')] 警告: $*" >&2; }
die()  { echo "[$(date '+%H:%M:%S')] 错误: $*" >&2; exit 1; }

is_uint() { case "$1" in ''|*[!0-9]*) return 1 ;; *) return 0 ;; esac; }

# ---------------------------------------------------------------------------
# 参数解析
# ---------------------------------------------------------------------------
while getopts "p:t:F:m:e:o:n:i:k:f:aSNzh" opt; do
    case "$opt" in
        p) PROC="$OPTARG" ;;
        t) DURATION="$OPTARG" ;;
        F) FREQ="$OPTARG" ;;
        m) MMAP_PAGES="$OPTARG" ;;
        e) EVENT="$OPTARG" ;;
        o) OUTDIR="$OPTARG" ;;
        n) ROUNDS="$OPTARG" ;;
        i) INTERVAL="$OPTARG" ;;
        k) KEEP="$OPTARG" ;;
        f) MIN_FREE_MB="$OPTARG" ;;
        a) SYSTEM_WIDE=1 ;;
        S) STAT_ONLY=1 ;;
        N) NO_SCRIPT=1 ;;
        z) PACK=1 ;;
        h) usage; exit 0 ;;
        *) usage; exit 1 ;;
    esac
done
shift $((OPTIND - 1))

# 按位置传参：进程名 采样秒数 采样频率
[ $# -ge 1 ] && [ -n "$1" ] && PROC="$1"
[ $# -ge 2 ] && [ -n "$2" ] && DURATION="$2"
[ $# -ge 3 ] && [ -n "$3" ] && FREQ="$3"
[ $# -gt 3 ] && { echo "多余的参数: $*" >&2; usage; exit 1; }

for v in DURATION FREQ MMAP_PAGES ROUNDS INTERVAL KEEP MIN_FREE_MB; do
    eval "val=\$$v"
    is_uint "$val" || die "$v 必须是非负整数: $val"
done
[ "$DURATION" -gt 0 ] || die "采集时长必须大于 0"
[ "$ROUNDS" -gt 0 ] || die "采集次数必须大于 0"

# ---------------------------------------------------------------------------
# 环境检查
# ---------------------------------------------------------------------------
[ "$(id -u)" -eq 0 ] || die "请用 root 运行（需要设置内核参数和读取内核符号）"
command -v "$PERF" > /dev/null 2>&1 || die "找不到 perf（$PERF），请先部署 perf 或通过 PERF=路径 指定"
HAVE_ANALYZER=0
command -v "$ANALYZER" > /dev/null 2>&1 && HAVE_ANALYZER=1

mkdir -p "$OUTDIR" || die "无法创建输出目录 $OUTDIR"

free_mb() {
    # 兼容 BusyBox df：取输出最后一行的可用 KB
    df -k "$1" 2>/dev/null | awk 'NR>1 {avail=$(NF-2)} END {print int(avail/1024)}'
}

# ---------------------------------------------------------------------------
# 内核参数：采集前放开，退出时恢复
# ---------------------------------------------------------------------------
PARANOID_FILE=/proc/sys/kernel/perf_event_paranoid
KPTR_FILE=/proc/sys/kernel/kptr_restrict
OLD_PARANOID=""
OLD_KPTR=""
[ -r "$PARANOID_FILE" ] && OLD_PARANOID=$(cat "$PARANOID_FILE")
[ -r "$KPTR_FILE" ] && OLD_KPTR=$(cat "$KPTR_FILE")

restore_sysctl() {
    [ -n "$OLD_PARANOID" ] && echo "$OLD_PARANOID" > "$PARANOID_FILE" 2>/dev/null
    [ -n "$OLD_KPTR" ] && echo "$OLD_KPTR" > "$KPTR_FILE" 2>/dev/null
}

CUR_PERF_PID=""
on_exit() {
    if [ -n "$CUR_PERF_PID" ] && kill -0 "$CUR_PERF_PID" 2>/dev/null; then
        kill -INT "$CUR_PERF_PID" 2>/dev/null
        wait "$CUR_PERF_PID" 2>/dev/null
    fi
    restore_sysctl
}
trap on_exit EXIT
trap 'warn "收到中断，停止采集"; exit 130' INT TERM

# 等同于：
#   echo -1 > /proc/sys/kernel/perf_event_paranoid   允许采集所有事件和内核调用栈
#   echo 0  > /proc/sys/kernel/kptr_restrict         让 /proc/kallsyms 显示真实地址
echo -1 > "$PARANOID_FILE" 2>/dev/null || warn "无法设置 $PARANOID_FILE"
echo 0  > "$KPTR_FILE" 2>/dev/null     || warn "无法设置 $KPTR_FILE"
log "已设置 perf_event_paranoid=$(cat "$PARANOID_FILE" 2>/dev/null) kptr_restrict=$(cat "$KPTR_FILE" 2>/dev/null)（原值 ${OLD_PARANOID:-?}/${OLD_KPTR:-?}，退出时恢复）"

# ---------------------------------------------------------------------------
# 旧结果清理
# ---------------------------------------------------------------------------
list_runs() {
    # 只列出结果目录（不含打包文件），目录名带时间戳，按名字排序即按时间排序
    for d in "$OUTDIR"/"${TAG}"_*; do
        [ -d "$d" ] && echo "$d"
    done | sort
}

cleanup_old() {
    [ "$KEEP" -gt 0 ] || return 0
    total=$(list_runs | wc -l)
    del=$((total - KEEP))
    [ "$del" -gt 0 ] || return 0
    list_runs | head -n "$del" | while read -r d; do
        rm -rf "$d" "$d.tar.gz" && log "已删除旧结果 $(basename "$d")"
    done
}

# ---------------------------------------------------------------------------
# 单次采集
# ---------------------------------------------------------------------------
if [ "$SYSTEM_WIDE" -eq 1 ]; then TAG="system"; else TAG="$PROC"; fi

capture_once() {
    round="$1"
    stamp=$(date '+%Y%m%d_%H%M%S')
    dir="$OUTDIR/${TAG}_${stamp}"

    avail=$(free_mb "$OUTDIR")
    if [ -n "$avail" ] && [ "$avail" -lt "$MIN_FREE_MB" ]; then
        warn "$OUTDIR 剩余 ${avail} MB，低于 ${MIN_FREE_MB} MB，跳过本次采集"
        return 1
    fi

    # 目标进程（每次重新获取，进程重启后也能继续）
    target=""
    pid=""
    if [ "$SYSTEM_WIDE" -eq 1 ]; then
        target="-a"
    else
        pids=$(pidof "$PROC" 2>/dev/null)
        [ -n "$pids" ] || { warn "找不到进程 $PROC，跳过本次采集"; return 1; }
        pid=$(echo "$pids" | awk '{print $1}')
        [ "$(echo "$pids" | wc -w)" -gt 1 ] && warn "有多个 $PROC 进程（$pids），只采集 $pid"
        target="-p $pid"
    fi

    mkdir -p "$dir" || { warn "无法创建 $dir"; return 1; }
    log "第 $round/$ROUNDS 次：$TAG ${pid:+(pid $pid) }采集 ${DURATION} 秒 -> $dir"

    # 采集信息
    {
        echo "开始时间: $(date '+%Y-%m-%d %H:%M:%S')"
        echo "主机名:   $(hostname 2>/dev/null)"
        echo "内核:     $(uname -a)"
        echo "perf:     $($PERF --version 2>&1 | head -n 1)"
        echo "进程:     $TAG ${pid:+pid=$pid}"
        if [ -n "$pid" ] && [ -r "/proc/$pid/exe" ]; then
            exe=$(readlink "/proc/$pid/exe" 2>/dev/null)
            echo "程序:     $exe"
            [ -n "$exe" ] && echo "md5:      $(md5sum "$exe" 2>/dev/null | awk '{print $1}')"
        fi
        echo "参数:     时长=${DURATION}s 频率=${FREQ}Hz 事件=$EVENT 缓冲区=${MMAP_PAGES}页"
    } > "$dir/info.txt"

    # 内存映射快照（采集前保存，进程在采集中重启也有据可查）
    [ -n "$pid" ] && cp "/proc/$pid/maps" "$dir/maps.txt" 2>/dev/null

    if [ "$STAT_ONLY" -eq 1 ]; then
        echo "命令:     $PERF stat -e task-clock,context-switches,cpu-migrations,page-faults -I 10000 $target -- sleep $DURATION" >> "$dir/info.txt"
        # shellcheck disable=SC2086
        "$PERF" stat -e task-clock,context-switches,cpu-migrations,page-faults -I 10000 \
            $target -o "$dir/stat.txt" -- sleep "$DURATION" &
        CUR_PERF_PID=$!
        wait "$CUR_PERF_PID"
        CUR_PERF_PID=""
        log "计数结果: $dir/stat.txt"
    else
        echo "命令:     $PERF record -e $EVENT -F $FREQ -m $MMAP_PAGES --call-graph fp --proc-map-timeout 10000 $target -o perf.data -- sleep $DURATION" >> "$dir/info.txt"
        # shellcheck disable=SC2086
        "$PERF" record -e "$EVENT" -F "$FREQ" -m "$MMAP_PAGES" --call-graph fp \
            --proc-map-timeout 10000 $target -o "$dir/perf.data" -- sleep "$DURATION" \
            > "$dir/record.log" 2>&1 &
        CUR_PERF_PID=$!
        wait "$CUR_PERF_PID"
        rc=$?
        CUR_PERF_PID=""
        if [ "$rc" -ne 0 ] || [ ! -s "$dir/perf.data" ]; then
            warn "perf record 失败（返回 $rc），详见 $dir/record.log"
            return 1
        fi

        # 内核符号：采集后立即导出，驱动模块仍处于加载状态
        cat /proc/kallsyms > "$dir/kallsyms" 2>/dev/null

        # 样本统计
        "$PERF" report -i "$dir/perf.data" --header-only 2>/dev/null \
            | grep -iE "nr_samples|sample|lost" | head -n 5 >> "$dir/info.txt"
        grep -iE "lost|wake|samples" "$dir/record.log" | tail -n 3 >> "$dir/info.txt"

        # 导出文本（含内存映射），供 perf_analyzer 分析
        if [ "$NO_SCRIPT" -eq 0 ] || [ "$HAVE_ANALYZER" -eq 1 ]; then
            "$PERF" script -i "$dir/perf.data" --show-mmap-events 2>/dev/null > "$dir/out.perf"
            if [ "$HAVE_ANALYZER" -eq 1 ] && [ -s "$dir/out.perf" ]; then
                "$ANALYZER" "$dir/out.perf" --duration "$DURATION" \
                    --addr-list "$dir/addrs.txt" --folded-out "$dir/out.folded" \
                    > "$dir/report.txt" 2> "$dir/analyzer.log" \
                    && log "分析报告: $dir/report.txt" \
                    || warn "perf_analyzer 运行失败，详见 $dir/analyzer.log"
            fi
            if [ "$NO_SCRIPT" -eq 0 ]; then
                gzip -f "$dir/out.perf"
            else
                rm -f "$dir/out.perf"
            fi
        fi
    fi

    echo "结束时间: $(date '+%Y-%m-%d %H:%M:%S')" >> "$dir/info.txt"

    if [ "$PACK" -eq 1 ]; then
        tar -czf "$dir.tar.gz" -C "$OUTDIR" "$(basename "$dir")" 2>/dev/null \
            && log "已打包: $dir.tar.gz" || warn "打包失败"
    fi

    size=$(du -sk "$dir" 2>/dev/null | awk '{print $1}')
    log "完成，占用 ${size:-?} KB，剩余空间 $(free_mb "$OUTDIR") MB"
    cleanup_old
    return 0
}

# ---------------------------------------------------------------------------
# 主循环
# ---------------------------------------------------------------------------
log "进程 $([ "$SYSTEM_WIDE" -eq 1 ] && echo 全系统 || echo "$PROC")，每次 ${DURATION} 秒，频率 ${FREQ} Hz，共 ${ROUNDS} 次"
log "输出目录 $OUTDIR，剩余 $(free_mb "$OUTDIR") MB；perf_analyzer：$([ "$HAVE_ANALYZER" -eq 1 ] && echo 可用 || echo 未找到，只保存原始数据)"

i=1
while [ "$i" -le "$ROUNDS" ]; do
    start=$(date +%s)
    capture_once "$i"
    if [ "$i" -lt "$ROUNDS" ] && [ "$INTERVAL" -gt 0 ]; then
        elapsed=$(( $(date +%s) - start ))
        wait_s=$(( INTERVAL - elapsed ))
        if [ "$wait_s" -gt 0 ]; then
            log "等待 ${wait_s} 秒后开始下一次采集"
            sleep "$wait_s"
        fi
    fi
    i=$((i + 1))
done

log "全部完成，结果在 $OUTDIR"
