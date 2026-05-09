#!/usr/bin/env python3
"""
Dump and cross-check hwcfg.ehc / swcfg.esc binary configuration files.

Parses the binary format as defined by RD_Initialize (hwcfg.c) and
SC_Initialize (swcfg.c), then dumps the key sections in human-readable
form.  When both files are given it cross-references DI IDs to find
mismatches.

Usage:
  python dump_config.py hwcfg.ehc
  python dump_config.py swcfg.esc
  python dump_config.py hwcfg.ehc swcfg.esc
"""

import sys
import os
import re
import struct

# ═══════════════════════════════════════════════════════════════════════════
#  common helpers
# ═══════════════════════════════════════════════════════════════════════════

def u16le(b, off):
    return b[off] | (b[off + 1] << 8)

def u32le(b, off):
    return b[off] | (b[off + 1] << 8) | (b[off + 2] << 16) | (b[off + 3] << 24)

def f32le(b, off):
    return struct.unpack('<f', b[off:off + 4])[0]

def decode_cfg_text(raw):
    """Decode config strings.  Chinese names in the config are usually GBK."""
    raw = raw.rstrip(b'\x00\xff')
    for enc in ('gb18030', 'utf-8', 'ascii'):
        try:
            return raw.decode(enc)
        except UnicodeDecodeError:
            pass
    return raw.decode('gb18030', errors='replace')

def read_len_str(b, off):
    """Read a length-prefixed string: b[off] = len, bytes follow."""
    n = b[off]
    s = decode_cfg_text(b[off + 1 : off + 1 + n])
    return s, off + 1 + n

def decode_xxd_dump(text):
    """Convert an xxd-style text dump back to raw bytes."""
    out = bytearray()
    parsed = False

    for line in text.splitlines():
        m = re.match(r'^\s*[0-9a-fA-F]+:\s*(.*)$', line)
        if not m:
            if line.strip():
                return None
            continue

        rest = m.group(1)
        hex_area = rest.split('  ', 1)[0]
        hex_text = ''.join(re.findall(r'[0-9a-fA-F]{2,4}', hex_area))
        if not hex_text or len(hex_text) % 2:
            return None

        out.extend(int(hex_text[i:i + 2], 16) for i in range(0, len(hex_text), 2))
        parsed = True

    return bytes(out) if parsed else None

def load_config_bytes(path):
    """Load a raw config file, accepting checked-in xxd text dumps as input."""
    with open(path, 'rb') as f:
        data = f.read()

    if data[:4] in (HW_MAGIC_HDR, SW_MAGIC_HDR):
        return data

    try:
        text = data.decode('ascii')
    except UnicodeDecodeError:
        return data

    decoded = decode_xxd_dump(text)
    return decoded if decoded is not None else data

# ═══════════════════════════════════════════════════════════════════════════
#  hwcfg.ehc  (hardware config)
# ═══════════════════════════════════════════════════════════════════════════

HW_MAGIC_HDR = bytes([0x11, 0x00, 0x00, 0xEE])
HW_MAGIC_FTR = bytes([0x14, 0x00, 0x00, 0xEB])

HW_TYPES = {
    0:  "资源/机箱配置",
    1:  "硬件AI通道",
    2:  "逻辑AI通道",
    3:  "DI开入通道",
    4:  "DO开出通道",
    5:  "硬件LED",
    6:  "软件LED",
    7:  "虚拟AI",
    8:  "测量AI",
    9:  "AI增益修正",
    10: "CT变比",
    11: "脉冲输入PI",
    12: "脉冲输出PO",
    13: "模拟量输出AO",
}

FREQ_NAMES = {
    0x00: "50Hz / 5A",
    0x01: "50Hz / 1A",
    0x02: "60Hz / 5A",
    0x03: "60Hz / 1A",
    0x80: "50Hz / 1A+5A",
    0x81: "60Hz / 1A+5A",
    0xC0: "50/60Hz / 5A",
    0xC1: "50/60Hz / 1A",
    0xC2: "50/60Hz / 1A+5A",
}

