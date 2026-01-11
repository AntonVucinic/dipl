{-# LANGUAGE FlexibleInstances #-}

module AlgorithmW.Type where

import Data.List (intercalate)
import Prelude hiding (showsPrec)

type Type' = Type String
data Type a
    = Var a
    | Arrow (Type a) (Type a)
    | Int
    | Bool
    | Tuple [Type a]
    deriving (Eq, Show)

isArrow :: Type a -> Bool
isArrow (Arrow _ _) = True
isArrow _ = False

typePretty :: Int -> Type String -> ShowS
typePretty _ (Var a) = showString a
typePretty prec (Arrow a b) = showParen (isArrow a) (typePretty prec a) . showString " -> " . typePretty prec b
typePretty _ Int = showString "Int"
typePretty _ Bool = showString "Bool"
typePretty prec (Tuple types) =
    showChar '('
        . showString (intercalate ", " (map (($ "") . typePretty prec) types))
        . showChar ')'

data Scheme a = Scheme
    { vars :: [a]
    , typ :: Type a
    }
    deriving (Show)
