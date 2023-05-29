/**
 * Canvas.re
 *
 * Module for integrating with the Skia canvas
 */
open Revery_Core;

module Log = (val Log.withNamespace("Revery.CanvasContext"));

open Skia;

type t = {
  maybeGPUContext: option(Skia.Gr.Context.t),
  surface: Skia.Surface.t,
  canvas: Skia.Canvas.t,
  mutable rootTransform: option(Skia.Matrix.t),
  maybeWindow: option(Window.t),
};

let createFromSurface = (surface: Skia.Surface.t) => {
  maybeGPUContext: None,
  surface,
  canvas: Skia.Surface.getCanvas(surface),
  rootTransform: None,
  maybeWindow: None,
};

let createGpuContext = () => {
  let interface =
    switch (Skia.Gr.Gl.Interface.makeNative()) {
    | None =>
      Log.debug("Unable to create native interface. Falling back to SDL2...");
      // On Windows - we use the OpenGL ES context, in order to support ANGLE, for improved compatibility.
      // ANGLE provides a Direct3D implementation of the OpenGL ES API.
      Skia.Gr.Gl.Interface.(Sys.win32 ? makeSdl2ES() : makeSdl2());
    | Some(_) as nativeInterface =>
      Log.debug("Native interface created successfully.");
      nativeInterface;
    };
  Log.info("Creating Skia context...");
  Skia.Gr.Context.makeGl(interface);
};

let create = (gpuContext, window: Revery_Core.Window.t) => {
  // Issue #759 - first, let's try to create a native context, since that is the most reliable...
  // We'll fall back to an SDL2 context if not available (ie, in Wayland)
  // TODO: There is still something busted with the way GL is being setup, for the SDL strategy not to work!
  // Likely a fix or change is required here: https://github.com/revery-ui/reason-sdl2/blob/94dcd9094534c693998984fd684c642b0f658a43/src/sdl2_wrapper.cpp#L1065
  switch (gpuContext) {
  | None =>
    Log.error("Unable to create skia context");
    None;
  | Some(glContext) =>
    Log.debug("Skia context created successfully.");
    let framebufferId = Sdl2.Gl.getFramebufferBinding();
    Log.debug(Printf.sprintf("Framebuffer binding %d.", framebufferId));
    let framebufferInfo =
      Gr.Gl.FramebufferInfo.make(
        Unsigned.UInt.of_int(framebufferId),
        Unsigned.UInt.of_int(0x8058),
      );

    let framebufferSize = Window.getFramebufferSize(window);
    let backendRenderTarget =
      Gr.BackendRenderTarget.makeGl(
        framebufferSize.width,
        framebufferSize.height,
        0,
        8,
        framebufferInfo,
      );
    let surfaceProps = SurfaceProps.make(Unsigned.UInt32.of_int(0), RgbH);
    switch (
      Surface.makeFromBackendRenderTarget(
        glContext,
        backendRenderTarget,
        BottomLeft,
        Rgba8888,
        None,
        Some(surfaceProps),
      )
    ) {
    | None =>
      Log.error("Unable to create skia surface");
      None;
    | Some(v) =>
      Log.infof(m =>
        m(
          "Successfully created canvas: %dx%d",
          framebufferSize.width,
          framebufferSize.height,
        )
      );
      let surface = v;
      Some({
        maybeGPUContext: Some(glContext),
        surface,
        canvas: Surface.getCanvas(surface),
        rootTransform: None,
        maybeWindow: Some(window),
      });
    };
  };
};

let createLayer = (~forceCpu=false, ~width, ~height, context: t) => {
  Log.infof(m => m("Creating layer: %ldx%ld", width, height));
  let width =
    if (width <= 0l) {
      1l;
    } else {
      width;
    };

  let height =
    if (height <= 0l) {
      1l;
    } else {
      height;
    };

  let imageInfo = ImageInfo.make(width, height, Rgba8888, Premul, None);

  let createCpuSurface = () => {
    Log.tracef(m => m("Created CPU surface: %ld x %ld", width, height));
    Surface.makeRaster(imageInfo, 0, None);
  };

  let createGpuSurface = gpuContext => {
    Log.trace("Trying to create GPU surface...");
    let surfaceProps = SurfaceProps.make(Unsigned.UInt32.of_int(0), RgbH);
    Skia.Surface.makeRenderTarget(
      gpuContext,
      false,
      imageInfo,
      0,
      BottomLeft,
      Some(surfaceProps),
      false,
    );
  };

  let (gpuContext, maybeSurface) =
    switch (context.maybeGPUContext) {
    | None => (None, createCpuSurface())
    | Some(_) when forceCpu => (None, createCpuSurface())
    | Some(gpuContext) as outContext =>
      let maybeGpuSurface = createGpuSurface(gpuContext);

      // The gpu surface creation can fail, so we should be
      // prepared to fall back to a CPU surface.
      switch (maybeGpuSurface) {
      | Some(_) as surf =>
        Log.tracef(m =>
          m("Successfully created GPU surface: %ld x %ld", width, height)
        );
        (outContext, surf);
      | None =>
        Log.warn("Unable to create GPU surface; falling back to CPU surface");
        (None, createCpuSurface());
      };
    };

  maybeSurface
  |> Option.map(surface => {
       {
         maybeGPUContext: gpuContext,
         surface,
         canvas: Surface.getCanvas(surface),
         rootTransform: None,
         maybeWindow: context.maybeWindow,
       }
     });
};

