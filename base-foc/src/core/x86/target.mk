include $(PRG_DIR)/../target.inc

REQUIRES += x86
SRC_CC   += x86/platform_x86.cc

vpath io_port_session_component.cc $(GEN_CORE_DIR)/x86

