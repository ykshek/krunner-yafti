# KRunner plugin
kcoreaddons_add_plugin(krunner-yafti INSTALL_NAMESPACE "kf6/krunner")

target_sources(krunner-yafti PRIVATE
    yaftirunner.cpp
)

target_link_libraries(krunner-yafti
    PRIVATE
        KF6::Runner
        KF6::I18n
        KF6::ConfigCore
        Qt6::Core
        Qt6::Gui
        Qt6::DBus
        yaml-cpp
)

target_compile_options(krunner-yafti PRIVATE -fexceptions)
