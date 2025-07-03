#include <stdio.h>
#include <cstring>

#include <caml/alloc.h>
#include <caml/bigarray.h>
#include <caml/callback.h>
#include <caml/custom.h>
#include <caml/memory.h>
#include <caml/mlvalues.h>

#include <hb-ot.h>
#include <hb.h>

/* #define TEST_FONT "E:/FiraCode-Regular.ttf" */
/* #define TEST_FONT "E:/Hasklig-Medium.otf" */
/* #define TEST_FONT "E:/Cassandra.ttf" */
/* #define TEST_FONT "E:/Lato-Regular.ttf" */

extern "C" {

// hb_font_t*

    CAMLprim value Val_success(value v) {
        CAMLparam1(v);
        CAMLlocal1(some);
        some = caml_alloc(1, 0);
        Store_field(some, 0, v);
        CAMLreturn(some);
    }

    CAMLprim value Val_error(const char *szMsg) {
        CAMLparam0();
        CAMLlocal1(error);
        error = caml_alloc(1, 1);
        Store_field(error, 0, caml_copy_string(szMsg));
        CAMLreturn(error);
    }

    static void custom_finalize_hb_font(value vFontBlock) {
        hb_font_t *pFont = *((hb_font_t **)Data_custom_val(vFontBlock));

        fprintf(stderr, "Finalizing hb_font_t* at %p\n", (void *)pFont);
        if (pFont) {
            hb_font_destroy(pFont);
        }
    }

// Define the custom operations for hb_font_t
    static struct custom_operations hb_font_custom_ops = {
        "harfbuzz.font",
        custom_finalize_hb_font,
        custom_compare_default,
        custom_hash_default,
        custom_serialize_default,
        custom_deserialize_default,
        custom_compare_ext_default,
        custom_fixed_length_default
    };

    /* Use native open type implementation to load font
      https://github.com/harfbuzz/harfbuzz/issues/255 */
    hb_font_t *get_font_ot(char *data, int length, int size) {
        hb_blob_t *blob =
            hb_blob_create(data, length, HB_MEMORY_MODE_WRITABLE, (void *)data, free);
        hb_face_t *face = hb_face_create(blob, 0);

        hb_blob_destroy(blob); // face will keep a reference to blob

        hb_font_t *font = hb_font_create(face);
        hb_face_destroy(face); // font will keep a reference to face

        hb_ot_font_set_funcs(font);
        hb_font_set_scale(font, size, size);

        return font;
    }

    CAMLprim value rehb_face_from_path(value vString) {
        CAMLparam1(vString);
        CAMLlocal1(ret);

        const char *szFont = String_val(vString);

        FILE *file = fopen(szFont, "rb");

        if (!file) {
            CAMLreturn(Val_error("File does not exist"));
        }

        fseek(file, 0, SEEK_END);
        unsigned int length = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *data = (char *)malloc(length);
        fread(data, length, 1, file);
        fclose(file);

        hb_font_t *hb_font;
        hb_font = get_font_ot(data, length, 12 /*iSize*/ * 64);

        if (!hb_font) {
            ret = Val_error("Unable to load font");
        } else {
            CAMLlocal1(custom_font_block);
            custom_font_block =
                caml_alloc_custom(&hb_font_custom_ops, sizeof(hb_font_t *), 0, 1);
            *((hb_font_t **)Data_custom_val(custom_font_block)) = hb_font;
            ret = Val_success(custom_font_block);
        }
        CAMLreturn(ret);
    }

    CAMLprim value rehb_face_from_bytes(value vPtr, value vLength) {
        CAMLparam2(vPtr, vLength);
        CAMLlocal1(ret);

        char *caml_data = Bp_val(vPtr);
        int length = Int_val(vLength);

        char *data = (char *)malloc(length * sizeof(char));
        std::memcpy(data, caml_data, length);

        hb_font_t *hb_font;
        hb_font = get_font_ot(data, length, 12 /*iSize*/ * 64);

        if (!hb_font) {
            ret = Val_error("Unable to load font");
        } else {
            CAMLlocal1(custom_font_block);
            custom_font_block =
                caml_alloc_custom(&hb_font_custom_ops, sizeof(hb_font_t *), 0, 1);
            *((hb_font_t **)Data_custom_val(custom_font_block)) = hb_font;
            ret = Val_success(custom_font_block);
        }
        CAMLreturn(ret);
    }

    static value createShapeTuple(unsigned int codepoint, unsigned int cluster) {
        CAMLparam0();

        CAMLlocal1(ret);
        ret = caml_alloc(2, 0);
        Store_field(ret, 0, Val_int(codepoint));
        Store_field(ret, 1, Val_int(cluster));
        CAMLreturn(ret);
    }

    CAMLprim value rehb_shape(value vFace, value vString, value vFeatures,
                              value vStart, value vLen) {
        CAMLparam5(vFace, vString, vFeatures, vStart, vLen);
        CAMLlocal2(ret, feat);

        int start = Int_val(vStart);
        int len = Int_val(vLen);

        int featuresLen = Wosize_val(vFeatures);
        hb_feature_t *features =
            (hb_feature_t *)malloc(featuresLen * sizeof(hb_feature_t));
        for (int i = 0; i < featuresLen; i++) {
            feat = Field(vFeatures, i);
            const char *tag = String_val(Field(feat, 0));
            features[i].tag = HB_TAG(tag[0], tag[1], tag[2], tag[3]);
            features[i].value = Int_val(Field(feat, 1));
            features[i].start = Int_val(Field(feat, 2));
            features[i].end = Int_val(Field(feat, 3));
        }

        hb_font_t *hb_font = *((hb_font_t **)Data_custom_val(vFace));

        hb_buffer_t *hb_buffer;
        hb_buffer = hb_buffer_create();
        hb_buffer_add_utf8(hb_buffer, String_val(vString), -1, start, len);
        hb_buffer_guess_segment_properties(hb_buffer);

        hb_shape(hb_font, hb_buffer, features, featuresLen);

        unsigned int glyph_count;
        hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, &glyph_count);

        ret = caml_alloc(glyph_count, 0);

        for (int i = 0; i < glyph_count; i++) {
            Store_field(ret, i, createShapeTuple(info[i].codepoint, info[i].cluster));
        }
        free(features);
        hb_buffer_destroy(hb_buffer);
        CAMLreturn(ret);
    }

    CAMLprim value rehb_version_string_compiled() {
        CAMLparam0();
        CAMLlocal1(ret);

        ret = caml_copy_string(HB_VERSION_STRING);

        CAMLreturn(ret);
    }

    CAMLprim value rehb_version_string_runtime() {
        CAMLparam0();
        CAMLlocal1(ret);

        ret = caml_copy_string(hb_version_string());

        CAMLreturn(ret);
    }
}
