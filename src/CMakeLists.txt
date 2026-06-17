# KRunner plugin
kcoreaddons_add_plugin(yaftirunner INSTALL_NAMESPACE "kf6/krunner")

target_sources(yaftirunner PRIVATE
    yaftirunner.cpp
)

target_link_libraries(yaftirunner
    Qt6::Core
    Qt6::Gui
    Qt6::DBus
    KF6::Runner
    KF6::I18n
    yaml-cpp
)

target_compile_options(yaftirunner PRIVATE -fexceptions)
