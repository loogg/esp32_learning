
GEN_CSRCS += $(notdir $(wildcard $(PRJ_DIR)/custom/*.c))
GEN_CSRCS += $(notdir $(wildcard $(PRJ_DIR)/custom/customer_setup_screens/*.c))

DEPPATH += --dep-path $(PRJ_DIR)/custom
DEPPATH += --dep-path $(PRJ_DIR)/custom/customer_setup_screens

VPATH += :$(PRJ_DIR)/custom
VPATH += :$(PRJ_DIR)/custom/customer_setup_screens

CFLAGS += "-I$(PRJ_DIR)/custom"
CFLAGS += "-I$(PRJ_DIR)/custom/customer_setup_screens"
