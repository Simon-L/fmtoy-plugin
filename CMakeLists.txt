# DISTRHO Plugin Framework (DPF)
# Copyright (C) 2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
# Copyright (C) 2021-2022 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: ISC

cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 17)

set(NAME fmtoy-plugin)
project(${NAME})

add_subdirectory(dpf)

set(fmtoy_src fmtoy/fmtoy.c fmtoy/cmdline.c fmtoy/tools.c fmtoy/midi.c
	fmtoy/libfmvoice/fm_voice.c fmtoy/libfmvoice/op3_file.c fmtoy/libfmvoice/opm_file.c
    fmtoy/libfmvoice/bnk_file.c fmtoy/libfmvoice/ins_file.c fmtoy/libfmvoice/sbi_file.c
    fmtoy/libfmvoice/tfi_file.c fmtoy/libfmvoice/y12_file.c fmtoy/libfmvoice/syx_dx21.c
    fmtoy/libfmvoice/syx_fb01.c fmtoy/libfmvoice/dmp_file.c fmtoy/chips/ym2151.c
    fmtoy/chips/fm.c fmtoy/chips/fm2612.c fmtoy/chips/ymdeltat.c fmtoy/chips/fmopl.c fmtoy/chips/ymf262.c
	fmtoy/fmtoy_ym2151.c fmtoy/fmtoy_ym2203.c fmtoy/fmtoy_ym2608.c fmtoy/fmtoy_ym2610.c
fmtoy/fmtoy_ym2610b.c fmtoy/fmtoy_ym2612.c fmtoy/fmtoy_ym3812.c fmtoy/fmtoy_ymf262.c)

dpf_add_plugin(${NAME}
  TARGETS vst3
  FILES_DSP
      src/PluginDSP.cpp
      ${fmtoy_src}
  FILES_UI
      src/PluginUI.cpp
      ${fmtoy_src}
      dpf-widgets/opengl/DearImGui.cpp)

target_include_directories(${NAME} PUBLIC src)
target_include_directories(${NAME} PUBLIC include)
target_include_directories(${NAME} PUBLIC dpf-widgets/generic)
target_include_directories(${NAME} PUBLIC dpf-widgets/opengl)

target_link_libraries(${NAME} PUBLIC z)

add_custom_command(TARGET fmtoy-plugin-vst3 POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/res $<TARGET_FILE_DIR:fmtoy-plugin-vst3>/../Resources)