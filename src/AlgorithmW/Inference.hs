module AlgorithmW.Inference where

import qualified AlgorithmW.Type as T

data InferenceTree = InferenceTree
        { rule :: String
        , input :: String
        , output :: String
        , children :: [InferenceTree]
        }
        deriving (Show)

data InferenceError a
        = UnificationFailure
                { expected :: T.Type a
                , actual :: T.Type a
                }
        | OccursCheck
                { var :: a
                , typ :: T.Type a
                }
        | TupleLengthMismatch
                { leftLen :: Int
                , rightLen :: Int
                }
        | UnboundedVariable a
        deriving (Show)
