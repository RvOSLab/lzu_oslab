import gdb

class Unwinder(object):
    def __init__(self, name):
        self.name = name
        self.enabled = True

    def __call__(self, pending_frame):
        raise NotImplementedError("Unwinder __call__.")

def register_unwinder(locus, unwinder, replace=False):
    if locus is None:
        if gdb.parameter("verbose"):
            gdb.write("Registering global %s unwinder ...\n" % unwinder.name)
        locus = gdb
    elif isinstance(locus, gdb.Objfile) or isinstance(locus, gdb.Progspace):
        if gdb.parameter("verbose"):
            gdb.write("Registering %s unwinder for %s ...\n" %
                      (unwinder.name, locus.filename))
    else:
        raise TypeError("locus should be gdb.Objfile or gdb.Progspace or None")

    i = 0
    for needle in locus.frame_unwinders:
        if needle.name == unwinder.name:
            if replace:
                del locus.frame_unwinders[i]
            else:
                raise RuntimeError("Unwinder %s already exists." %
                                   unwinder.name)
        i += 1
    locus.frame_unwinders.insert(0, unwinder)
    gdb.invalidate_cached_frames()

class FrameId(object):
    def __init__(self, sp, pc):
        self.sp = sp
        self.pc = pc

class OSLabTrapUnwinder(Unwinder):
    def __init__(self):
        super(OSLabTrapUnwinder, self).__init__("OSLabTrapUnwinder")
        self.trap_addr = None
        self.other_reg = ["zero", "ra", "gp", "tp"]
        self.other_reg += ["t%d" % i for i in range(0, 6 + 1)]
        self.other_reg += ["a%d" % i for i in range(0, 7 + 1)]
        self.other_reg += ["s%d" % i for i in range(1, 11 + 1)]

    def __call__(self, pending_frame):
        if not self.trap_addr:
            self.trap_addr = gdb.parse_and_eval("__trapret")
        pc = pending_frame.read_register("pc")
        if pc == self.trap_addr: # 发现__trapret所在的栈帧
            # 获取栈上保存的struct trapframe结构体
            sp = pending_frame.read_register("sp")
            trapframe = sp.cast(gdb.lookup_type("struct trapframe").pointer())
            # 提供新的栈帧
            break_sp = trapframe["gpr"]["sp"]
            break_pc = trapframe["epc"]
            unwind_info = pending_frame.create_unwind_info(FrameId(break_sp, break_pc))
            # 提供栈帧相关的寄存器
            unwind_info.add_saved_register("pc", break_pc)
            unwind_info.add_saved_register("sp", break_sp)
            unwind_info.add_saved_register("fp", trapframe["gpr"]["s0"])
            # 提供其他保存的寄存器
            for reg in self.other_reg:
                unwind_info.add_saved_register(reg, trapframe["gpr"][reg])
            # 提供保存的CSR寄存器
            unwind_info.add_saved_register("sstatus", trapframe["status"])
            unwind_info.add_saved_register("scause", trapframe["cause"])
            unwind_info.add_saved_register("sepc", trapframe["epc"])
            return unwind_info # 提交栈帧信息
        return None

register_unwinder(None, OSLabTrapUnwinder())

class OSLabFilter():
    def __init__(self):
        self.name = "OSLabFrameFilter"
        self.priority = 42
        self.enabled = True
        gdb.frame_filters[self.name] = self

    def filter(self, frame_iter):
        frame_iter = map(OSLabFrameDecorator, frame_iter)
        return frame_iter

class SymValue():
    def __init__(self, symbol, value):
        self.sym = symbol
        self.val = value

    def value(self):
        return self.val

    def symbol(self):
        return self.sym

from gdb.FrameDecorator import FrameDecorator
class OSLabFrameDecorator(FrameDecorator):
    def __init__(self, fobj):
        super(OSLabFrameDecorator, self).__init__(fobj)

    def function(self):
        frame = self.inferior_frame()
        name = str(frame.name())
        if name == "trap_from_user":
            return "[中断处理过程]"
        return super().function()

    def frame_args(self):
        frame = self.inferior_frame()
        name = str(frame.name())
        if name == "trap_from_user":
            sp = frame.read_register("sp")
            trapframe = sp.cast(gdb.lookup_type("struct trapframe").pointer())
            scause = trapframe["cause"]
            return [SymValue("scause", scause)]
        return super().frame_args()

    def filename(self):
        frame = self.inferior_frame()
        return frame.find_sal().symtab.fullname()

OSLabFilter()

print("栈帧大师已加载")

PAGE_SIZE = 4096
MIN_ALLOC_SIZE_LOG2 = 4

def mem_kb(mem_size):
    return mem_size / 1024

