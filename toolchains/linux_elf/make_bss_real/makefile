
########## Variables

include ../make.arch

LIBC_BASE=../libc_base/
ZOOG_ROOT=../../..

IFC_MODULE=$(ZOOG_ROOT)/common/ifc/

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils/
COMMON_UTILS_MODULE_OBJS=\
	LiteLib.o \

ELFOBJ_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/elfobj/
ELFOBJ_MODULE_OBJS=\
	elfobj.o \
	elf_flat_reader.o \

MODULES=$(ELFOBJ_MODULE) $(COMMON_UTILS_MODULE) $(IFC_MODULE)
MODULE_OBJS=$(ELFOBJ_MODULE_OBJS) $(COMMON_UTILS_MODULE_OBJS)
	

DEP_SRCS=$(wildcard *.c)
TARGETS=ld_bss.so

CFLAGS=\
	-DELFOBJ_USE_LIBC=1 \
	-DSTATIC_WORD_SIZE=$(STATIC_WORD_SIZE) \

########## Targets

include ../make.in

########## Rules

$(BUILD)/make_bss_real: $(BUILD)/make_bss_real.o $(MODULE_OBJS)
	gcc -g $^ -o $@

$(BUILD)/librtld.os: $(LIBC_BASE)/elf/librtld.os
	cp $^ $@

$(BUILD)/ld.so.lds:
	gcc   -nostdlib -nostartfiles -shared 	\
		  -Wl,-z,combreloc -Wl,-z,relro -Wl,--hash-style=both -Wl,-z,defs -Wl,--verbose 2>&1 |	\
		  LC_ALL=C \
		  sed -e '/^=========/,/^=========/!d;/^=========/d'	\
		      -e 's/\. = .* + SIZEOF_HEADERS;/& _begin = . - SIZEOF_HEADERS;/' \
		  > $@

$(BUILD)/ld.map: $(LIBC_BASE)
	cp $(LIBC_BASE)/ld.map $@

$(BUILD)/ld_bss.so: $(BUILD)/make_bss_real $(BUILD)/librtld.os $(BUILD)/ld.so.lds $(BUILD)/ld.map
	(cd $(BUILD); ./make_bss_real ./ librtld.os ld.so.lds $(notdir $@))
