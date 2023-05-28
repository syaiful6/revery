let prefix = "skia_wrapped_stub"

let prologue = {|
#include "include/c/sk_types.h"
#include "include/c/gr_context.h"
#include "include/c/sk_canvas.h"
#include "include/c/sk_data.h"
#include "include/c/sk_image.h"
#include "include/c/sk_imagefilter.h"
#include "include/c/sk_paint.h"
#include "include/c/sk_path.h"
#include "include/c/sk_surface.h"
#include "include/c/sk_rrect.h"
#include "include/c/sk_matrix.h"
#include "include/c/sk_typeface.h"
#include "include/c/sk_stream.h"
#include "include/c/sk_string.h"
#include "include/c/sk_svg.h"
#include "include/c/sk_font.h"
|}

let () =
  print_endline prologue;
  Cstubs.Types.write_c Format.std_formatter (module SkiaWrappedTypes.M)