MOD_NAMES = {
    0x04: "交流模件(AI)",
    0x11: "开入模件(DI)",
    0x12: "开入开出模件(DIO)",
    0x13: "CKDIO模件",
    0x16: "通讯模件(COM)",
    0x20: "开出模件(DO)",
}

POS_NAMES = {
    0: "主机箱",
    1: "扩展机箱",
    8: "冗余机箱",
}

def hw_parse_resource(data):
    """Parse type-0 item: RD_Cfg_Resource."""
    unPartNum = u16le(data, 0)
    flags0    = data[2]
    envType   = data[3]
    flags1    = data[4]
    cpuFlags  = data[5]
    appType   = data[6]

    print(f"    模件数量  : {unPartNum}")
    print(f"    标志字节0 : 0x{flags0:02X}  扩展={bool(flags0&1)} 64K光纵1={bool(flags0&2)} "
          f"2M光纵1={bool(flags0&4)} 64K光纵2={bool(flags0&8)} 2M光纵2={bool(flags0&16)}")
    freq_desc = FREQ_NAMES.get(envType, f"未知(0x{envType:02X})")
    print(f"    频率制式  : 0x{envType:02X}  ({freq_desc})")
    print(f"    标志字节1 : 0x{flags1:02X}  同杆并架={bool(flags1&1)} 智能操作箱={bool(flags1&2)} "
          f"AssmDev9_1={bool(flags1&4)} AssmDev_xn={bool(flags1&8)} 虚拟机箱={bool(flags1&16)}")
    print(f"    CPU标志   : 0x{cpuFlags:02X}  双CPU={not bool(cpuFlags&1)} CPUPos={1 if (cpuFlags&2) else 0}")
    print(f"    应用类型  : {appType}")
    print(f"    模件条目偏移: 8")

    # determine nominal power frequency for AiRate calculation
    nominal_freq = 50 if envType in (0x00, 0x01, 0x80, 0xC0, 0xC1, 0xC2) else 60

    off = 8
    results = {"unPartNum": unPartNum, "modules": [], "nominal_freq": nominal_freq}
    for idx in range(unPartNum):
        if off + 3 > len(data):
            print(f"    !! 模件#{idx}: 偏移{off}超出数据范围")
            break
        totalLen = u16le(data, off)
        idLen    = data[off + 2]
        aucId    = decode_cfg_text(data[off + 3 : off + 3 + idLen])
        modOff   = off + 3 + idLen
        if modOff + 13 > len(data):
            print(f"    !! 模件#{idx} \"{aucId}\": 条目数据不足")
            off += totalLen + 2
            continue

        ucType     = data[modOff]
        ucPosition = data[modOff + 1]
        hwAddr     = (data[modOff+2], data[modOff+3], data[modOff+4], data[modOff+5])
        # modOff+6..8 = 3 reserved bytes
        ucAoNum = data[modOff + 9]
        ucAiNum = data[modOff + 10]
        ucDiNum = data[modOff + 11]
        ucDoNum = data[modOff + 12]

        mod_name = MOD_NAMES.get(ucType, f"0x{ucType:02X}")
        pos_name = POS_NAMES.get(ucPosition, f"位置{ucPosition}")

        unAiPts = 0
        aiRate  = 0
        aiWarn  = ""
        if ucType == 0x04 and modOff + 15 < len(data):
            unAiPts = u16le(data, modOff + 13)
            aiRate  = unAiPts * nominal_freq
            if not (600 <= aiRate <= 20000):
                aiWarn = f"  *** 非法! 采样率={aiRate}Hz 超出[600,20000] ***"

        print(f"    [{idx}] ID=\"{aucId}\" 类型={mod_name} {pos_name} "
              f"hwAddr=[{hwAddr[0]},{hwAddr[1]},{hwAddr[2]},{hwAddr[3]}] "
              f"AO={ucAoNum} AI={ucAiNum} DI={ucDiNum} DO={ucDoNum}"
              f"{'  周波点数=' + str(unAiPts) + ' 采样率=' + str(aiRate) + 'Hz' if aiRate else ''}"
              f"{aiWarn}")

        results["modules"].append({
            "id": aucId, "type": ucType, "pos": ucPosition,
            "ai_pts": unAiPts, "ai_rate": aiRate
        })
        off += totalLen + 2

    return results


