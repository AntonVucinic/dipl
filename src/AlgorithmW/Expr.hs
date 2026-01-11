module AlgorithmW.Expr where

type Expr' = Expr String
data Expr a
    = Var a
    | App (Expr a) (Expr a)
    | Abs a (Expr a)
    | Let a (Expr a) (Expr a)
    | Lit Lit
    | Tuple [Expr a]
    deriving (Show)

data Lit = Int Int | Bool Bool deriving (Show)
