/*
  ==============================================================================

    SPUnicode.cpp
    Created: 04 Jul 2026
    Author:  boherm

  ==============================================================================
*/

// CoreFoundation must be included BEFORE JuceHeader: after it, MacTypes' legacy `Point`
// clashes with juce::Point. That forces __APPLE__ here (JUCE_MAC isn't defined yet — it
// comes from JuceHeader). Everywhere else we use JUCE_MAC as usual.
#if defined(__APPLE__)
 #include <CoreFoundation/CFString.h>
#endif

#include "SPUnicode.h"

juce::String SPUnicode::toNFC(const juce::String& s)
{
#if JUCE_MAC
    if (s.isEmpty()) return s;

    CFStringRef cf = CFStringCreateWithCString(kCFAllocatorDefault, s.toRawUTF8(), kCFStringEncodingUTF8);
    if (cf == nullptr) return s;

    CFMutableStringRef m = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, cf);
    CFRelease(cf);
    if (m == nullptr) return s;

    CFStringNormalize(m, kCFStringNormalizationFormC);

    juce::String result = s;
    const CFIndex length = CFStringGetLength(m);
    const CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
    juce::HeapBlock<char> buffer((size_t) maxSize);
    if (CFStringGetCString(m, buffer, maxSize, kCFStringEncodingUTF8))
        result = juce::String::fromUTF8(buffer);

    CFRelease(m);
    return result;
#else
    // Windows / Linux filenames aren't NFD-normalized the way macOS's are: no-op.
    return s;
#endif
}
