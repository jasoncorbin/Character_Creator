#include "Interaction/RPGInteractable.h"

// Intentionally empty.
//
// For a BlueprintNativeEvent declared on a UINTERFACE, UHT emits the _Implementation bodies
// inline into the generated header as part of GENERATED_BODY(). Defining them here as well is
// a redefinition (C2084) - which is exactly how this file failed its first build.
//
// The generated defaults return a value-initialised result: priority None, empty label,
// IsInteractEligible false, Interact false. "Ineligible until something says otherwise" is
// the right default anyway - a half-implemented interactable stays silent rather than
// showing a prompt that does nothing.
//
// UInteractableComponent supplies the real fallbacks (DefaultPriority / DefaultPromptLabel)
// for owners that do not implement the interface at all, so nothing depends on these.