def hw_parse_di(data):
    """Parse type-3 item: RD_Cfg_DI.  Returns list of DI IDs."""
    diIds = []
    if len(data) < 6:
        print("    数据不足")
        return diIds

    diCount = u16le(data, 0)
    print(f"    DI通道总数: {diCount}")
    print(f"    {'ID':<20s} {'名称':<16s} {'简称':<6s} {'滤波(ms)':>8s} {'模件ID':<16s} 模件通道 录波 标志 测量")
    print(f"    {'─'*20} {'─'*16} {'─'*6} {'─'*8} {'─'*16}")

    off = 6
    for idx in range(diCount):
        if off + 3 > len(data):
            break
        itemLen = u16le(data, off)
        idLen   = data[off + 2]
        aucId   = decode_cfg_text(data[off + 3 : off + 3 + idLen])
        p       = off + 3 + idLen

        unLgcSN   = u16le(data, p); p += 2
        name, p   = read_len_str(data, p)
        abrv      = decode_cfg_text(data[p:p+4]); p += 4
        filtTime  = u32le(data, p); p += 4          # stored as ms in file
        _resvAttr   = data[p]; p += 1
        dftVal    = data[p]; p += 1
        mmiShow   = data[p]; p += 1
        p += 5                                        # reserved 5
        refreshRate = data[p]; p += 1
        modId, p    = read_len_str(data, p)
        modCh     = data[p]; p += 1
        bRec      = data[p]; p += 1
        recId = ""
        if bRec:
            recId, p = read_len_str(data, p)
        bFlag = data[p]; p += 1
        flagId = ""
        if bFlag:
            flagId, p = read_len_str(data, p)
        bMea  = data[p]; p += 1
        meaId = ""
        if bMea:
            meaId, p = read_len_str(data, p)

        print(f"    {aucId:<20s} {name:<16s} {abrv:<6s} {filtTime:>8d} {modId:<16s} "
              f"{modCh:>4d}  {bRec}/{bFlag}/{bMea}")
        diIds.append(aucId)
        off += itemLen + 2

    return diIds


def parse_hwcfg(path):
    """Parse a hwcfg.ehc file, return dict of parsed items."""
    data = load_config_bytes(path)

    if len(data) < 16:
        print(f"ERROR: {path} too small ({len(data)} bytes)")
        return None
    if data[0:4] != HW_MAGIC_HDR:
        print(f"ERROR: bad header magic {data[0:4].hex()}, expected {HW_MAGIC_HDR.hex()}")
        return None
    if data[-4:] != HW_MAGIC_FTR:
        print(f"WARNING: bad footer magic {data[-4:].hex()}, expected {HW_MAGIC_FTR.hex()}")

    protoVer = u16le(data, 4)
    progVer  = u16le(data, 6)
    itemCnt  = data[8]
    hwVer    = u16le(data, 9)

    print(f"\n{'='*60}")
    print(f"  硬件配置: {os.path.basename(path)}  ({len(data)} bytes)")
    print(f"{'='*60}")
    print(f"  协议版本 = {protoVer}  程序版本 = {progVer}")
    print(f"  配置项数 = {itemCnt}  硬件版本 = {hwVer}")

    result = {"di_ids": set(), "items": [], "resource": None}
    off = 12
    for i in range(itemCnt):
        if off + 5 > len(data) - 4:
            print(f"  !! 配置项#{i}: 超出文件范围")
            break
        ucType = data[off]
        ulLen  = u32le(data, off + 1)
        body   = data[off + 5 : off + 5 + ulLen]

        tname = HW_TYPES.get(ucType, f"未知({ucType})")
        print(f"\n  [{i}] type={ucType} ({tname})  size={ulLen}")

        if ucType == 0:
            result["resource"] = hw_parse_resource(body)
        elif ucType == 3:
            diIds = hw_parse_di(body)
            result["di_ids"].update(diIds)
            print(f"    → DI ID 数量: {len(diIds)}")
        elif ucType == 2:
            # Logic AI: just count
            aiCount = u16le(body, 0)
            print(f"    逻辑AI通道数量: {aiCount}")
        elif ucType == 1:
            aiCount = u16le(body, 0)
            print(f"    硬件AI通道数量: {aiCount}")
        else:
            # brief hex preview
            prev = body[:24].hex()
            if len(body) > 24:
                prev += "..."
            print(f"    hex: {prev}")

        result["items"].append((ucType, ulLen))
        off += 5 + ulLen

    return result


