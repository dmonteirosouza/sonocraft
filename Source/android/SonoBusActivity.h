
#pragma once

#include "JuceHeader.h"

#if JUCE_ANDROID


#include "juce_core/native/juce_BasicNativeHeaders.h"

#include "juce_core/juce_core.h"
#include "juce_core/native/juce_JNIHelpers_android.h"

namespace juce {

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    METHOD (setForegroundServiceActive,     "setForegroundServiceActive", "(Z)V")

    DECLARE_JNI_CLASS (SonoBusActivity, JUCE_ANDROID_ACTIVITY_OVERRIDE_CLASSPATH);
#undef JNI_CLASS_MEMBERS


}


#endif
