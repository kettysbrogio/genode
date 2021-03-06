#
# Configuration for L4 build system (for kernel-bindings, sigma0, bootstrap).
#
L4_CONFIG = $(call select_from_repositories,contrib/l4/mk/defconfig/config.x86)

#
# Create mirror for architecture-specific L4sys header files
#
L4_INC_TARGETS = x86/l4/sys \
                 x86/l4f/l4/sys \
                 x86/l4/vcpu

include $(REP_DIR)/lib/mk/platform.inc