let width = ({surface, _}) => {
  Surface.getWidth(surface);
};

let height = ({surface, _}) => {
  Surface.getHeight(surface);
};

let resize = (window: Revery_Core.Window.t, v: option(t)) => {
  switch (v) {
  | None => None
  | Some({maybeGPUContext, surface, _}) as v =>
    let framebufferSize = Window.getFramebufferSize(window);
    if (Surface.getWidth(surface) != framebufferSize.width
        || Surface.getHeight(surface) != framebufferSize.height) {
      Log.infof(m =>
        m(
          "Resizing canvas: %dx%d->%dx%d",
          Surface.getWidth(surface),
          Surface.getHeight(surface),
          framebufferSize.width,
          framebufferSize.height,
        )
      );
      create(maybeGPUContext, window);
    } else {
      v;
    };
  };
};

let setRootTransform = (matrix: Skia.Matrix.t, canvas: t) => {
  canvas.rootTransform = Some(matrix);
};

let save = (v: t) => {
  Skia.Canvas.save(v.canvas);
};

let restore = (v: t) => {
  Skia.Canvas.restore(v.canvas);
};

let flush = (v: t) => {
  Skia.Canvas.flush(v.canvas);
};

let translate = (v: t, x: float, y: float) => {
  Skia.Canvas.translate(v.canvas, x, y);
};

let clear = (~color: Skia.Color.t, v: t) => {
  Canvas.clear(v.canvas, color);
};

let drawLayer =
    (~paint: Skia.Paint.t, ~layer: t, ~x: float, ~y: float, target: t) => {
  Surface.draw(
    ~paint=Some(paint),
    ~canvas=target.canvas,
    ~x,
    ~y,
    layer.surface,
  );
};

let drawPath = (~path: Skia.Path.t, ~paint: Paint.t, canvasContext: t) => {
  Canvas.drawPath(canvasContext.canvas, path, paint);
};

let drawRect = (~rect: Skia.Rect.t, ~paint: Paint.t, v: t) => {
  Canvas.drawRect(v.canvas, rect, paint);
};

let drawRectLtwh =
    (
      ~left: float,
      ~top: float,
      ~width: float,
      ~height: float,
      ~paint: Paint.t,
      v: t,
    ) => {
  Canvas.drawRectLtwh(v.canvas, left, top, width, height, paint);
};

let drawRoundRect =
    (~rect: Skia.Rect.t, ~rx: float, ~ry: float, ~paint: Paint.t, v: t) => {
  Canvas.drawRoundRect(v.canvas, rect, rx, ry, paint);
};

let drawOval = (~rect: Skia.Rect.t, ~paint: Paint.t, v: t) => {
  Canvas.drawOval(v.canvas, rect, paint);
};

let drawCircle = (~x: float, ~y: float, ~radius: float, ~paint: Paint.t, v: t) => {
  Canvas.drawCircle(v.canvas, x, y, radius, paint);
};

let drawRRect = (v: t, rRect: Skia.RRect.t, paint) => {
  Canvas.drawRRect(v.canvas, rRect, paint);
};

let drawImage = (~x, ~y, ~width, ~height, ~paint=?, data: Skia.Image.t, v: t) => {
  Canvas.drawImageRect(
    v.canvas,
    data,
    None,
    Rect.makeLtrb(x, y, x +. width, y +. height),
    paint,
  );
};

let drawText = (~paint, ~font, ~x=0., ~y=0., ~text, v: t) => {
  Canvas.drawText(v.canvas, text, x, y, font, paint);
};

let drawTextWithEncoding = (~encoding, ~paint, ~font, ~x=0., ~y=0., ~text, v: t) => {
  Canvas.drawSimpleText(~encoding, v.canvas, text, x, y, font, paint, ());
}

let _topMatrix = Skia.Matrix.make();
let setMatrix = (v: t, mat: Skia.Matrix.t) => {
  switch (v.rootTransform) {
  | None => Canvas.setMatrix(v.canvas, mat)
  | Some(rootMatrix) =>
    Skia.Matrix.concat(_topMatrix, rootMatrix, mat);
    Canvas.setMatrix(v.canvas, _topMatrix);
  };
};

let concat = (transform, context) => {
  Canvas.concat(context.canvas, transform);
};

let clipRect =
    (v: t, ~clipOp: clipOp=Intersect, ~antiAlias=false, rect: Skia.Rect.t) => {
  Canvas.clipRect(v.canvas, rect, clipOp, antiAlias);
};

let clipRRect =
    (v: t, ~clipOp: clipOp=Intersect, ~antiAlias=false, rRect: Skia.RRect.t) => {
  Canvas.clipRRect(v.canvas, rRect, clipOp, antiAlias);
};

let clipPath =
    (v: t, ~clipOp: clipOp=Intersect, ~antiAlias=false, path: Skia.Path.t) => {
  Canvas.clipPath(v.canvas, path, clipOp, antiAlias);
};
