// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#include "Modules/ModuleManager.h"

#include "PaperGolfEditorLogging.h"
#include "Logging/LoggingUtils.h"


class FPaperGolfEditor : public IModuleInterface
{
protected:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};


IMPLEMENT_MODULE( FPaperGolfEditor, PaperGolfEditor );

// Logging
DEFINE_LOG_CATEGORY(LogPaperGolfEditor);


void FPaperGolfEditor::StartupModule()
{
	UE_LOG(LogPaperGolfEditor, Log, TEXT("StartupModule"));
}

void FPaperGolfEditor::ShutdownModule()
{
	UE_LOG(LogPaperGolfEditor, Log, TEXT("ShutdownModule"));
}