# ═══════════════════════════════════════════════════════════════════════════
#  swcfg.esc  (software config)
# ═══════════════════════════════════════════════════════════════════════════

SW_MAGIC_HDR = bytes([0x22, 0x00, 0x00, 0xDD])
SW_MAGIC_FTR = bytes([0x28, 0x00, 0x00, 0xD7])

SW_TYPES = {
    0:  "定值",
    1:  "内部定值",
    2:  "压板配置(Link)",
    3:  "事件配置",
    4:  "告警配置",
    5:  "录波标志",
    6:  "录波AI",
    7:  "录波DI",
    8:  "测量AI",
    9:  "测量DI",
    10: "测量值",
    11: "测量DO",
    12: "参数定值",
}

def sw_parse_link(data):
    """Parse type-2 item: SC_Cfg_Link.  Returns list of DI references."""
    diRefs = []   # list of (linkId, linkName, diSrcId)
    secondDiRefs = []

    # ── skip the first ID field (commented out in source) ──
    id0Len = data[0]
    off    = 1 + id0Len                         # puc += 1 + puc[0]

    iLkPgNum = data[off]; off += 1              # iLkPgNum_g = *puc++

    print(f"    跳过首ID长度: {id0Len}")
    print(f"    压板页数量: {iLkPgNum}")

    # ── first pass: count pages & links ──
    pucPgBgn = off + 4                          # pucPgBgn = puc + 4
    pageInfo = []
    totalLinks = 0
    pgOff = pucPgBgn
    for pgIdx in range(iLkPgNum):
        iPgCfgLen  = u16le(data, pgOff)         # iPgCfgLen = U8_TO_U16(puc[1], puc[0])
        nameLen    = data[pgOff + 2]
        pgName     = decode_cfg_text(data[pgOff + 3 : pgOff + 3 + nameLen])
        tmp        = pgOff + 3 + nameLen
        bIsPub     = bool(data[tmp]); tmp += 1
        protectId  = ""
        if not bIsPub:
            protLen = data[tmp]; tmp += 1
            if protLen > 0:
                protectId = decode_cfg_text(data[tmp : tmp + protLen])
                tmp += protLen
        iLinkNum   = u16le(data, tmp)
        pageInfo.append((pgName, bIsPub, protectId, iLinkNum, pgOff, iPgCfgLen))
        totalLinks += iLinkNum
        pgOff += 2 + iPgCfgLen                  # pucPgBgn += 2 + iPgCfgLen

    print(f"    压板总数: {totalLinks}")

    # ── second pass: parse links (page-level detail + link items) ──
    off = data[0] + 6                           # puc = pucCfg + pucCfg[0] + 6

    globalIdx = 0
    for pgIdx, (pgName, bIsPub, protectId, iLinkNum, _pgOff, _pgLen) in enumerate(pageInfo):
        iPgCfgLen = u16le(data, off)
        nameLen   = data[off + 2]
        pgName2   = decode_cfg_text(data[off + 3 : off + 3 + nameLen])
        p = off + 3 + nameLen
        bIsPub2   = bool(data[p]); p += 1       # skip IsPub
        protectId2 = ""
        if not bIsPub2:
            iPgCfgLen -= 1 + data[p]
            protLen = data[p]; p += 1
            if protLen > 0:
                protectId2 = decode_cfg_text(data[p : p + protLen])
                p += protLen
        p += 6                                  # skip 6 bytes

        print(f"\n    压板页[{pgIdx}] \"{pgName2}\" IsPub={bIsPub2} Protect=\"{protectId2}\" {iLinkNum}个压板")
        print(f"      {'ID':<20s} {'名称':<16s} {'简称':<6s} {'HW类型':>6s} {'默认':>4s} {'DI来源ID':<20s} {'第二DI来源':<16s}")
        print(f"      {'─'*20} {'─'*16} {'─'*6} {'─'*6} {'─'*4} {'─'*20} {'─'*16}")

        for li in range(iLinkNum):
            iItemCfgLen = u16le(data, p); p += 2
            iPgCfgLen -= 2 + iItemCfgLen

            # ID
            iItemCfgLen -= data[p]
            linkId, p = read_len_str(data, p)
            # Name
            iItemCfgLen -= data[p]
            linkName, p = read_len_str(data, p)
            # ABRV (4)
            abrv = decode_cfg_text(data[p:p+4]); p += 4
            hwLinkType    = data[p]; p += 1
            linkSwitchMode = data[p]
            p += 3                              # LinkSwitchMode + 2 reserved bytes
            p += 2                              # skip SEQ
            bDftVal = bool(data[p]); p += 1

            # DI source
            iItemCfgLen -= data[p]
            diSrcLen = data[p]; p += 1
            diSrcId  = ""
            if diSrcLen > 0:
                diSrcId = decode_cfg_text(data[p : p + diSrcLen])
                p += diSrcLen

            secondDiId = ""
            if diSrcLen > 0 and (hwLinkType & 0x01):
                iItemCfgLen -= 1 + data[p]
                secLen = data[p]; p += 1
                if secLen > 0:
                    secondDiId = decode_cfg_text(data[p : p + secLen])
                    p += secLen

            hasDi = "← 引用DI" if diSrcLen else ""
            print(f"      {linkId:<20s} {linkName:<16s} {abrv:<6s} {hwLinkType:>6d} {int(bDftVal):>4d} "
                  f"{diSrcId:<20s} {secondDiId:<16s} {hasDi}")

            if diSrcLen:
                diRefs.append((linkId, linkName, diSrcId))
            if secondDiId:
                secondDiRefs.append((linkId, linkName, secondDiId))
            globalIdx += 1

        off = p                                 # synchronize with source's puc advancement

    return diRefs, secondDiRefs


