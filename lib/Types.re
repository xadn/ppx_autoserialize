
open Migrate_parsetree.Ast_403;

type config = {
  prefix: Parsetree.structure,
  suffix: string,
  variant: (Parsetree.core_type => Parsetree.expression) => list Parsetree.constructor_declaration => Parsetree.expression,
  record: (Parsetree.core_type => Parsetree.expression) => list Parsetree.label_declaration => Parsetree.expression,
};