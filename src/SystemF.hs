{-# LANGUAGE MultiParamTypeClasses #-}

module SystemF where

import Control.Monad.Trans.Class (MonadTrans (lift))
import Control.Monad.Trans.Except (ExceptT, throwE)
import Control.Monad.Trans.Maybe (hoistMaybe, maybeToExceptT)
import Control.Monad.Trans.State (State)
import Data.Map (Map)
import Data.Monoid (First (..))
import SystemF.Error (TypeError (..))
import SystemF.Expr (Expr)
import qualified SystemF.Expr as E
import SystemF.Type (Type)
import qualified SystemF.Type as T
import ToText (ToText (..))

data InferenceTree
    = InferenceTree
        -- | Rule
        String
        -- | Input
        String
        -- | Output
        String
        -- | Children
        [InferenceTree]

type TypeVar a = a

type TermVar a = a

newtype Env a = Env (Map (TermVar a) (Scheme a))

data Scheme a = Scheme
    { vars :: [a]
    , typ :: Type a
    }

newtype Subst a = Subst {getSubst :: Map (TypeVar a) (Type a)}

type Context s a = [Entry s a]

data Entry s a
    = VarBnd (TermVar s) (Type a)
    | TVarBnd (TypeVar a)
    | ETVarBnd (TypeVar a)
    | SETVarBnd (TypeVar a) (Type a)
    | Mark (TypeVar a)
    deriving (Show)

isVarBnd :: Entry s a -> Bool
isVarBnd (VarBnd _ _) = True
isVarBnd _ = False

isTVarBnd :: Entry s a -> Bool
isTVarBnd (TVarBnd _) = True
isTVarBnd _ = False

isETVarBnd :: Entry s a -> Bool
isETVarBnd (ETVarBnd _) = True
isETVarBnd _ = False

isSETVarBnd :: Entry s a -> Bool
isSETVarBnd (SETVarBnd _ _) = True
isSETVarBnd _ = False

isMark :: Entry s a -> Bool
isMark (Mark _) = True
isMark _ = False

instance (Show a, Show s) => ToText (Entry s a) where
    toTexts = shows

substType :: (Eq a) => TypeVar a -> Type a -> Type a -> Type a
substType var replacement ty@(T.Var name)
    | name == var = replacement
    | otherwise = ty
substType var replacement ty@(T.ETVar name)
    | name == var = replacement
    | otherwise = ty
substType _ _ T.Int = T.Int
substType _ _ T.Bool = T.Bool
substType var replacement (T.Arrow t1 t2) =
    T.Arrow
        (substType var replacement t1)
        (substType var replacement t2)
substType var replacement ty@(T.Forall boundVar body)
    | boundVar == var = ty
    | otherwise = T.Forall boundVar (substType var replacement body)

applyContextType :: (Eq a) => Context s a -> Type a -> Type a
applyContextType context = go <$> applyContextTypeOnce context <*> id
  where
    go newType ty
        | newType /= ty = go (applyContextTypeOnce context newType) newType
        | otherwise = ty

applyContextTypeOnce :: (Eq a) => Context s a -> Type a -> Type a
applyContextTypeOnce context ty@(T.ETVar a) =
    maybe ty (applyContextTypeOnce context) $
        getFirst $
            foldMap (First . matches) context
  where
    matches (SETVarBnd name replacement) | name == a = Just replacement
    matches _ = Nothing
applyContextTypeOnce context (T.Arrow t1 t2) =
    T.Arrow
        (applyContextTypeOnce context t1)
        (applyContextTypeOnce context t2)
applyContextTypeOnce context (T.Forall var body) =
    T.Forall var (applyContextTypeOnce context body)
applyContextTypeOnce _ ty = ty

class BiDirectional b a where
    freshTypeVar :: State b a

-- TODO: Context should be part of state
-- TODO: How to convert from s to a
infer :: (BiDirectional b a, Show a, Show s, Eq s) => Context s a -> Expr s a -> ExceptT (TypeError s a) (State b) (Type a, Context s a, InferenceTree)
infer context expr = do
    let input = toText context ++ " ⊢ " ++ show expr
        output ty ctx = input ++ " ⇒ " ++ show ty ++ " ⊣ " ++ toText ctx
    case expr of
        E.Var x -> do
            ty <-
                maybeToExceptT
                    (UnboundVariable x Nothing)
                    (hoistMaybe . getFirst $ foldMap (First . matches) context)
            pure (ty, context, InferenceTree "InfVar" input (output ty context) [])
          where
            matches (VarBnd name ty) | name == x = Just ty
            matches _ = Nothing
        E.Ann expr' ty -> do
            (context', tree) <- check context expr' ty
            pure (ty, context', InferenceTree "InfAnn" input (output ty context') [tree])
        E.LitInt _ ->
            pure (T.Int, context, InferenceTree "InfLitInt" input (output T.Int context) [])
        E.LitBool _ ->
            pure (T.Bool, context, InferenceTree "InfLitBool" input (output T.Bool context) [])
        E.Abs x paramTy body -> do
            b <- lift freshTypeVar
            let newContext = ETVarBnd b : VarBnd x paramTy : context
            (context1, tree) <- check newContext body (T.ETVar b)
            let (left, right') = break isVarBnd context1
                right = drop 1 right'
                resultTy = T.Arrow paramTy (T.ETVar b)
                context' = right ++ filter isSETVarBnd left
            pure (resultTy, context', InferenceTree "InfLam" input (output resultTy context') [tree])
        E.App function argument -> do
            (functionType, context', tree1) <- infer context function
            let functionTypeApplied = applyContextType context' functionType
            (resultTy, context'', tree2) <- inferApp context' functionTypeApplied argument
            pure (resultTy, context'', InferenceTree "InfApp" input (output resultTy context'') [tree1, tree2])
        E.TAbs a body -> do
            let newContext = TVarBnd a : context
            (bodyType, context', tree) <- infer newContext body
            let resolvedBodyType = applyContextType context' bodyType
                (left, right') = break matches context'
                right = drop 1 right'
                context'' = right ++ filter isSETVarBnd left
                resultType = T.Forall a resolvedBodyType
            pure (resultType, context'', InferenceTree "InfTAbs" input (output resultType context'') [tree])
          where
            matches (TVarBnd name) | name == a = True
            matches _ = False
        E.TApp function typeArg -> do
            (functionType, context', tree1) <- infer context function
            case functionType of
                T.Forall a bodyType -> do
                    let resultType = substType a typeArg bodyType
                    pure (resultType, context', InferenceTree "InfTApp" input (output resultType context') [tree1])
                _ -> throwE $ TypeApplicationError functionType Nothing
        E.Let x e1 e2 -> do
            (type1, context1, tree1) <- infer context e1
            let newContext = VarBnd x type1 : context1
            (type2, context2, tree2) <- infer newContext e2
            let (left, right') = break matches context2
                right = drop 1 right'
                context' = right ++ filter isSETVarBnd left
            pure (type2, context', InferenceTree "InfLeft" input (output type2 context') [tree1, tree2])
          where
            matches (VarBnd name _) | name == x = True
            matches _ = False
        E.IfThenElse cond e1 e2 -> do
            (contextCond, treeCond) <- check context cond T.Bool
            (type1, context1, tree1) <- infer contextCond e1
            (type2, context2, tree2) <- infer context1 e2
            (unifiedContext, treeUnify) <- subtype context2 type1 type2
            pure (type2, unifiedContext, InferenceTree "InfIf" input (output type2 unifiedContext) [treeCond, tree1, tree2, treeUnify])
        E.BinOp op e1 e2
            | op `elem` [E.Add, E.Sub, E.Mul, E.Div] -> do
                (context', tree1) <- check context e1 T.Int
                (context'', tree2) <- check context' e2 T.Int
                pure (T.Int, context'', InferenceTree "InfArith" input (output T.Int context'') [tree1, tree2])
            | op `elem` [E.And, E.Or] -> do
                (context', tree1) <- check context e1 T.Bool
                (context'', tree2) <- check context' e2 T.Bool
                pure (T.Bool, context'', InferenceTree "InfBool" input (output T.Bool context'') [tree1, tree2])
            | op `elem` [E.Lt, E.Le, E.Gt, E.Ge] -> do
                (context', tree1) <- check context e1 T.Int
                (context'', tree2) <- check context' e2 T.Int
                pure (T.Bool, context'', InferenceTree "InfCmp" input (output T.Bool context'') [tree1, tree2])
            | op `elem` [E.Eq, E.Ne] -> do
                (type1, context', tree1) <- infer context e1
                (context'', tree2) <- check context' e2 type1
                pure (T.Bool, context'', InferenceTree "InfEq" input (output T.Bool context'') [tree1, tree2])
            | otherwise -> undefined

check :: Context s a -> Expr s a -> Type a -> ExceptT (TypeError s a) (State b) (Context s a, InferenceTree)
check context expr ty = do
    let input = toText context ++ " ⊢ " ++ show expr ++ " ⇐ " ++ toText ty
    case (expr, ty) of
        (E.LitInt _, T.Int) -> pure (context, InferenceTree "ChkLitInt" input (toText context) [])
        (E.LitBool _, T.Bool) -> pure (context, InferenceTree "ChkLitBool" input (toText context) [])
        (E.Abs x _ body, T.Arrow expectedParam resultType) -> do
            let newContext = VarBnd x expectedParam : context
            (context', tree) <- check newContext body resultType
            let right = drop 1 $ dropWhile (not . matches) context'
            pure (right, InferenceTree "ChkLam" input (toText right) [tree])
          where
            matches (VarBnd name _) = name == x
            matches _ = False
        (_, T.Forall a typeBody) -> do
            let newContext = TVarBnd a : context
            (context', tree) <- check newContext expr typeBody
            let right = drop 1 $ dropWhile (not . matches) context'
            pure (right, InferenceTree "ChkAll" input (toText right) [tree])
          where
            matches (TVarBnd name) = name == a
            matches _ = False
        _ -> do
            (inferredType, context', tree1) <- infer context expr
            let inferredApplied = applyContextType context' inferredType
                typeApplied = applyContextType context' ty
            (context'', tree2) <- subtype context' inferredApplied typeApplied
            pure (context'', InferenceTree "ChkSub" input (toText context'') [tree1, tree2])

subtype :: Context s a -> Type a -> Type a -> ExceptT (TypeError s a) (State b) (Context s a, InferenceTree)
subtype context type1 type2 = do
    let input = toText context ++ "⊢ " ++ toText type1 ++ " <: " ++ toText type2
    case (type1, type2) of
        (T.Int, T.Int) -> pure (context, InferenceTree "SubRefl" input (toText context) [])
        (T.Var a, T.Var b) | a == b -> pure (context, InferenceTree "SubReflETVar" input (toText context) [])
        (T.Arrow a1 a2, T.Arrow b1 b2) -> do
            (context', tree1) <- subtype context b1 a1
            (context'', tree2) <- subtype context' a2 b2
            pure (context'', InferenceTree "SubArr" input (toText context'') [tree1, tree2])
        (_, T.Forall b type2Body) -> do
            let newContext = TVarBnd b : context
            (context', tree) <- subtype newContext type1 type2Body
            let right = drop 1 $ dropWhile matches context'
            pure (right, InferenceTree "SubAllR" input (toText right) [tree])
          where
            matches (TVarBnd name) = name == b
            matches _ = False
        (T.Forall a type1Body, _) -> do
            let substType1 = substType a (T.ETVar a) type1Body
            let newContext = Mark a : ETVarBnd a : context
            (context', tree) <- subtype newContext substType1 type2
            let right = drop 1 $ dropWhile matches context'
            pure (right, InferenceTree "SubAllL" input (toText right) [tree])
          where
            matches (Mark name) = name == a
            matches _ = False
        (T.ETVar a, _) | a `notElem` freeVars type2 -> do
            (context', tree) <- instL context a type2
            pure (context', InferenceTree "SubInstL" input (toText context') [tree])
        (_, T.ETVar a) | a `notElem` freeVars type1 -> do
            (context', tree) <- instR context type1 a
            pure (context', InferenceTree "SubInstR" input (toText context') [tree])
        _ -> throwE $ SubtypingError type1 type2 Nothing

instL :: Context s a -> TypeVar a -> Type a -> ExceptT (TypeError s a) (State b) (Context s a, InferenceTree)
instL context a ty = do
    let input = toText context ++ "⊢ " ++ toText a ++ " :=< " ++ toText ty
    case ty of
        T.ETVar b | before context a b -> do
            let (left, right') = break matches context
                right = drop 1 right'
                newContext = left <> [SETVarBnd b (T.ETVar a)] <> right
            pure (newContext, InferenceTree "InstLReach" input (toText newContext) [])
          where
            matches (ETVarBnd name) = name == b
            matches _ = False
        T.Arrow t1 t2 -> do
            a1 <- freshTypeVar
            a2 <- freshTypeVar
            let (left, right') = break matches context
                right = drop 1 right'
                newContext = left <> [SETVarBnd a (T.Arrow (T.ETVar a1) (T.ETVar a2))]
            _

instR :: Context s a -> Type a -> TypeVar a -> ExceptT (TypeError s a) (State b) (Context s a, InferenceTree)
instR = _