def page_kb(page_num):
    return mem_kb(page_num * PAGE_SIZE)

class MallocInfo(gdb.Command):
    def __init__(self):
        super(MallocInfo, self).__init__ ("lab-malloc", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        # 提取桶描述符目录struct bucket_desc* bucket_dir
        bucket_dir = gdb.parse_and_eval("bucket_dir")
        dir_start, dir_end = bucket_dir.type.range()

        print("malloc info:")
        print("\tsize:\tnum:")
        now_size = 2 ** (MIN_ALLOC_SIZE_LOG2 - 1)
        total_page_num = 0
        total_malloced_mem = 0

        for i in range(dir_start, dir_end + 1):
            now_alloced_item = 0
            now_size *= 2
            bucket_ptr = bucket_dir[i]
            if bucket_ptr == 0: continue
            while bucket_ptr != 0:
                bucket = bucket_ptr.dereference()
                now_alloced_item += bucket["refcnt"]
                total_page_num += 1
                bucket_ptr = bucket["next"]
            total_malloced_mem += now_alloced_item * now_size
            print("\t%4dB\t%4d" % (now_size, now_alloced_item))

        print("malloced memory: %0.2fKB" % mem_kb(total_malloced_mem))
        print("malloc page used: %d (%0.2fKB)" % (total_page_num, page_kb(total_page_num)))

MallocInfo()

def page_lowbits_str(bits_int):
    u = "U " if bits_int & 0b10000 else "S "
    x = "X" if bits_int & 0b01000 else "-"
    w = "W" if bits_int & 0b00100 else "-"
    r = "R" if bits_int & 0b00010 else "-"
    return "".join((u, x, w, r))

def virual_page_addr(vpn1, vpn2, vpn3, offset):
    return (vpn1 << 30) | (vpn2 << 21) | (vpn3 << 12) | offset

def page_table_entry_iter(page_table):
    uint64_t = gdb.lookup_type("uint64_t")

    entry_num = PAGE_SIZE // uint64_t.sizeof
    table_t = uint64_t.array(entry_num)

    page_table = page_table.cast(table_t.pointer()).dereference()
    page_table.fetch_lazy()
    for i in range(entry_num):
        page_table_entry = page_table[i]
        page_table_entry.fetch_lazy()
        if not (page_table_entry & 0b00001): continue
        if page_table_entry & 0b01110:
            yield (
                virual_page_addr(i, 0, 0, 0),
                virual_page_addr(i, 0x1ff, 0x1ff, 0xfff),
                (page_table_entry & 0x3ffffffffffc00) << 2,
                page_table_entry & 0x3f
            )
        else:
            page_table_2 = (page_table_entry & 0x3ffffffffffc00) << 2
            page_table_2 = page_table_2.cast(table_t.pointer()).dereference()
            page_table_2.fetch_lazy()
            for j in range(entry_num):
                page_table_entry_2 = page_table_2[j]
                page_table_entry_2.fetch_lazy()
                if not (page_table_entry_2 & 0b00001) : continue
                if page_table_entry_2 & 0b01110:
                    yield (
                        virual_page_addr(i, j, 0, 0),
                        virual_page_addr(i, j, 0x1ff, 0xfff),
                        (page_table_entry_2 & 0x3ffffffffffc00) << 2,
                        page_table_entry_2 & 0x3f
                    )
                else:
                    page_table_3 = (page_table_entry_2 & 0x3ffffffffffc00) << 2
                    page_table_3 = page_table_3.cast(table_t.pointer()).dereference()
                    page_table_3.fetch_lazy()
                    for k in range(entry_num):
                        page_table_entry_3 = page_table_3[k]
                        page_table_entry_3.fetch_lazy()
                        if not (page_table_entry_3 & 0b00001) : continue
                        if page_table_entry_3 & 0b01110:
                            yield (
                                virual_page_addr(i, j, k, 0),
                                virual_page_addr(i, j, k, 0xfff),
                                (page_table_entry_3 & 0x3ffffffffffc00) << 2,
                                page_table_entry_3 & 0x3f
                            )

def page_table_print(v_start, v_end, p_start, stat):
    print("0x%x-0x%x => 0x%x-0x%x (%s)" % (
        v_start, v_end,
        p_start, p_start + v_end - v_start,
        page_lowbits_str(stat)
    ))

class PageInfo(gdb.Command):
    def __init__(self):
        super(PageInfo, self).__init__ ("lab-page", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        gdb.execute("maintenance packet Qqemu.PhyMemMode:1")
        pg_dir = gdb.parse_and_eval("$satp") << 12
        is_start = False
        now_start = None; now_end = None; now_phy_addr = None; now_stat = None
        for start, end, phy_addr, stat in page_table_entry_iter(pg_dir):
            if not is_start:
                now_start = start; now_end = end
                now_phy_addr = phy_addr; now_stat = stat & 0b11111
                is_start = True
                continue
            if now_end + 1 == start and now_stat == (stat & 0b11111):
                if now_phy_addr + (now_end - now_start) + 1 == phy_addr:
                    now_end = end
                    continue
            page_table_print(now_start, now_end, now_phy_addr, now_stat)
            now_start = start; now_end = end
            now_phy_addr = phy_addr; now_stat = stat & 0b11111
        else:
            page_table_print(now_start, now_end, now_phy_addr, now_stat)
        gdb.execute("maintenance packet Qqemu.PhyMemMode:0")

PageInfo()

print("内存大师已加载")

def type_decorator(type_name, field_name):
    real_type = gdb.lookup_type(type_name)
    if field_name not in real_type:
        print("类型<%s>没有名为<%s>的域" % (type_name, field_name))
        return None
    ds_field = real_type[field_name]
    ds_offset = ds_field.bitpos >> 3
    def container_of(obj):
        if obj.type != ds_field.type:
            print("数据结构类型不匹配 需要<%s> 实际<%s>" % (ds_field.type, obj.type))
            return None
        real_addr = obj.address.cast(gdb.lookup_type("uint64_t")) - ds_offset
        return real_addr.cast(real_type.pointer()).dereference()
    return container_of, type_name

linked_list_map = {}

class linked_list:
    def __init__(self, obj, container_of, type_name):
        self.obj = obj
        self.container_of = container_of
        self.type_name = type_name
        self.gdb_type = None
    def enum(self):
        if not self.gdb_type:
            self.gdb_type = gdb.lookup_type("struct linked_list_node").pointer()
        node = self.obj["next"]
        head = self.obj.address.cast(self.gdb_type)
        i = 0
        while node != head:
            node_d = node.dereference()
            yield str(i), self.container_of(node_d)
            node = node_d["next"]
            i += 1
    def children(self):
        return self.enum()
    def to_string(self):
        return "[双向链表 head@0x%x]<%s>" % (self.obj.address, self.type_name)
    def display_hint(self):
        return "array"

hash_table_map = {}

class hash_table:
    def __init__(self, obj, container_of, type_name):
        self.obj = obj
        self.container_of = container_of
        self.type_name = type_name
    def enum(self):
        List_P = gdb.lookup_type("struct linked_list_node").pointer()
        node_container, _ = type_decorator("struct hash_table_node", "confliced_list")
        buffer_length = self.obj["buffer_length"]
        buffer = self.obj["buffer"]
        buffer.cast(gdb.lookup_type("struct hash_table_node").array(buffer_length))
        i = 0
        for idx in range(buffer_length):
            linklist = buffer[idx]["confliced_list"]
            head = linklist.address.cast(List_P)
            node = linklist["next"]
            while node != head:
                node_d = node.dereference()
                yield str(i), self.container_of(node_container(node_d))
                node = node_d["next"]
                i += 1
            
    def children(self):
        return self.enum()
    def to_string(self):
        return "[哈希表 head@0x%x]<%s>" % (self.obj.address, self.type_name)
    def display_hint(self):
        return "array"

def data_structure_printer(obj):
    if obj.address and int(obj.address) in linked_list_map:
        return linked_list_map[int(obj.address)]
    elif obj.address and int(obj.address) in hash_table_map:
        return hash_table_map[int(obj.address)]
    return None

gdb.pretty_printers.append(data_structure_printer)

class EnumDataStructure(gdb.Command):
    def __init__(self):
        super(EnumDataStructure, self).__init__ ("type-set", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        HashTable = gdb.lookup_type("struct hash_table")
        HashTable_P = HashTable.pointer()
        List = gdb.lookup_type("struct linked_list_node")
        List_P = List.pointer()
        argv = gdb.string_to_argv(arg)
        if len(argv) < 3:
            print("用法: enum-ds <数据结构对象> <具体对象类型> <数据结构在具体类型中所用名称>")
            return
        obj = gdb.parse_and_eval(argv[0])
        if obj.type == List_P or obj.type == HashTable_P:
            obj = obj.dereference()
        if obj.type == List:
            print("[双向链表] %s" % obj.type)
            container_of, type_name = type_decorator(argv[1], argv[2])
            linked_list_map[int(obj.address)] = linked_list(obj, container_of, type_name)
        elif obj.type == HashTable:
            print("[哈希表] %s" % obj.type)
            container_of, type_name = type_decorator(argv[1], argv[2])
            hash_table_map[int(obj.address)] = hash_table(obj, container_of, type_name)
            pass
        else:
            print("不支持的对象类型<%s>" % obj.type)
EnumDataStructure()

print("数据结构大师已加载")