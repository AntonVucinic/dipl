module Utils where

import Data.Maybe (fromMaybe)
import Data.Monoid (First (..))

braces :: String -> String
braces = (++ "}") . ('{' :)

findMap :: (Foldable t) => b -> (a -> Maybe b) -> t a -> b
findMap def matcher = fromMaybe def . getFirst . foldMap (First . matcher)
