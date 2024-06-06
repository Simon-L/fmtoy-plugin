/*
 * ImGui plugin example
 * Copyright (C) 2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
 * Copyright (C) 2021-2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: ISC
 */

#include "DistrhoPlugin.hpp"
#include "extra/ValueSmoother.hpp"

#include "DistrhoPluginUtils.hpp"

#include <string>
#include <list>
#include <iostream>

extern "C"
{
    #include "../fmtoy/tools.h"
    #include "../fmtoy/fmtoy.h"
    #include "../fmtoy/libfmvoice/fm_voice.h"
    #include "../fmtoy/midi.h"
}

#include <filesystem>
namespace fs = std::filesystem;

#define DEFAULT_CLOCK 3579545

#include <json.hpp>
using json = nlohmann::json;

START_NAMESPACE_DISTRHO


// --------------------------------------------------------------------------------------------------------------------

static constexpr const float CLAMP(float v, float min, float max)
{
    return std::min(max, std::max(min, v));
}

static constexpr const float DB_CO(float g)
{
    return g > -90.f ? std::pow(10.f, g * 0.05f) : 0.f;
}

// --------------------------------------------------------------------------------------------------------------------

class ImGuiPluginDSP : public Plugin
{
    enum Parameters {
        kParamGain = 0,
        kParamCount
    };

    float fGainDB = 0.0f;
    ExponentialValueSmoother fSmoothGain;

public:
    struct fmtoy fmtoy;
    fs::path res;
    fs::path opm_file;
    std::list<fs::path> opm_list;
    bool requestChangeOpmFile = false;
   /**
      Plugin class constructor.@n
      You must set all parameter values to their defaults, matching ParameterRanges::def.
    */
    ImGuiPluginDSP()
        : Plugin(kParamCount, 0, 2) // parameters, programs, states
    {
        fSmoothGain.setSampleRate(getSampleRate());
        fSmoothGain.setTargetValue(DB_CO(0.f));
        fSmoothGain.setTimeConstant(0.020f); // 20ms

        res = fs::path(getBinaryFilename()).parent_path().parent_path() / "Resources";
        opm_file = res / "default.opm";
        
        fmtoy_init(&fmtoy, DEFAULT_CLOCK, getSampleRate());

    }
    

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // Information
    const char* getLabel() const noexcept override { return "fmtoy";}
    const char* getDescription() const override { return "fmtoy";}
    const char* getLicense() const noexcept override { return "ISC";}
    uint32_t getVersion() const noexcept override { return d_version(1, 0, 0);}
    int64_t getUniqueId() const noexcept override { return d_cconst('f', 'm', 't', 'y');}
    const char* getMaker() const noexcept override { return "vampirefrog, Simon-L, Jean Pierre Cimalando, falkTX";}
    
    // ----------------------------------------------------------------------------------------------------------------
    // Init

