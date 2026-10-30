cmake_minimum_required(VERSION 3.22)
project(SonoCraftAndroid LANGUAGES C CXX)
# Use the same source/resource list as desktop and iOS.
add_subdirectory("../../../.." "sonocraft")
set_target_properties(SonoBus_Standalone PROPERTIES OUTPUT_NAME juce_jni)
