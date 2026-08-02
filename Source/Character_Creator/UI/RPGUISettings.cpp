#include "UI/RPGUISettings.h"

FName URPGUISettings::GetCategoryName() const
{
	return TEXT("Game");
}

const URPGUISettings* URPGUISettings::Get()
{
	return GetDefault<URPGUISettings>();
}