    void initParameter(uint32_t index, Parameter& parameter) override
    {
        DISTRHO_SAFE_ASSERT_RETURN(index == 0,);

        parameter.ranges.min = -90.0f;
        parameter.ranges.max = 30.0f;
        parameter.ranges.def = 0.0f;
        parameter.hints = kParameterIsAutomatable;
        parameter.name = "Gain";
        parameter.shortName = "Gain";
        parameter.symbol = "gain";
        parameter.unit = "dB";
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Internal data
    
    /**
       Initialize the state @a index.@n
       This function will be called once, shortly after the plugin is created.@n
       Must be implemented by your plugin class only if DISTRHO_PLUGIN_WANT_STATE is enabled.
     */
    void initState(uint32_t index, State& state) override
    {
      // std::cout << "initState " << index << '\n';
      if (index == 0)
      {
        json j = opm_list;
        state.key = "file_list";
        state.defaultValue = j.dump().c_str();
      }
      if (index == 1)
      {
        state.key = "file";
        state.defaultValue = "";
        state.hints = kStateIsFilenamePath;
        std::cout << "DSP initState 1 " << state.key << '\n';
      }
    }
    
    void setState(const char* key, const char* value) override
    {
      if (std::strcmp(key, "file") == 0)
      {
        auto old_file = opm_file;
        opm_file = fs::path(value);
        if (std::strcmp(opm_file.extension().c_str(), ".opm") != 0)
        {
          std::cout << "Not an opm file!" << '\n';
          opm_file = old_file;
        } else {
          std::cout << "OK! " << opm_file << '\n';
          requestChangeOpmFile = true;
        }
      }
    }
   /**
      Get the current value of a parameter.@n
      The host may call this function from any context, including realtime processing.
    */
    float getParameterValue(uint32_t index) const override
    {
        DISTRHO_SAFE_ASSERT_RETURN(index == 0, 0.0f);

        return fGainDB;
    }

   /**
      Change a parameter value.@n
      The host may call this function from any context, including realtime processing.@n
      When a parameter is marked as automatable, you must ensure no non-realtime operations are performed.
      @note This function will only be called for parameter inputs.
    */
    void setParameterValue(uint32_t index, float value) override
    {
        DISTRHO_SAFE_ASSERT_RETURN(index == 0,);

        fGainDB = value;
        fSmoothGain.setTargetValue(DB_CO(CLAMP(value, -90.0, 30.0)));
    }
    
    void loadBank()
    {
      struct fm_voice_bank bank;
      fm_voice_bank_init(&bank);
      size_t data_len;
      uint8_t *data = load_file(opm_file.c_str(), &data_len);
      if(!data) {
        printf("Could not open %s\n", opm_file.c_str());
        return;
      }
      fm_voice_bank_load(&bank, data, data_len);
      fmtoy_append_fm_voice_bank(&fmtoy, &bank);
      
      for(int i = 0; i < 16; i++)
        fmtoy_program_change(&fmtoy, i, 0);
        
      std::cout << "Done, loaded " << fmtoy.num_voices << " voices from " << opm_file << " (" << data_len << ") with sampleRate : " << getSampleRate() << '\n';
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Audio/MIDI Processing

   /**
      Activate this plugin.
    */
    void activate() override
    {
        fSmoothGain.clearToTargetValue();
        loadBank();
          
    }
    
#define EVENT_NOTEON 0x90
#define EVENT_NOTEOFF 0x80
#define EVENT_PITCHBEND 0xE0
#define EVENT_PGMCHANGE 0xC0
#define EVENT_CONTROLLER 0xB0
    
    void handleMidi(const MidiEvent* event)
    {   
      uint8_t b0 = event->data[0]; // status + channel
      uint8_t b0_status = b0 & 0xF0; // status + channel
      uint8_t b0_channel = b0 & 0x0F; // status + channel
      uint8_t b1 = event->data[1]; // note
      uint8_t b2 = event->data[2]; // velocity
      d_stdout("MIDI in 0x%x (status: 0x%x, channel: 0x%x) %d %d", b0, b0_status, b0_channel, b1, b2);
      
      switch (b0_status) {
        case EVENT_NOTEON:
          printf("%s: Note \033[32mON\033[0m  %s%d (%d) %d\n", fmtoy_channel_name(&fmtoy, b0_channel), midi_note_name(b1), midi_note_octave(b1), b1, b2);
          if(b2 > 0)
            fmtoy_note_on(&fmtoy, b0_channel, b1, b2);
          else
            fmtoy_note_off(&fmtoy, b0_channel, b1, b2);
          break;
        case EVENT_NOTEOFF:
          printf("%s: Note \033[31mOFF\033[0m %s%d (%d) %d\n", fmtoy_channel_name(&fmtoy, b0_channel), midi_note_name(b1), midi_note_octave(b1), b1, b2);
          fmtoy_note_off(&fmtoy, b0_channel, b1, b2);
          break;
        case EVENT_PITCHBEND:
          printf("\033[33mPitchbend \033[1m%d\033[0m \n", (b1 | (b2 << 8)));
          fmtoy_pitch_bend(&fmtoy, b0_channel, (b1 | (b2 << 8)));
          break;
        case EVENT_PGMCHANGE:
          printf("\033[33mProgram \033[1m%d\033[0m %.256s\n", b1, fmtoy.num_voices > b1 ? fmtoy.opm_voices[b1].name : "No voice");
          for(int i = 0; i < 16; i++)
            fmtoy_program_change(&fmtoy, i, b1);
          break;
        case EVENT_CONTROLLER:
          printf("%s: CC 0x%02x (%s) %d\n", fmtoy_channel_name(&fmtoy, b0_channel), b1, midi_cc_name(b1), b2);
          fmtoy_cc(&fmtoy, b0_channel, b1, b2);
          break;
      }
    }
   /**
      Run/process function for plugins without MIDI input.
      @note Some parameters might be null if there are no audio inputs or outputs.
    */
    void run(const float** inputs, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        if (requestChangeOpmFile)
        {
          fmtoy_destroy(&fmtoy);
          fmtoy_init(&fmtoy, DEFAULT_CLOCK, getSampleRate());
          loadBank();
          requestChangeOpmFile = false;
        }
        
        for (size_t i = 0; i < midiEventCount; i++) {
          handleMidi(&midiEvents[i]);
        }
        // get the left and right audio outputs
        float* const outL = outputs[0];
        float* const outR = outputs[1];
        
        fmtoy_render(&fmtoy, frames);

        // apply gain against all samples
        for (uint32_t i=0; i < frames; ++i)
        {
            const float gain = fSmoothGain.next();
            outL[i] = (fmtoy.render_buf_l[i] / 32767.0f) * gain;
            outR[i] = (fmtoy.render_buf_r[i] / 32767.0f) * gain;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Callbacks (optional)

   /**
      Optional callback to inform the plugin about a sample rate change.@n
      This function will only be called when the plugin is deactivated.
      @see getSampleRate()
    */
    void sampleRateChanged(double newSampleRate) override
    {
        fSmoothGain.setSampleRate(newSampleRate);
        std::cout << "SR changed to " << newSampleRate << '\n';
    }

    // ----------------------------------------------------------------------------------------------------------------

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImGuiPluginDSP)
};

// --------------------------------------------------------------------------------------------------------------------

Plugin* createPlugin()
{
    return new ImGuiPluginDSP();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
