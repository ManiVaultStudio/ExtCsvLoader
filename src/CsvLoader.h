#pragma once

#include <LoaderPlugin.h>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QLineEdit>

#include <actions/DatasetPickerAction.h>
#include <actions/StringAction.h>
#include <actions/ToggleAction.h>
#include <actions/OptionAction.h>

using namespace mv::plugin;

// =============================================================================
// CsvLoader
// =============================================================================

class CsvLoader : public LoaderPlugin
{
    Q_OBJECT

        QFileDialog _fileDialog;

    mv::gui::StringAction _separatorLineEdit;
    mv::gui::ToggleAction _columnHeaderCheckBox;
    mv::gui::ToggleAction _rowHeaderCheckBox;
    mv::gui::ToggleAction _transposeCheckBox;
    mv::gui::ToggleAction _mixedDataHierarchyCheckbox;
    mv::gui::OptionAction _sourceTypeComboBox;
    mv::gui::OptionAction _storageTypeComboBox;
    mv::gui::DatasetPickerAction _datasetPickerAction;
    mv::gui::ToggleAction _derivedDataCheckBox;

public:
    CsvLoader(const PluginFactory* factory);
    ~CsvLoader(void) override;

    void init() override;
    void loadData() Q_DECL_OVERRIDE;
};


// =============================================================================
// Factory
// =============================================================================

class CsvLoaderFactory : public LoaderPluginFactory
{
    Q_INTERFACES(mv::plugin::LoaderPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "nl.tudelft.ExtCsvLoader"
                      FILE  "CsvLoader.json")

public:
    CsvLoaderFactory(void);
    ~CsvLoaderFactory(void) override {}

    /**
     * Produces the plugin
     * @return Pointer to the produced plugin
     */
    LoaderPlugin* produce() override;

    /**
     * Get the data types that the plugin supports
     * @return Supported data types
     */
    mv::DataTypes supportedDataTypes() const override;
};
