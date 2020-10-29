include prorab.mk

# include guard
ifneq ($(prorab_config_included),true)
    prorab_config_included := true

    ifeq ($(config),)
        override config := $(c)
    else
        override c := $(config)
    endif
	ifeq ($(config),)
        override config := rel
        override c := $(config)
	endif

    define prorab-config
        $(if $1,,$(error no 'config dir' argument is given to prorab-config macro))

        this_out_dir := out/$(c)/
        $(eval config_dir := $(abspath $(d)$(filter-out ,$1))/) # filter-out is needed to trim possible leading and trailing spaces

        $(eval prorab_private_config_file := $(config_dir)$(c).mk)
        $(if $(wildcard $(prorab_private_config_file)),,$(error no $(c).mk config file found in $(filter-out ,$1) directory))
        include $(prorab_private_config_file)
    endef

endif # ~include guard
