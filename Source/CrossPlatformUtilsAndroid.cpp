// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#if JUCE_ANDROID

#include "CrossPlatformUtils.h"
#include "JuceHeader.h"
#include "juce_core/native/juce_BasicNativeHeaders.h"
#include "juce_core/native/juce_JNIHelpers_android.h"

//#include "../JuceLibraryCode/JuceHeader.h"

//#include "DebugLogC.h"

void getSafeAreaInsets(void * component, float & top, float & bottom, float & left, float & right)
{
    top = bottom = left = right = 0;
    if (component == nullptr) return;
    auto* env = juce::getEnv();
    auto view = static_cast<jobject>(component);
    // The JUCE peer is a separate window; system-bar insets belong to the Activity.
    const auto activity = juce::getMainActivity();
    juce::LocalRef<jobject> decor;
    if (activity != nullptr) {
        juce::LocalRef<jclass> activityClass(env->GetObjectClass(activity.get()));
        auto windowMethod=env->GetMethodID(activityClass.get(),"getWindow","()Landroid/view/Window;");
        juce::LocalRef<jobject> window(env->CallObjectMethod(activity.get(),windowMethod));
        if(window != nullptr){
            juce::LocalRef<jclass> windowClass(env->GetObjectClass(window.get()));
            auto decorMethod=env->GetMethodID(windowClass.get(),"getDecorView","()Landroid/view/View;");
            decor=juce::LocalRef<jobject>(env->CallObjectMethod(window.get(),decorMethod));
            if(decor != nullptr)view=decor.get();
        }
    }
    juce::LocalRef<jclass> viewClass(env->GetObjectClass(view));
    auto getInsets = env->GetMethodID(viewClass.get(), "getRootWindowInsets", "()Landroid/view/WindowInsets;");
    if (getInsets == nullptr) { env->ExceptionClear(); return; }
    juce::LocalRef<jobject> insets(env->CallObjectMethod(view, getInsets));
    if (insets == nullptr) return;
    juce::LocalRef<jclass> insetClass(env->GetObjectClass(insets.get()));
    const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
    const float scale = display != nullptr ? float(display->scale) : 1.0f;
    auto read = [&](const char* method) {
        auto id = env->GetMethodID(insetClass.get(), method, "()I");
        if (id == nullptr) { env->ExceptionClear(); return 0.0f; }
        return float(env->CallIntMethod(insets.get(), id)) / scale;
    };
    top=read("getSystemWindowInsetTop"); bottom=read("getSystemWindowInsetBottom");
    left=read("getSystemWindowInsetLeft"); right=read("getSystemWindowInsetRight");
}

#endif
