/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   about_png;
    const int            about_pngSize = 118622;

    extern const char*   crash_png;
    const int            crash_pngSize = 110194;

    extern const char*   fade_cue_png;
    const int            fade_cue_pngSize = 3977;

    extern const char*   default_ponylayout;
    const int            default_ponylayoutSize = 3038;

    extern const char*   playlist_cue_png;
    const int            playlist_cue_pngSize = 3123;

    extern const char*   icon_png;
    const int            icon_pngSize = 40809;

    extern const char*   tray_icon_png;
    const int            tray_icon_pngSize = 2548;

    extern const char*   audio_cue_png;
    const int            audio_cue_pngSize = 2581;

    extern const char*   audio_interface_png;
    const int            audio_interface_pngSize = 2882;

    extern const char*   group_cue_png;
    const int            group_cue_pngSize = 2036;

    extern const char*   midi_interface_png;
    const int            midi_interface_pngSize = 4238;

    extern const char*   mixer_interface_png;
    const int            mixer_interface_pngSize = 4283;

    extern const char*   network_interface_png;
    const int            network_interface_pngSize = 2011;

    extern const char*   noicon_png;
    const int            noicon_pngSize = 2379;

    extern const char*   note_cue_png;
    const int            note_cue_pngSize = 1988;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 15;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
