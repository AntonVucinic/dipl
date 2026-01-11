{-# LANGUAGE FlexibleInstances #-}
{-# LANGUAGE MultiParamTypeClasses #-}

module AlgorithmW where

import AlgorithmW.Expr (Expr)
import qualified AlgorithmW.Expr as E
import AlgorithmW.Inference (InferenceError, InferenceTree)
import qualified AlgorithmW.Inference as I
import AlgorithmW.Type (Scheme, Type)
import qualified AlgorithmW.Type as T
import Control.Monad (foldM)
import Control.Monad.Trans.Class (lift)
import Control.Monad.Trans.Except (ExceptT, except, runExceptT, throwE)
import Control.Monad.Trans.State (State, evalState, get, put)
import Data.List (intercalate)
import Data.Map (Map)
import qualified Data.Map as Map
import Data.Set (Set)
import qualified Data.Set as Set
import Utils (braces)

type TypeVar a = a

type TermVar a = a

type Env a = Map (TermVar a) (Scheme a)

prettyEnv :: (Show a) => Env a -> String
prettyEnv = braces . intercalate ", " . map (\(k, v) -> show k ++ ": " ++ show v) . Map.toList

newtype Subst a = Subst {getSubst :: Map (TypeVar a) (Type a)}

instance (Ord a) => Semigroup (Subst a) where
    (<>) = composeSubst

instance (Ord a) => Monoid (Subst a) where
    mempty = Subst Map.empty

prettySubst :: (Show a) => Subst a -> String
prettySubst = braces . intercalate ", " . map (\(k, v) -> show v ++ "/" ++ show k) . Map.toList . getSubst

class TypeInference s a where
    freshTypeVar :: State s (TypeVar a)

instance TypeInference [Int] String where
    freshTypeVar = do
        state <- get
        case state of
            [] -> undefined
            (fresh : rest) -> do
                put rest
                pure $ 't' : show fresh

instance TypeInference String Char where
    freshTypeVar = do
        state <- get
        case state of
            "" -> undefined
            (fresh : rest) -> do
                put rest
                pure fresh

applySubst :: (Ord a) => Subst a -> Type a -> Type a
applySubst (Subst subst) (T.Var name) = Map.findWithDefault (T.Var name) name subst
applySubst subst (T.Arrow t1 t2) = T.Arrow (applySubst subst t1) (applySubst subst t2)
applySubst subst (T.Tuple types) = T.Tuple $ map (applySubst subst) types
applySubst _ t = t

composeSubst :: (Ord a) => Subst a -> Subst a -> Subst a
composeSubst s1'@(Subst s1) (Subst s2) = Subst $ Map.foldrWithKey (\k -> Map.insert k . applySubst s1') s1 s2

applySubstEnv :: (Ord a) => Subst a -> Env a -> Env a
applySubstEnv subst = Map.map (applySubstScheme subst)

applySubstScheme :: (Ord a) => Subst a -> Scheme a -> Scheme a
applySubstScheme (Subst subst) scheme = scheme{T.typ = applySubst filteredSubst (T.typ scheme)}
  where
    filteredSubst = Subst $ foldr Map.delete subst (T.vars scheme)

unify :: (Eq a, Show a, Ord a) => Type a -> Type a -> Either (InferenceError a) (Subst a, InferenceTree)
unify t1 t2 = case (t1, t2) of
    (T.Int, T.Int) -> unifyBase
    (T.Bool, T.Bool) -> unifyBase
    (T.Var v, ty) -> unifyVar v ty
    (ty, T.Var v) -> unifyVar v ty
    (T.Arrow a1 a2, T.Arrow b1 b2) -> do
        (s1, tree1) <- unify a1 b1
        let a2' = applySubst s1 a2
            b2' = applySubst s1 b2
        (s2, tree2) <- unify a2' b2'
        let finalSubst = s2 <> s1
        pure
            ( finalSubst
            , I.InferenceTree
                "Unify-Arrow"
                input
                (prettySubst finalSubst)
                [tree1, tree2]
            )
    (T.Tuple ts1, T.Tuple ts2)
        | length ts1 /= length ts2 -> Left $ I.TupleLengthMismatch (length ts1) (length ts2)
        | otherwise -> do
            (subst, trees) <-
                foldM
                    ( \(subst, trees) (t1, t2) -> do
                        let t1' = applySubst subst t1
                            t2' = applySubst subst t2
                        (s, tree) <- unify t1' t2'
                        pure (s <> subst, tree : trees)
                    )
                    (mempty, [])
                    (zip ts1 ts2)
            pure
                ( subst
                , I.InferenceTree
                    "Unify-Tuple"
                    input
                    (prettySubst subst)
                    trees
                )
    _ -> Left $ I.UnificationFailure t1 t2
  where
    input = show t1 ++ " ~ " ++ show t2

    unifyBase =
        Right
            ( mempty
            , I.InferenceTree
                "Unify-Base"
                input
                "{}"
                []
            )

    unifyVar v ty
        | T.Var v == ty =
            Right
                ( mempty
                , I.InferenceTree
                    "Unify-Var-Same"
                    input
                    "{}"
                    []
                )
        | occursCheck v ty = Left $ I.OccursCheck v ty
        | otherwise =
            Right
                ( Subst $ Map.singleton v ty
                , I.InferenceTree
                    "Unify-Var"
                    input
                    ("{" ++ show ty ++ "/" ++ show v ++ "}")
                    []
                )

occursCheck :: (Eq a) => TypeVar a -> Type a -> Bool
occursCheck var (T.Var name) = name == var
occursCheck var (T.Arrow t1 t2) = occursCheck var t1 || occursCheck var t2
occursCheck var (T.Tuple types) = any (occursCheck var) types
occursCheck _ _ = False

infer :: (TypeInference s a, Show a, Ord a) => Env a -> Expr a -> ExceptT (InferenceError a) (State s) (Subst a, Type a, InferenceTree)
infer env expr = case expr of
    E.Lit (E.Int _) -> pure (mempty, T.Int, I.InferenceTree "T-Int" input "Int" [])
    E.Lit (E.Bool _) -> pure (mempty, T.Bool, I.InferenceTree "T-Bool" input "Bool" [])
    E.Var name -> do
        case env Map.!? name of
            Nothing -> throwE $ I.UnboundedVariable name
            Just scheme -> do
                instantiated <- lift $ instantiate scheme
                pure (mempty, instantiated, I.InferenceTree "T-Var" input (show instantiated) [])
    E.Abs param body -> do
        paramType <- T.Var <$> lift freshTypeVar
        let env' = Map.insert param (T.Scheme [] paramType) env
        (s, bodyType, tree) <- infer env' body
        let resultType = T.Arrow (applySubst s paramType) bodyType
        pure (s, resultType, I.InferenceTree "T-Abs" input (show resultType) [tree])
    E.App func arg -> do
        resultType <- T.Var <$> lift freshTypeVar
        (s1, funcType, tree1) <- infer env func
        let envSubst = applySubstEnv s1 env
        (s2, argType, tree2) <- infer envSubst arg
        let funcTypeSubst = applySubst s2 funcType
            expectedFuncType = T.Arrow argType resultType
        (s3, tree3) <- except $ unify funcTypeSubst expectedFuncType
        let finalType = applySubst s3 resultType
        pure (s3 <> s2 <> s1, finalType, I.InferenceTree "T-App" input (show finalType) [tree1, tree2, tree3])
    E.Let var value body -> do
        (s1, valueType, tree1) <- infer env value
        let envSubst = applySubstEnv s1 env
            generalizedType = generalize envSubst valueType
            newEnv = Map.insert var generalizedType envSubst
        (s2, bodyType, tree2) <- infer newEnv body
        pure (s2 <> s1, bodyType, I.InferenceTree "T-Let" input (show bodyType) [tree1, tree2])
    E.Tuple exprs -> do
        (subst, types, trees, _) <-
            foldM
                ( \(subst, types, trees, currentEnv) expr -> do
                    (s, ty, tree) <- infer currentEnv expr
                    pure (s <> subst, ty : types, tree : trees, applySubstEnv s currentEnv)
                )
                (mempty, mempty, mempty, env)
                exprs
        let resultType = T.Tuple types
        pure (subst, resultType, I.InferenceTree "T-Tuple" input (show resultType) trees)
  where
    input = prettyEnv env ++ " ⊢ " ++ show expr ++ " ⇒"

generalize :: (Ord a) => Env a -> Type a -> Scheme a
generalize env ty = T.Scheme (Set.toAscList $ freeTypeVars ty Set.\\ freeTypeVarsEnv env) ty

freeTypeVars :: (Ord a) => Type a -> Set (TypeVar a)
freeTypeVars (T.Var name) = Set.singleton name
freeTypeVars (T.Arrow t1 t2) = freeTypeVars t1 <> freeTypeVars t2
freeTypeVars (T.Tuple types) = mconcat $ map freeTypeVars types
freeTypeVars _ = mempty

freeTypeVarsEnv :: (Ord a) => Env a -> Set (TypeVar a)
freeTypeVarsEnv = mconcat . map (freeTypeVarsScheme . snd) . Map.toList

freeTypeVarsScheme :: (Ord a) => Scheme a -> Set (TypeVar a)
freeTypeVarsScheme scheme = foldr Set.delete (freeTypeVars $ T.typ scheme) (T.vars scheme)

instantiate :: (TypeInference s a, Ord a) => Scheme a -> State s (Type a)
instantiate (T.Scheme vars typ) = do
    flip applySubst typ . Subst
        <$> foldM
            ( \subst var -> do
                fresh <- freshTypeVar
                pure $ Map.insert var (T.Var fresh) subst
            )
            mempty
            vars

inferTypeOnly :: Expr String -> Either (InferenceError String) (Type String)
inferTypeOnly expr =
    evalState
        ( runExceptT $ do
            (_, typ, _) <- infer mempty expr
            pure typ
        )
        [1 :: Int ..]

infer' :: Expr String -> Either (InferenceError String) (Subst String, Type String, InferenceTree)
infer' expr =
    evalState
        (runExceptT $ infer mempty expr)
        [1 :: Int ..]
