type measureResult = {
  width: float,
  height: float,
};

let measure =
    (~smoothing: Smoothing.t, ~features=[], font, size, text: string) => {
  let {height, _}: FontMetrics.t = FontCache.getMetrics(font, size);

  let shapes = FontCache.shape(~features, font, text);

  let width =
    List.fold_left(
      (acc, run: ShapeResult.shapedRun) =>
        List.fold_left(
          (acc2: float, node: ShapeResult.shapeNode) =>
            acc2 +. node.xAdvance *. size /. node.unitsPerEm,
          acc,
          run.nodes,
        ),
      0.,
      shapes,
    );
  {
    height,
    width,
  };
};
