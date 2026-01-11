module SystemF.Error where

import SystemF.Type (Type)

data TypeError s a
    = UnboundVariable
        { name :: s
        , expr :: Maybe s
        }
    | ApplicationTypeError
        { actual :: Type a
        , expr :: Maybe s
        }
    | TypeApplicationError
        { actual :: Type a
        , expr :: Maybe s
        }
    | OccursCheck
        { var :: a
        , ty :: Type a
        , expr :: Maybe s
        }
    | SubtypingError
        { left :: Type a
        , right :: Type a
        , expr :: Maybe s
        }
    | InstantiationError
        { var :: a
        , ty :: Type a
        , expr :: Maybe s
        }
