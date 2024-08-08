/***************************************************************************************************
 * Copyright 2023-2024 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part of xTools project.
 *
 * xTools is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#pragma once

#include <QAbstractTableModel>
#include <QActionGroup>
#include <QJsonObject>
#include <QMenu>
#include <QModelIndex>
#include <QTableView>
#include <QWidget>

#include "../AbstractIOUi.h"

namespace Ui {
class AbstractModelUi;
}

namespace xTools {

class DataEditor;
class AbstractModel;
class AbstractModelUi : public AbstractIOUi
{
    Q_OBJECT
public:
    explicit AbstractModelUi(QWidget *parent = nullptr);
    ~AbstractModelUi();

    QTableView *tableView() const;

    void setupIO(AbstractIO *io) override;

protected:
    virtual QList<int> universalColumns() const;

protected:
    QAbstractTableModel *m_model{nullptr};

private:
    Ui::AbstractModelUi *ui{nullptr};
    AbstractModel *m_io{nullptr};
    DataEditor *m_editor{nullptr};

private:
    void onPushButtonClearClicked();
    void onPushButtonDeleteClicked();
    void onPushButtonImportClicked();
    void onPushButtonExportClicked();
    void onPushButtonAddClicked();

    void onCellDoubleClicked(const QModelIndex &index);
};

} // namespace xTools
