cmake_minimum_required(VERSION 3.26)

include(ExternalProject)
include(FetchContent)

###########################################################
### JUCE 
# 
# 注意 : ISC Licenseで提供されるモジュールのみ利用可能
#      * juce_core
#      * juce_audio_devices
#      * juce_audio_basics
#      * juce_events
#        

set(JUCE_MODULES_ONLY YES)
set(JUCE_BUILD_EXTRAS NO)
set(JUCE_BUILD_EXAMPLES NO)

add_subdirectory(third_party/JUCE)
