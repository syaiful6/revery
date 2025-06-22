
/*
 * Use this file for any manual, raw bindings - ie,
 * ones that use [@noalloc], [@unboxed].
 */

#include "c_stubs.h"
#include "include/c/sk_canvas.h"
#include "include/c/sk_matrix.h"
#include "include/c/sk_paint.h"
#include "include/c/sk_types.h"
#include "include/c/sk_typeface.h"
#include "include/c/sk_data.h"
#include "include/c/sk_stream.h"

#include <stdio.h>
#include <stdlib.h>

#include "ctypes_cstubs_internals.h"

double reason_skia_stub_sk_color_float_get_b(int32_t color) {
    return (double)(sk_color_get_b(color) / 255.0);
}

CAMLprim value reason_skia_stub_sk_color_float_get_b_byte(value vColor) {
    return caml_copy_double(
               reason_skia_stub_sk_color_float_get_b(Int32_val(vColor)));
}

double reason_skia_stub_sk_color_float_get_g(int32_t color) {
    return (double)(sk_color_get_g(color) / 255.0);
}

CAMLprim value reason_skia_stub_sk_color_float_get_g_byte(value vColor) {
    return caml_copy_double(
               reason_skia_stub_sk_color_float_get_g(Int32_val(vColor)));
}

double reason_skia_stub_sk_color_float_get_r(int32_t color) {
    return (double)(sk_color_get_r(color) / 255.0);
}

CAMLprim value reason_skia_stub_sk_color_float_get_r_byte(value vColor) {
    return caml_copy_double(
               reason_skia_stub_sk_color_float_get_r(Int32_val(vColor)));
}

double reason_skia_stub_sk_color_float_get_a(int32_t color) {
    return (double)(sk_color_get_a(color) / 255.0);
}

CAMLprim value reason_skia_stub_sk_color_float_get_a_byte(value vColor) {
    return caml_copy_double(
               reason_skia_stub_sk_color_float_get_a(Int32_val(vColor)));
}

uint32_t reason_skia_color_float_make_argb(double a, double r, double g,
        double b) {
    int iA = (int)(255.0 * a);
    int iR = (int)(255.0 * r);
    int iG = (int)(255.0 * g);
    int iB = (int)(255.0 * b);
    return (uint32_t)sk_color_set_argb(iA, iR, iG, iB);
}

CAMLprim value reason_skia_color_float_make_argb_byte(value vA, value vR,
        value vG, value vB) {
    return caml_copy_int32(reason_skia_color_float_make_argb(
                               Double_val(vA), Double_val(vR), Double_val(vG), Double_val(vB)));
}

uint32_t reason_skia_stub_sk_color_get_a(int32_t color) {
    return (uint32_t)sk_color_get_a(color);
}

CAMLprim value reason_skia_stub_sk_color_get_a_byte(value vColor) {
    return caml_copy_int32(reason_skia_stub_sk_color_get_a(Int32_val(vColor)));
}

uint32_t reason_skia_stub_sk_color_get_r(int32_t color) {
    return (uint32_t)sk_color_get_r(color);
}

CAMLprim value reason_skia_stub_sk_color_get_r_byte(value vColor) {
    return caml_copy_int32(reason_skia_stub_sk_color_get_r(Int32_val(vColor)));
}

uint32_t reason_skia_stub_sk_color_get_g(int32_t color) {
    return (uint32_t)sk_color_get_g(color);
}

CAMLprim value reason_skia_stub_sk_color_get_g_byte(value vColor) {
    return caml_copy_int32(reason_skia_stub_sk_color_get_g(Int32_val(vColor)));
}

uint32_t reason_skia_stub_sk_color_get_b(int32_t color) {
    return (uint32_t)sk_color_get_b(color);
}

CAMLprim value reason_skia_stub_sk_color_get_b_byte(value vColor) {
    return caml_copy_int32(reason_skia_stub_sk_color_get_b(Int32_val(vColor)));
}

uint32_t reason_skia_stub_sk_color_set_argb(int32_t alpha, int32_t red,
        int32_t green, int32_t blue) {
    return (uint32_t)sk_color_set_argb(alpha, red, green, blue);
}

CAMLprim value reason_skia_stub_sk_color_set_argb_byte(value vAlpha, value vRed,
        value vGreen,
        value vBlue) {
    return caml_copy_int32(reason_skia_stub_sk_color_set_argb(
                               Int32_val(vAlpha), Int32_val(vRed), Int32_val(vGreen), Int32_val(vBlue)));
}
