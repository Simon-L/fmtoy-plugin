/*
 * ImGui plugin example
 * Copyright (C) 2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
 * Copyright (C) 2021-2022 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: ISC
 */

#include <iostream>

#include "DistrhoUI.hpp"
#include "ResizeHandle.hpp"
#include "DistrhoPluginUtils.hpp"

#include <filesystem>
namespace fs = std::filesystem;
#include <json.hpp>
using json = nlohmann::json;

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class ImGuiPluginUI : public UI
{
    float fGain = 0.0f;
    int i0 = 0;
    ResizeHandle fResizeHandle;

    // ----------------------------------------------------------------------------------------------------------------

public:
   /**
      UI class constructor.
      The UI should be initialized to a default state that matches the plugin side.
    */
    fs::path res;
    std::list<std::string> opm_list;
    int item_current = 0;
    
    ImGuiPluginUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true),
          fResizeHandle(this)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        // hide handle if UI is resizable
        if (isResizable())
            fResizeHandle.hide();
            
        res = fs::path(getBinaryFilename()).parent_path().parent_path() / "Resources";
    }

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // DSP/Plugin Callbacks
    /**
       A state has changed on the plugin side.
       This is called by the host to inform the UI about state changes.
     */
     void stateChanged(const char* key, const char* value) override
     {
         // std::cout << "UI: stateChanged " << key << value << '\n';
         const bool valueOnOff = (std::strcmp(value, "true") == 0);

         // check which block changed
         if (std::strcmp(key, "file_list") == 0)
         {
           opm_list = std::list<std::string>(json::parse(value));
           // uint n = 0;
           // for (auto const& i : opm_list) {
           //   // std::cout << n << " : " << fs::path(res / i).c_str() << std::endl;
           //   n++;
           // }
         }
         if (std::strcmp(key, "file") == 0)
         {
           std::cout << "UI stateChanged file " << value << '\n';
         }
         // trigger repaint
         repaint();
     }
     
   /**
      A parameter has changed on the plugin side.@n
      This is called by the host to inform the UI about parameter changes.
    */
    void parameterChanged(uint32_t index, float value) override
    {
        DISTRHO_SAFE_ASSERT_RETURN(index == 0,);

        fGain = value;
        repaint();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Widget Callbacks

   /**
      ImGui specific onDisplay function.
    */
    void onImGuiDisplay() override
    {
        const float width = getWidth();
        const float height = getHeight();
        const float margin = 20.0f * getScaleFactor();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(width, height));

        if (ImGui::Begin("fmtoy", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
        {

            if (ImGui::Button("Load .opm"))
            {
              requestStateFile("file");
            }
            
            if (ImGui::SliderFloat("Gain (dB)", &fGain, -90.0f, 30.0f))
            {
                if (ImGui::IsItemActivated())
                    editParameter(0, true);

                setParameterValue(0, fGain);
            }
                
            if (ImGui::IsItemDeactivated())
            {
                editParameter(0, false);
            }
            
            // ImGui::ShowDemoWindow(nullptr);
        }
        ImGui::End();
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImGuiPluginUI)
};

// --------------------------------------------------------------------------------------------------------------------

UI* createUI()
{
    return new ImGuiPluginUI();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
