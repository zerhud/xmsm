builddir := make_build
GCC := g++ -MMD -pipe -std=gnu++26 -fwhole-program -march=native -fdiagnostics-color=always -fdiagnostics-all-candidates 
CLANG := clang++ -MMD -pipe -std=gnu++26 -march=native -fdiagnostics-color=always -ftemplate-backtrace-limit=0 -Wno-overloaded-shift-op-parentheses
CLANG_UNWRAP := $(shell which -a clang++ | sed -n '2p')
GCC_UNWRAP := $(shell which -a g++ | sed -n '2p')
INCLUDES := -I. -I./absd -Ijiexpr -Iast_graph -Ivirtual_variant -Ijinja

tests_src := $(shell find . -ipath '*/tests/*.cpp' | sed 's/^..//g')

.PHONY: all force_clang vis vis_build vb

base = $(basename $(subst tests/,,$(1)))
all: $(foreach src_file,$(tests_src),$(call base,$(src_file)))
	@echo -e "\e[7;32mAll Done\e[0m"

force_clang: $(foreach src_file,$(tests_src),$(call base,$(src_file))_force_clang)

define create_test_dir_template
$(1):
	mkdir -p $$@
endef

extra_flags = $(if $(findstring rtsnitch_,$(1)),-lsnitch,)

define create_test_template

-include $(builddir)/$(call base,$(1))_gcc.d
$(builddir)/$(call base,$(1))_gcc: makefile $(1) | $(builddir)/$(dir $(call base,$(1)))
	$(GCC) $(call extra_flags,$(1)) $(INCLUDES) $(1) -o $$@
-include $(builddir)/$(call base,$(1))_clang.d
ifeq (,$(findstring gcconly_,$(1)))
$(builddir)/$(call base,$(1))_clang: makefile $(1) | $(builddir)/$(dir $(call base,$(1)))
	@echo build for $(call base,$(1))
	$(CLANG) $(call extra_flags,$(1)) $(INCLUDES) $(1) -o $$@
$(builddir)/$(call base,$(1))_force_clang: makefile $(1)
	@echo -e "\033[0;31mskiping \033[1;36m$(1)\033[0;31m for FORCE clang\033[0m"
else 
$(builddir)/$(call base,$(1))_clang: makefile $(1)
	@echo -e "\033[0;31mskiping for \033[1;36m$(1)\033[0;31m for clang\033[0m"
$(builddir)/$(call base,$(1))_force_clang: makefile $(1) | $(builddir)/$(dir $(call base,$(1)))
	@echo build for $(call base,$(1))
	$(CLANG) $(call extra_flags,$(1)) $(INCLUDES) $(1) -o $$@

endif
.PHONY: $(call base,$(1)) $(call base,$(1))_force_clang
$(call base,$(1))_force_clang: $(builddir)/$(call base,$(1))_force_clang
$(call base,$(1)): $(builddir)/$(call base,$(1))_gcc $(builddir)/$(call base,$(1))_clang
	$(builddir)/$(call base,$(1))_gcc
	test -f $(builddir)/$(call base,$(1))_clang && $(builddir)/$(call base,$(1))_clang || true
clean::
	rm -f $(builddir)/$(call base,$(1))_gcc{,.d}
	rm -f $(builddir)/$(call base,$(1))_clang{,.d}

endef

directories := $(sort $(foreach src_file,$(tests_src),$(builddir)/$(dir $(call base,$(src_file)))))
$(foreach src_file,$(directories), $(eval $(call create_test_dir_template,$(src_file))))
$(foreach src_file,$(tests_src),$(eval $(call create_test_template,$(src_file))))

-include $(builddir)/vis.d
$(builddir)/vis.wasm: vis/machine.cpp makefile | $(builddir)
	$(CLANG_UNWRAP) -fdiagnostics-color=always -MMD -std=gnu++26 --target=wasm32 -nostdlib -O3 -Wl,--no-entry -Wl,--export-all -Wl,--import-undefined -fno-rtti -fno-exceptions -I. -ftemplate-backtrace-limit=0 -o $@ vis/machine.cpp
$(builddir)/vis-network.min.js: vis/vis-network.min.js
	cp -t $(builddir) vis/vis-network.min.js
$(builddir)/index.html: vis/index.html
	cp -t $(builddir) vis/index.html

vis_build: $(builddir)/vis.wasm $(builddir)/vis-network.min.js $(builddir)/index.html
vb: vis_build
vis: vis_build
	cd $(builddir) && http-server -c1
