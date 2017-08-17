
open Types;
open Utils;
open Migrate_parsetree.Ast_403;

let stringify = {
  prefix: [%str
    let int__to_yojson x => `Int x;
    let float__to_yojson x => `Float x;
    let list__to_yojson convert items => `List (List.map convert items);
    let string__to_yojson x => `String x;
    let array__to_yojson convert items => `List (Array.to_list (Array.map convert items));
    let boolean__to_yojson x => `Bool x;
  ],
  suffix: "__to_yojson",

  variant: fun core_type_converter constructors => {
    open Parsetree;
    open Ast_helper;
    open Longident;
    let cases = List.map
    (fun {pcd_name: {txt, loc}, pcd_args} => {
      let strConst = (Exp.constant (Pconst_string txt None));
      let lid = (Location.mknoloc (Lident txt));
      switch pcd_args {
      | Pcstr_tuple types => {
        switch types {
          | [] => Exp.case (Pat.construct lid None) [%expr `String [%e strConst]]
          | _ => {
            let items = List.mapi
            (fun i typ => Utils.patVar ("arg" ^ (string_of_int i)))
            types;

            let pat = Pat.construct lid (Some (Pat.tuple items));
            let values = List.mapi
            (fun i typ => {
              let larg = Utils.expIdent ("arg" ^ (string_of_int i));
              [%expr [%e core_type_converter typ] [%e larg]]
            })
            types;
            let values = [[%expr `String [%e strConst]], ...values];
            Exp.case pat [%expr `List [%e Utils.list values]]
          }
        }
      }
      /* This isn't supported in 4.02 anyway */
      | Pcstr_record labels => Utils.fail loc "Nope record labels"
      }
    })
    constructors;
    Exp.fun_
    Asttypes.Nolabel
    None
    (Utils.patVar "value")
    (Exp.match_ [%expr value] cases)
  },

  record: fun core_type_converter labels => {
    open Parsetree;
    open Longident;
    open Ast_helper;

    let sets = List.map
    (fun {pld_name: {txt}, pld_type} => {
      let value = Exp.field [%expr value] (Location.mknoloc (Lident txt));
      let strConst = Exp.constant (Pconst_string txt None);
      [%expr ([%e strConst], ([%e core_type_converter pld_type] [%e value]))]
    })
    labels;

    Exp.fun_
      Asttypes.Nolabel
      None
      (Pat.var (Location.mknoloc "value"))
      [%expr `Assoc [%e Utils.list sets]];
  }
};

let parse = {
  prefix: [%str
    let int__from_yojson x => switch x {
    | `Int n => Some n
    | _ => None
    };
    let float__from_yojson x => switch x {
    | `Float n => Some n
    | _ => None
    };
    let list__from_yojson convert items => switch items {
    | `List arr => {
      try {
        let items = List.map (fun item => {
          switch (convert item) {
          | Some x => x
          | None => failwith "Item failed to parse"
          }
        }) arr;
        Some items
      } {
        | _ => None
      }
    }
    | _ => None
    };
    let string__from_yojson value => switch value {
    | `String str => Some str
    | _ => None
    };
    let array__from_yojson convert items => switch items {
    | `List arr => {
      try {
        let items = List.map (fun item => {
          switch (convert item) {
          | Some x => x
          | None => failwith "Item failed to parse"
          }
        }) arr;
        Some (Array.of_list items)
      } {
        | _ => None
      }
    }
    | _ => None
    };
    let boolean__from_yojson value => switch value {
    | `Bool v => Some v
    | _ => None
    };
  ],
  suffix: "__from_yojson",
  variant: fun core_type_converter constructors => {
    simple (Longident.Lident "jkjk")
  },

  record: fun core_type_converter labels => {
    open Parsetree;
    open Longident;
    open Ast_helper;

    let body = Exp.record
    (List.map 
    (fun {pld_name: {txt}} => (
      (Location.mknoloc (Lident txt)),
      Exp.ident (Location.mknoloc (Lident (txt ^ "_extracted")))
    ))
    labels)
    None;

    let body = List.fold_right
    (fun {pld_name: {txt}, pld_type} body => {
      let strConst = Exp.constant (Pconst_string txt None);
      let strPat = Pat.var (Location.mknoloc (txt ^ "_extracted"));

      [%expr switch (get items [%e strConst]) {
      | None => None
      | Some attr => switch ([%e core_type_converter pld_type] attr) {
        | None => None
        | Some [%p strPat] => [%e body]
        }
      }]
    })
    labels
    [%expr Some [%e body]];

    let body = [%expr  {
      let rec get items name => switch items {
      | [] => None
      | [(attr, value), ..._] when attr == name => Some value
      | [_, ...rest] => get rest name
      };

      switch value {
      | `Assoc items => [%e body]
      | _ => None
      }
    }];

    Exp.fun_
      Asttypes.Nolabel
      None
      (Pat.var (Location.mknoloc "value"))
      body;
  }
};

