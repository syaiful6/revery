open Utility;

type t = Logs.reporter;

external prerr_native: string => unit = "timber_prerr_native";

let defaultColorsEnabled = Sys.win32 ? false : true;

let console = (~enableColors=defaultColorsEnabled, ()) => {
  // We use `native_error` instead of `prerr` / default formatter to work around:
  // https://github.com/ocaml/ocaml/issues/9252
  let formatter = {
    let buffer = Buffer.create(0);

    Format.make_formatter(
      Buffer.add_substring(buffer),
      () => {
        prerr_native(Buffer.contents(buffer));
        Buffer.clear(buffer);
      },
    );
  };

  if (enableColors) {
    Fmt.set_style_renderer(formatter, `Ansi_tty);
  } else {
    Fmt.set_style_renderer(formatter, `None);
  };

  let getDeltaTime = DeltaTime.generator();

  Logs.{
    report: (_src, level, ~over, k, msgf) => {
      let k = _ => {
        over();
        k();
      };

      msgf((~header as _=?, ~tags=?, fmt) => {
        let tags = Option.get(tags);
        let namespace = Tag.get(Namespace.tag, tags);
        let microlevel = Tag.find(Level.Microlevel.tag, tags);
        let level = (level, microlevel);
        let color = Namespace.pickColor(namespace);
        let style = pp => Fmt.(styled(`Fg(color), pp));

        Format.kfprintf(
          k,
          formatter,
          "%a %a %a : @[" ^^ fmt ^^ "@]@.",
          Level.pp_styled,
          level,
          DeltaTime.pp,
          getDeltaTime(),
          ppf => Fmt.pf(ppf, "%a", style(Namespace.pp)),
          namespace,
        );
      });
    },
  };
};

let file = (~truncate=false, path) => {
  // Open new log file
  let mode =
    truncate
      ? [Open_append, Open_creat, Open_trunc, Open_text]
      : [Open_append, Open_creat, Open_text];
  let channel = open_out_gen(mode, 0o666, path);

  // Write log file header
  Printf.fprintf(
    channel,
    "\n--\nLog started at %s\n--\n\n%!",
    Unix.time() |> Unix.gmtime |> Time.toString,
  );

  let getDeltaTime = DeltaTime.generator();

  Logs.{
    report: (_src, level, ~over, k, msgf) => {
      let k = _ => {
        over();
        k();
      };

      let ppf = Format.formatter_of_out_channel(channel);

      msgf((~header as _=?, ~tags=?, fmt) => {
        let tags = Option.get(tags);
        let namespace = Tag.get(Namespace.tag, tags);
        let microlevel = Tag.find(Level.Microlevel.tag, tags);
        let level = (level, microlevel);

        Format.kfprintf(
          k,
          ppf,
          "%a %a %a : @[" ^^ fmt ^^ "@]@.",
          Level.pp,
          level,
          DeltaTime.pp,
          getDeltaTime(),
          Namespace.pp,
          namespace,
        );
      });
    },
  };
};

let none = Logs.nop_reporter;

let combine = (firstReporter, secondReporter) => {
  Logs.{
    report: (src, level, ~over, k, msgf) => {
      let kret = firstReporter.report(src, level, ~over=() => (), k, msgf);
      secondReporter.report(src, level, ~over, () => kret, msgf);
    },
  };
};

let set = Logs.set_reporter;
