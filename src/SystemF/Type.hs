module SystemF.Type where

data Type a
    = Var a
    | ETVar a
    | Arrow (Type a) (Type a)
    | Forall a (Type a)
    | Int
    | Bool
    deriving (Show, Eq)
