include(CMakePrintHelpers)

set(SRC spinner_core.h
        spnr.h spnr.cpp
        md_ref.h md_ref.cpp
        records.h records.cpp
        spinner_core_util.h spinner_core_util.cpp
        delta_builder.h
        config.h config.cpp
       )

add_library(spinner_core STATIC ${SRC})
set_property(TARGET spinner_core PROPERTY CXX_STANDARD 23)