def parse_swcfg(path):
    """Parse a swcfg.esc file, return dict of parsed items."""
    data = load_config_bytes(path)

    if len(data) < 16:
        print(f"ERROR: {path} too small ({len(data)} bytes)")
        return None
    if data[0:4] != SW_MAGIC_HDR:
        print(f"ERROR: bad header magic {data[0:4].hex()}, expected {SW_MAGIC_HDR.hex()}")
        return None
    if data[-4:] != SW_MAGIC_FTR:
        print(f"WARNING: bad footer magic {data[-4:].hex()}, expected {SW_MAGIC_FTR.hex()}")

    protoVer = u16le(data, 4)
    progVer  = u16le(data, 6)
    itemCnt  = data[8]
    swVer    = u16le(data, 9)

    print(f"\n{'='*60}")
    print(f"  软件配置: {os.path.basename(path)}  ({len(data)} bytes)")
    print(f"{'='*60}")
    print(f"  协议版本 = {protoVer}  程序版本 = {progVer}")
    print(f"  配置项数 = {itemCnt}  软件版本 = {swVer}")

    result = {"di_refs": [], "items": []}
    off = 12
    for i in range(itemCnt):
        if off + 5 > len(data) - 4:
            print(f"  !! 配置项#{i}: 超出文件范围")
            break
        ucType = data[off]
        ulLen  = u32le(data, off + 1)
        body   = data[off + 5 : off + 5 + ulLen]

        tname = SW_TYPES.get(ucType, f"未知({ucType})")
        print(f"\n  [{i}] type={ucType} ({tname})  size={ulLen}")

        if ucType == 2:
            diRefs, secRefs = sw_parse_link(body)
            result["di_refs"].extend(diRefs)
            result["di_refs"].extend(secRefs)
        else:
            prev = body[:24].hex()
            if len(body) > 24:
                prev += "..."
            print(f"    hex: {prev}")

        result["items"].append((ucType, ulLen))
        off += 5 + ulLen

    return result


