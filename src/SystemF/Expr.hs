module SystemF.Expr where

import qualified SystemF.Type as T

data Expr s a
    = Var s
    | App (Expr s a) (Expr s a)
    | Abs s (T.Type a) (Expr s a)
    | TApp (Expr s a) (T.Type a)
    | TAbs s (Expr s a)
    | Ann (Expr s a) (T.Type a)
    | LitInt Int
    | LitBool Bool
    | Let s (Expr s a) (Expr s a)
    | IfThenElse (Expr s a) (Expr s a) (Expr s a)
    | BinOp BinOp (Expr s a) (Expr s a)
    deriving (Show)

data BinOp
    = -- Arithmetic
      Add -- +
    | Sub -- -
    | Mul
    | Div -- /
    -- Boolean
    | And -- &&
    | Or -- \||
    --  Comparison
    | Eq -- ==
    | Ne -- !=
    | Lt -- <
    | Le -- <=
    | Gt -- >
    | Ge -- >=
    deriving (Show)
