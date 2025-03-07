#pragma once

#include <LoaderPlugin.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QLineEdit>

#include <actions/DatasetPickerAction.h>

using namespace mv::plugin;

// =============================================================================
// CsvLoader
// =============================================================================

class CsvLoader : public LoaderPlugin
{
    Q_OBJECT

	QFileDialog _fileDialog;

    QLineEdit* _separatorLineEdit;
    QCheckBox* _columnHeaderCheckBox;
    QCheckBox* _rowHeaderCheckBox;
    QCheckBox* _transposeCheckBox;
    QCheckBox* _mixedDataHierarchyCheckbox;
    QComboBox* _sourceTypeComboBox;
    QComboBox* _storageTypeComboBox;
    mv::gui::DatasetPickerAction _datasetPickerAction;

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