# ═══════════════════════════════════════════════════════════════════════════
#  cross-check
# ═══════════════════════════════════════════════════════════════════════════

def cross_check(hw, sw):
    """Cross-reference DI IDs between hwcfg and swcfg."""
    print(f"\n{'='*60}")
    print(f"  交叉校验: hwcfg DI定义  vs  swcfg 压板DI引用")
    print(f"{'='*60}")

    hwDiIds = hw.get("di_ids", set()) if hw else set()
    swDiRefs = sw.get("di_refs", []) if sw else []

    if not hwDiIds:
        print("\n  *** hwcfg 没有定义任何 DI 通道! ***")
        print("      这就是 plgcdich_g == NULL 的原因 → RD_Get_Handle 崩溃")
    else:
        print(f"\n  hwcfg 定义了 {len(hwDiIds)} 个 DI:")
        for di in sorted(hwDiIds):
            print(f"    - {di}")

    print(f"\n  swcfg 压板引用了 {len(swDiRefs)} 个 DI:")
    swDiSet = set()
    for linkId, linkName, diSrcId in swDiRefs:
        swDiSet.add(diSrcId)
        inHw = "✓" if diSrcId in hwDiIds else "✗ 缺失!"
        print(f"    - \"{diSrcId}\"  (压板 \"{linkName}\" id={linkId})  {inHw}")

    missing = swDiSet - hwDiIds
    if missing:
        print(f"\n  *** 有 {len(missing)} 个 DI 引用在 hwcfg 中不存在: ***")
        for di in sorted(missing):
            print(f"      - {di}")
        if not hwDiIds:
            print("  根因: hwcfg 完全没有 DI 配置项 (type=3)")
            print("  解决: 添加 EDP01_CA_OPT_BUILD 编译宏，或使用含 DI 配置的 hwcfg")
        else:
            print("  解决: 在 hwcfg 中补充这些 DI 定义，或修改 swcfg 压板去掉引用")
    else:
        print("\n  ✓ 所有 DI 引用都能在 hwcfg 中找到")


# ═══════════════════════════════════════════════════════════════════════════
#  main
# ═══════════════════════════════════════════════════════════════════════════

def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    hw_result = None
    sw_result = None

    for path in sys.argv[1:]:
        if not os.path.exists(path):
            print(f"ERROR: 文件不存在: {path}")
            continue
        data = load_config_bytes(path)
        magic = data[:4]

        if magic == HW_MAGIC_HDR:
            hw_result = parse_hwcfg(path)
        elif magic == SW_MAGIC_HDR:
            sw_result = parse_swcfg(path)
        else:
            print(f"ERROR: {path} 魔术字不匹配 ({magic.hex()})，不是有效的配置文件")

    if hw_result and sw_result:
        cross_check(hw_result, sw_result)


if __name__ == '__main__':
    main()
