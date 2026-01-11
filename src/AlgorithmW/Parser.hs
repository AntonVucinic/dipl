{-# LANGUAGE LambdaCase #-}

module AlgorithmW.Parser where

import AlgorithmW.Expr (Expr', Lit)
import qualified AlgorithmW.Expr as E
import AlgorithmW.Type (Type')
import qualified AlgorithmW.Type as T
import Control.Applicative (Alternative (..))
import Data.Char (isAlpha, isAsciiLower, isAsciiUpper, isDigit, isSpace)
import Parser (Parser, between, char, satisfy, sepBy1, string)
import Prelude hiding (lex)

type Token = String
type Parser' = Parser [Token]

keywords :: [String]
keywords = ["true", "false"]

lex :: String -> [Token]
lex (c : cs)
    | isSpace c = lex cs
    | isDigit c = (c : numToken) : lex restNum
    | isAlpha c = (c : varToken) : lex restId
    | otherwise = [c] : lex cs
  where
    (numToken, restNum) = span isDigit cs
    (varToken, restId) = span isIdChar cs
lex "" = []

expr :: Parser' Expr'
expr = term

term :: Parser' Expr'
term =
    (E.Let <$ char "let" <*> ident <* char "=" <*> expr <* char "in" <*> expr)
        <|> (E.Abs <$ char "\\" <*> ident <* string ["-", ">"] <*> expr)
        <|> app

app :: Parser' Expr'
app = foldl1 E.App <$> some factor

factor :: Parser' Expr'
factor =
    (E.Var <$> ident)
        <|> (E.Lit <$> lit)
        <|> between (char "(") (char ")") expr
        <|> (E.Tuple <$> between (char "(") (char ")") ((:) <$> expr <* char "," <*> commaList expr))

lit :: Parser' Lit
lit =
    (E.Int . read <$> satisfy (all isDigit))
        <|> (E.Bool True <$ char "true")
        <|> (E.Bool False <$ char "false")

ident :: Parser' String
ident = satisfy $ \case
    name@(first : _) -> not (isDigit first) && all isIdChar name && name `notElem` keywords
    _ -> False

isIdChar :: Char -> Bool
isIdChar c = any ($ c) [isAsciiLower, isAsciiUpper, isDigit, (== '_')]

commaList :: Parser' a -> Parser' [a]
commaList = (`sepBy1` char ",")

typ :: Parser' Type'
typ =
    (T.Arrow <$> atomType <* char "->" <*> typ)
        <|> atomType

atomType :: Parser' Type'
atomType =
    (T.Int <$ char "Int")
        <|> (T.Bool <$ char "Bool")
        <|> (T.Var <$> ident)
        <|> between (char "(") (char "(") typ
        <|> (T.Tuple <$> between (char "(") (char "(") ((:) <$> typ <*> commaList typ))
