module Main where

import AlgorithmW (inferTypeOnly)
import qualified AlgorithmW.Parser as Parser
import AlgorithmW.Type (typePretty)
import Control.Monad (unless)
import Data.Foldable (for_)
import Data.Maybe (listToMaybe)
import Parser (runParser)
import System.Environment (getArgs)

main :: IO ()
main = do
  [filename] <- getArgs
  exprs <- lines <$> readFile filename
  for_ exprs $ \expr -> do
    unless (null expr) $ do
      let parseResult = runParser Parser.expr (Parser.lex expr)
      case listToMaybe parseResult of
        Just (tree, []) -> case inferTypeOnly tree of
          Left err -> putStrLn expr >> putStr "\t" >> print err
          Right typ -> putStrLn $ expr ++ " :: " ++ typePretty 0 typ ""
        _ -> error $ "expr: " ++ expr ++ "\nres: " ++ show parseResult
