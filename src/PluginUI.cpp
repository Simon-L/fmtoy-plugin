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
    static int item_current = 0;
    
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
           uint n = 0;
           for (auto const& i : opm_list) {
             // std::cout << n << " : " << fs::path(res / i).c_str() << std::endl;
             n++;
           }
         }
         if (std::strcmp(key, "file") == 0)
         {
           std::cout << "UI stateChanged file " << value << '\n';
         }
         //     fParamGrid[0] = valueOnOff;
         // else if (std::strcmp(key, "top-center") == 0)
         //     fParamGrid[1] = valueOnOff;
         // else if (std::strcmp(key, "top-right") == 0)
         //     fParamGrid[2] = valueOnOff;
         // else if (std::strcmp(key, "middle-left") == 0)
         //     fParamGrid[3] = valueOnOff;
         // else if (std::strcmp(key, "middle-center") == 0)
         //     fParamGrid[4] = valueOnOff;
         // else if (std::strcmp(key, "middle-right") == 0)
         //     fParamGrid[5] = valueOnOff;
         // else if (std::strcmp(key, "bottom-left") == 0)
         //     fParamGrid[6] = valueOnOff;
         // else if (std::strcmp(key, "bottom-center") == 0)
         //     fParamGrid[7] = valueOnOff;
         // else if (std::strcmp(key, "bottom-right") == 0)
         //     fParamGrid[8] = valueOnOff;

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

        ImGui::SetNextWindowPos(ImVec2(margin, margin));
        ImGui::SetNextWindowSize(ImVec2(width - 2 * margin, height - 2 * margin));

        if (ImGui::Begin("fmtoy", nullptr, ImGuiWindowFlags_NoResize))
        {

            if (ImGui::SliderFloat("Gain (dB)", &fGain, -90.0f, 30.0f))
            {
                if (ImGui::IsItemActivated())
                    editParameter(0, true);

                setParameterValue(0, fGain);
            }
            
            std::vector<const char*> strings;
            for (auto const& f : opm_list) {
              strings.push_back(f.c_str());
            }
            if (ImGui::Combo("File", &item_current, strings.data(), strings.size()))
            {
              setState("file", strings[item_current]);
            }
                
            if (ImGui::IsItemDeactivated())
            {
                editParameter(0, false);
            }
            
            ImGui::ShowDemoWindow(nullptr);
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
