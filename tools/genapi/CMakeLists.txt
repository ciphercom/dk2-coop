set(TARGET genapi)

add_custom_target(${TARGET}
        SOURCES
        main.py
        gen_utils.py
        dk2cxx.py
        gen_functions_h.py
        gen_globals_h.py
        gen_struct_h.py
        )

add_custom_target(update_replace_globals_txt
        SOURCES
        main_update_replace_globals.py
        dk2cxx.py
        )
