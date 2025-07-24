#include "bindings.h"

#include "core/SkStream.h"
#include "core/SkTypeface.h"


sk_stream_asset_t* reason_skia_typeface_open_existing_stream(const sk_typeface_t* typeface, int* ttcIndex) {
    return reinterpret_cast<sk_stream_asset_t*>(reinterpret_cast<const SkTypeface&>(typeface).openExistingStream(ttcIndex).release());
}