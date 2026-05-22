// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//   Created from SFoliageEdit class code

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"
#include "Editor/EditorStyle/Public/EditorStyleSet.h"
#include "AcousticsEdMode.h"

#if ENGINE_MAJOR_VERSION == 4 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 1)
    #define STYLER FEditorStyle
#else
    #define STYLER FAppStyle
#endif

class SAcousticsPalette;
class UAcousticsType;
struct FAcousticsMeshUIInfo;
enum class ECheckBoxState : uint8;

typedef TSharedPtr<FAcousticsMeshUIInfo> FAcousticsMeshUIInfoPtr; // should match typedef in AcousticsEdMode.h

struct FAcousticsEditSharedProperties
{
    static inline FSlateFontInfo GetStandardFont()
    {
        return STYLER::GetFontStyle(TEXT("PropertyWindow.NormalFont"));
    }

    static const FMargin StandardPadding;
    static const FMargin ExtraPadding;
    static const FMargin DoubleBottomPadding;
    static const FMargin StandardLeftPadding;
    static const FMargin StandardExtraTopPadding;
    static const FMargin StandardRightPadding;
    static const FMargin StandardTextMargin;
};

class SAcousticsEdit : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SAcousticsEdit)
    {
    }
    SLATE_END_ARGS()

public:
    /** SCompoundWidget functions */
    void Construct(const FArguments& InArgs);

    /** Does a full refresh on the list. */
    void RefreshFullList();

    /** Will return the status of editing mode */
    // bool IsAcousticsEditorEnabled() const;

    /** Sets the current error text */
    void SetError(FString errorText);

    /** Helper to get help text panel for the tabs */
    static TSharedRef<SWidget> MakeHelpTextWidget(const FString& title, const FString& text);

    AcousticsActiveTab GetActiveTab() const
    {
        if (m_AcousticsEditMode != nullptr)
        {
            return m_AcousticsEditMode->AcousticsUISettings.CurrentTab;
        }
        return AcousticsActiveTab::ObjectTag;
    }

    /** Sync the tab change event to the Edit Mode manager */
    void OnActiveTabChanged(AcousticsActiveTab activeTab)
    {
        if (m_AcousticsEditMode != nullptr)
        {
            switch (activeTab)
            {
                case AcousticsActiveTab::ObjectTag:
                    m_AcousticsEditMode->OnClickObjectTab();
                    break;
                case AcousticsActiveTab::Materials:
                    m_AcousticsEditMode->OnClickMaterialsTab();
                    break;
                case AcousticsActiveTab::Probes:
                    m_AcousticsEditMode->OnClickProbesTab();
                    break;
                case AcousticsActiveTab::Bake:
                    m_AcousticsEditMode->OnClickBakeTab();
                    break;
            }
        }
    }

private:
    /** Creates the toolbar. */
    TSharedRef<SWidget> BuildToolBar();

    /** Checks if the tab mode is Object Mark. */
    bool IsObjectMarkTab() const;

    /** Checks if the tab mode is Materials. */
    bool IsMaterialsTab() const;

    /** Checks if the tab mode is Probes. */
    bool IsProbesTab() const;

    /** Checks if the tab mode is Bake. */
    bool IsBakeTab() const;

    FText GetActiveTabName() const;

    /** Helper function to build the individual tab UIs. */
    TSharedRef<SWidget> BuildObjectTagTab();
    TSharedRef<SWidget> BuildMaterialsTab();
    TSharedRef<SWidget> BuildProbesTab();
    TSharedRef<SWidget> BuildBakeTab();

    EVisibility GetObjectTagTabVisibility() const
    {
        return (IsObjectMarkTab() ? EVisibility::Visible : EVisibility::Collapsed);
    }

    EVisibility GetMaterialsTabVisibility() const
    {
        return (IsMaterialsTab() ? EVisibility::Visible : EVisibility::Collapsed);
    }

    EVisibility GetProbesTabVisibility() const
    {
        return (IsProbesTab() ? EVisibility::Visible : EVisibility::Collapsed);
    }

    EVisibility GetBakeTabVisibility() const
    {
        return (IsBakeTab() ? EVisibility::Visible : EVisibility::Collapsed);
    }

private:
    /** Tooltip text for 'Instance Count" column */
    // FText GetTotalInstanceCountTooltipText() const;

    /** Handler to trigger a refresh of the details view when the active tab changes */
    // void HandleOnTabChanged();

private:
    /** Complete list of available materials   */
    TSharedPtr<class MaterialsLibrary> m_MaterialsLibrary;

    /** Current error message */
    TSharedPtr<class SErrorText> m_ErrorText;

    /** Pointer to the acoustics edit mode. */

    FAcousticsEdMode* m_AcousticsEditMode;
};
