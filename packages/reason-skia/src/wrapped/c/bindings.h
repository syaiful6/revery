#ifndef REASON_SKIA_BINDINGS_H
#define REASON_SKIA_BINDINGS_H
#include "c/sk_types.h"
SK_C_PLUS_PLUS_BEGIN_GUARD

SK_C_API sk_stream_asset_t* reason_skia_typeface_open_existing_stream(const sk_typeface_t* typeface, int* ttcIndex);

SK_C_PLUS_PLUS_END_GUARD
#endif