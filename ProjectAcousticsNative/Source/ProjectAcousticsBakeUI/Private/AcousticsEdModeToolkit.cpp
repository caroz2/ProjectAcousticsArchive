// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//   Created from SFoliageEdit class code

#include "AcousticsEdModeToolkit.h"
#include "AcousticsEdMode.h"
#include "Engine/Selection.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "EditorModeManager.h"
#include "SAcousticsEdit.h"

#define LOCTEXT_NAMESPACE "FAcousticsEdModeToolkit"

FAcousticsEdModeToolkit::FAcousticsEdModeToolkit()
{
}

void FAcousticsEdModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
    m_AcousticsEdWidget = SNew(SAcousticsEdit);

    FModeToolkit::Init(InitToolkitHost);

    // LEFT IN FOR NOW AS SAMPLE CODE

    // struct Locals
    //{
    //  static bool IsWidgetEnabled()
    //  {
    //      return GEditor->GetSelectedActors()->Num() != 0;
    //  }

    //  static FReply OnButtonClick(FVector InOffset)
    //  {
    //      USelection* SelectedActors = GEditor->GetSelectedActors();

    //      // Let editor know that we're about to do something that we want to undo/redo
    //      GEditor->BeginTransaction(LOCTEXT("MoveActorsTransactionName", "MoveActors"));

    //      // For each selected actor
    //      for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
    //      {
    //          if (AActor* LevelActor = Cast<AActor>(*Iter))
    //          {
    //              // Register actor in opened transaction (undo/redo)
    //              LevelActor->Modify();
    //              // Move actor to given location
    //              LevelActor->TeleportTo(LevelActor->GetActorLocation() + InOffset, FRotator(0, 0, 0));
    //          }
    //      }

    //      // We're done moving actors so close transaction
    //      GEditor->EndTransaction();

    //      return FReply::Handled();
    //  }

    //  static TSharedRef<SWidget> MakeButton(FText InLabel, const FVector InOffset)
    //  {
    //      return SNew(SButton)
    //          .Text(InLabel)
    //          .OnClicked_Static(&Locals::OnButtonClick, InOffset);
    //  }
    //};

    // const float Factor = 256.0f;

    // SAssignNew(ToolkitWidget, SBorder)
    //  .HAlign(HAlign_Center)
    //  .Padding(25)
    //  .IsEnabled_Static(&Locals::IsWidgetEnabled)
    //  [
    //      SNew(SVerticalBox)
    //      + SVerticalBox::Slot()
    //      .AutoHeight()
    //      .HAlign(HAlign_Center)
    //      .Padding(50)
    //      [
    //          SNew(STextBlock)
    //          .AutoWrapText(true)
    //          .Text(LOCTEXT("HelperLabel", "Select some actors and move them around using buttons below"))
    //      ]
    //      + SVerticalBox::Slot()
    //          .HAlign(HAlign_Center)
    //          .AutoHeight()
    //          [
    //              Locals::MakeButton(LOCTEXT("UpButtonLabel", "Up"), FVector(0, 0, Factor))
    //          ]
    //      + SVerticalBox::Slot()
    //          .HAlign(HAlign_Center)
    //          .AutoHeight()
    //          [
    //              SNew(SHorizontalBox)
    //              + SHorizontalBox::Slot()
    //              .AutoWidth()
    //              [
    //                  Locals::MakeButton(LOCTEXT("LeftButtonLabel", "Left"), FVector(0, -Factor, 0))
    //              ]
    //              + SHorizontalBox::Slot()
    //                  .AutoWidth()
    //                  [
    //                      Locals::MakeButton(LOCTEXT("RightButtonLabel", "Right"), FVector(0, Factor, 0))
    //                  ]
    //          ]
    //      + SVerticalBox::Slot()
    //          .HAlign(HAlign_Center)
    //          .AutoHeight()
    //          [
    //              Locals::MakeButton(LOCTEXT("DownButtonLabel", "Down"), FVector(0, 0, -Factor))
    //          ]

    //  ];
}

FName FAcousticsEdModeToolkit::GetToolkitFName() const
{
    return FName("Project Acoustics");
}

FText FAcousticsEdModeToolkit::GetBaseToolkitName() const
{
    return NSLOCTEXT("AcousticsEdModeToolkit", "DisplayName", "Project Acoustics Bake Tool");
}

class FEdMode* FAcousticsEdModeToolkit::GetEditorMode() const
{
    return GLevelEditorModeTools().GetActiveMode(FAcousticsEdMode::EM_AcousticsEdModeId);
}

#undef LOCTEXT_NAMESPACE
