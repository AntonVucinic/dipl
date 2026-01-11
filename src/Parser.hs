{-# LANGUAGE FlexibleInstances #-}
{-# LANGUAGE FunctionalDependencies #-}

module Parser where

import Control.Applicative (Alternative (..))
import Control.Monad (MonadPlus (mzero), unless)
import Data.String (IsString)

newtype Parser s a = Parser {runParser :: s -> [(a, s)]}

instance Functor (Parser s) where
    fmap f (Parser p) = Parser $ \s -> do
        (x, s') <- p s
        pure (f x, s')

instance Applicative (Parser s) where
    pure x = Parser $ \s -> [(x, s)]
    Parser pf <*> Parser p = Parser $ \s -> do
        (f, s') <- pf s
        (x, s'') <- p s'
        pure (f x, s'')

instance Monad (Parser s) where
    Parser p >>= f = Parser $ \s -> do
        (x, s') <- p s
        runParser (f x) s'

instance Alternative (Parser s) where
    empty = Parser $ const []
    Parser pa <|> Parser pb = Parser $ (<|>) <$> pa <*> pb

instance MonadPlus (Parser s)

instance MonadFail (Parser s) where
    fail = const mzero

class Stream s t | s -> t where
    uncons :: s -> Maybe (t, s)

instance Stream [String] String where
    uncons (h : t) = Just (h, t)
    uncons [] = Nothing

get :: (Stream s t) => Parser s t
get = Parser $ \input ->
    case uncons input of
        Just (first, rest) -> [(first, rest)]
        Nothing -> empty

look :: Parser s s
look = Parser $ \s -> [(s, s)]

eof :: Parser [s] ()
eof = do
    s <- look
    unless (null s) empty

satisfy :: (Stream s t) => (t -> Bool) -> Parser s t
satisfy predicate = do
    c <- get
    if predicate c then pure c else empty

char :: (Stream s t, Eq t) => t -> Parser s t
char c = satisfy (== c)

string :: (Stream s t, Eq t) => s -> Parser s s
string s = do
    input <- look
    scan (uncons s) (uncons input)
  where
    scan Nothing _ = pure s
    scan (Just (x, xs)) (Just (y, ys)) | x == y = do _ <- get; scan (uncons xs) (uncons ys)
    scan _ _ = empty

between :: Parser s open -> Parser s close -> Parser s a -> Parser s a
between open close p = do
    _ <- open
    x <- p
    _ <- close
    pure x

sepBy :: Parser s a -> Parser s sep -> Parser s [a]
sepBy p sep = sepBy1 p sep <|> pure []

sepBy1 :: Parser s a -> Parser s sep -> Parser s [a]
sepBy1 p sep = do
    x <- p
    xs <- many (sep *> p)
    pure (x : xs)
