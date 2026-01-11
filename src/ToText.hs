module ToText where

class ToText a where
    {-# MINIMAL toTexts | toText #-}
    toTexts :: a -> ShowS
    toText :: a -> String
    toTextList :: [a] -> ShowS

    toTexts x s = toText x <> s
    toText x = toTexts x ""
    toTextList [] _ = ""
    toTextList (x : xs) s = toTexts x (showl xs)
      where
        showl [] = s
        showl (y : ys) = ", " ++ toTexts y (showl ys)

instance (ToText a) => ToText [a] where
    toTexts = toTextList
