#include <cstring>
#include <stdio.h>

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
        // hb_font_set_scale(font, size, size);

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

// Zero-copy version using memory pointer directly (SkSharper-style)
    CAMLprim value rehb_face_from_memory_ptr(value vPtr, value vLength,
            value vtcIndex) {
        CAMLparam3(vPtr, vLength, vtcIndex);
        CAMLlocal1(ret);

        void *memory_ptr = (void *)Nativeint_val(vPtr);
        int length = Int_val(vLength);
        int tcIndex = Int_val(vtcIndex);
        hb_blob_t *blob = hb_blob_create((const char *)memory_ptr, length,
                                         HB_MEMORY_MODE_READONLY, nullptr, nullptr);
        hb_blob_make_immutable(blob);
        hb_face_t *face = hb_face_create(blob, (unsigned)tcIndex);
        hb_blob_destroy(blob); // face will keep a reference to blob

        hb_font_t *font = hb_font_create(face);
        hb_face_destroy(face); // font will keep a reference to face

        hb_ot_font_set_funcs(font);

        if (!font) {
            ret = Val_error("Unable to load font from memory");
        } else {
            CAMLlocal1(custom_font_block);
            custom_font_block =
                caml_alloc_custom(&hb_font_custom_ops, sizeof(hb_font_t *), 0, 1);
            *((hb_font_t **)Data_custom_val(custom_font_block)) = font;
            ret = Val_success(custom_font_block);
        }
        CAMLreturn(ret);
    }

    static value
    create_hb_shaped_glyph_record(unsigned int glyphId, unsigned int cluster,
                                  hb_position_t xAdvance, hb_position_t yAdvance,
                                  hb_position_t xOffset, hb_position_t yOffset,
                                  double unitsPerEm) {
        CAMLparam0();
        CAMLlocal1(recordBlock);

        const int numFields = 7;
        recordBlock = caml_alloc(numFields, 0);

        // Set the fields of the OCaml record
        // Remember fields are 0-indexed
        Store_field(recordBlock, 0, Val_int(glyphId)); // glyphId: int
        Store_field(recordBlock, 1, Val_int(cluster)); // cluster: int

        // Convert HarfBuzz positions (hb_position_t, which is an int representing
        // font units) to float pixels
        Store_field(recordBlock, 2,
                    caml_copy_double((double)xAdvance)); // xAdvance: float
        Store_field(recordBlock, 3,
                    caml_copy_double((double)yAdvance)); // yAdvance: float
        Store_field(recordBlock, 4,
                    caml_copy_double((double)xOffset)); // xOffset: float
        Store_field(recordBlock, 5,
                    caml_copy_double((double)yOffset)); // yOffset: float
        Store_field(recordBlock, 6, caml_copy_double(unitsPerEm));
        CAMLreturn(recordBlock);
    }

    CAMLprim value rehb_shape(value vFace, value vString, value vFeatures,
                              value vStart, value vLen, value vFontSize) {
        CAMLparam5(vFace, vString, vFeatures, vStart, vLen);
        CAMLxparam1(vFontSize);
        CAMLlocal3(ret, feat, shapedGlyphRecord);

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
        hb_face_t *hb_face = hb_font_get_face(hb_font);
        double units_per_em = (double)hb_face_get_upem(hb_face);

        // Fix for Apple Color Emoji fonts that may return 0 or invalid units_per_em
        if (units_per_em <= 0.0) {
            units_per_em = 1000.0; // Use standard default value
        }

        // Create new buffer for each call to avoid reuse issues
        hb_buffer_t *hb_buffer = hb_buffer_create();

        hb_buffer_add_utf8(hb_buffer, String_val(vString), -1, start, len);
        hb_buffer_guess_segment_properties(hb_buffer);

        hb_shape(hb_font, hb_buffer, features, featuresLen);

        unsigned int glyph_count;
        hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, &glyph_count);
        hb_glyph_position_t *positions =
            hb_buffer_get_glyph_positions(hb_buffer, &glyph_count);

        ret = caml_alloc(glyph_count, 0);

        for (int i = 0; i < glyph_count; i++) {
            shapedGlyphRecord = create_hb_shaped_glyph_record(
                                    info[i].codepoint, info[i].cluster, positions[i].x_advance,
                                    positions[i].y_advance, positions[i].x_offset, positions[i].y_offset,
                                    units_per_em);
            Store_field(ret, i, shapedGlyphRecord);
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

// Table-based font creation for efficient emoji font handling
    struct table_callback_data {
        value ocaml_callback; // OCaml function to call for table data
        value user_data;      // User data passed to callback
    };

// HarfBuzz table callback - called when HarfBuzz needs a font table
    static hb_blob_t *get_table_callback(hb_face_t *face, hb_tag_t tag,
                                         void *user_data) {
        struct table_callback_data *data = (struct table_callback_data *)user_data;

        // Convert tag to OCaml value (uint32_t -> int32)
        CAMLparam0();
        CAMLlocal2(tag_val, result);

        tag_val = caml_copy_int32(tag);

        // Call OCaml callback: callback(tag, user_data) -> option(string)
        result = caml_callback2(data->ocaml_callback, tag_val, data->user_data);

        // Check if result is Some(data) or None
        if (Is_block(result) && Tag_val(result) == 0) {
            // Some(data) - extract the string
            value data_val = Field(result, 0);
            const char *table_data = String_val(data_val);
            size_t table_size = caml_string_length(data_val);

            // Create HarfBuzz blob with copy mode for safety
            hb_blob_t *blob = hb_blob_create(
                                  table_data, table_size, HB_MEMORY_MODE_DUPLICATE, nullptr, nullptr);
            CAMLreturnT(hb_blob_t *, blob);
        } else {
            // None - table not found
            CAMLreturnT(hb_blob_t *, nullptr);
        }
    }

// Cleanup callback for table data
    static void cleanup_table_callback_data(void *user_data) {
        struct table_callback_data *data = (struct table_callback_data *)user_data;
        caml_remove_global_root(&data->ocaml_callback);
        caml_remove_global_root(&data->user_data);
        free(data);
    }

// Create HarfBuzz face using table callback approach
    CAMLprim value rehb_face_create_for_tables(value vCallback, value vUserData) {
        CAMLparam2(vCallback, vUserData);
        CAMLlocal1(ret);

        // Allocate callback data structure
        struct table_callback_data *callback_data =
            (struct table_callback_data *)malloc(sizeof(struct table_callback_data));

        // Store OCaml callback and user data as global roots
        callback_data->ocaml_callback = vCallback;
        callback_data->user_data = vUserData;
        caml_register_global_root(&callback_data->ocaml_callback);
        caml_register_global_root(&callback_data->user_data);

        // Create HarfBuzz face with table callback
        hb_face_t *face = hb_face_create_for_tables(get_table_callback, callback_data,
                          cleanup_table_callback_data);

        // Create HarfBuzz font from face
        hb_font_t *font = hb_font_create(face);
        hb_face_destroy(face); // font keeps reference

        hb_ot_font_set_funcs(font);

        if (!font) {
            ret = Val_error("Unable to create font from tables");
        } else {
            CAMLlocal1(custom_font_block);
            custom_font_block =
                caml_alloc_custom(&hb_font_custom_ops, sizeof(hb_font_t *), 0, 1);
            *((hb_font_t **)Data_custom_val(custom_font_block)) = font;
            ret = Val_success(custom_font_block);
        }

        CAMLreturn(ret);
    }
}
