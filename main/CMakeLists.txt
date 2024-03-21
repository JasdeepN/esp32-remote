set(COMPONENT_REQUIRES conn time-sync)
set(COMPONENT_PRIV_REQUIRES )

# set(COMPONENT_SRCS "in.cma")
set(COMPONENT_ADD_INCLUDEDIRS "." )

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components )

# register_component()

idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "." 
                    REQUIRES conn time-sync)
